#pragma once

#include <gnuradio/digitizers/timing_receiver_simulated.h>

#include <gnuradio/tag.h>

#define ZMQ_BUILD_DRAFT_API
#include <zmq.h>

#include <cerrno>
#include <chrono>
#include <thread>

namespace gr::digitizers {

class timing_receiver_simulated_cpu : public timing_receiver_simulated
{
    std::atomic<bool> _stop_requested;
    std::jthread _thread;

public:
    explicit timing_receiver_simulated_cpu(const block_args& args);

    ~timing_receiver_simulated_cpu()
    {
        if (_thread.joinable()) {
            _stop_requested = true;
            _thread.join();
        }
    }

    bool start() override
    {
        _stop_requested = false;
        const auto trigger_name = std::get<std::string>(*this->param_trigger_name);
        const auto offset = std::get<double>(*this->param_trigger_offset);
        const auto mode = static_cast<timing_receiver_simulation_mode_t>(
            std::get<int>(*this->param_simulation_mode));

        if (mode == timing_receiver_simulation_mode_t::PERIODIC_INTERVAL) {
            const auto interval = std::chrono::milliseconds{ std::get<int64_t>(
                *this->param_tag_time_interval) };
            _thread = std::jthread([this, trigger_name, interval, offset] {
                periodical_loop(trigger_name, interval, offset);
            });
        }
        else if (mode == timing_receiver_simulation_mode_t::ZEROMQ) {
            const auto endpoint = std::get<std::string>(*this->param_zmq_endpoint);
            const auto group = std::get<std::string>(*this->param_zmq_group);
            _thread = std::jthread([this, endpoint, group, trigger_name, offset] {
                zmq_loop(endpoint, group, trigger_name, offset);
            });
        }
        return block::start();
    }

    bool stop() override
    {
        if (_thread.joinable()) {
            _stop_requested = true;
            _thread.join();
        }
        return block::stop();
    }

    void post_timing_message(std::string name, int64_t timestamp_ns, double offset)
    {
        std::map<std::string, pmtv::pmt> msg = { { tag::TRIGGER_NAME, name },
                                                 { tag::TRIGGER_OFFSET, offset },
                                                 { tag::TRIGGER_TIME, timestamp_ns } };

        d_msg_out->post(std::move(msg));
    }

private:
    void periodical_loop(std::string trigger_name,
                         std::chrono::milliseconds interval,
                         double trigger_offset)
    {
        while (!_stop_requested) {
            const auto now = std::chrono::system_clock::now();
            const auto now_ns_since_epoch =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    now.time_since_epoch())
                    .count();
            std::map<std::string, pmtv::pmt> msg = {
                { tag::TRIGGER_NAME, trigger_name },
                { tag::TRIGGER_OFFSET, trigger_offset },
                { tag::TRIGGER_TIME, static_cast<int64_t>(now_ns_since_epoch) }
            };
            d_msg_out->post(std::move(msg));

            if (_stop_requested) {
                return;
            }

            std::this_thread::sleep_for(interval);
        }
    }

    void zmq_loop(std::string zmq_endpoint,
                  std::string zmq_group,
                  std::string trigger_name,
                  double trigger_offset)
    {
        using namespace std::chrono_literals;

        auto term_ctx = [](auto ctx) { zmq_ctx_term(ctx); };
        auto context = std::unique_ptr<void, decltype(term_ctx)>(zmq_ctx_new(), term_ctx);

        if (!context) {
            d_logger->error("Could not create ZeroMQ context");
            return;
        }

        auto close_socket = [](auto socket) { zmq_close(socket); };
        auto socket = std::unique_ptr<void, decltype(close_socket)>(
            zmq_socket(context.get(), ZMQ_DISH), close_socket);

        d_logger->debug("Binding to ZeroMQ endpoint '{}'", zmq_endpoint);
        const auto rc = zmq_bind(socket.get(), zmq_endpoint.data());
        if (rc != 0) {
            d_logger->error("Could not bind to ZeroMQ endpoint '{}'", zmq_endpoint);
            return;
        }

        d_logger->debug("Joining ZeroMQ group '{}'", zmq_group);
        if (zmq_join(socket.get(), zmq_group.data()) != 0) {
            d_logger->error("Could not join ZeroMQ group '{}'", zmq_group);
            return;
        }

        while (!_stop_requested) {
            zmq_pollitem_t items[] = { { socket.get(), 0, ZMQ_POLLIN, 0 } };
            zmq_poll(&items[0], 1, 100 /*ms*/);

            if (_stop_requested) {
                return;
            }

            if ((items[0].revents & ZMQ_POLLIN) == 0) {
                continue;
            }

            std::array<char, sizeof(int64_t)> buf;

            bool have_more = true;

            while (have_more) {
                errno = 0;
                const auto nread =
                    zmq_recv(socket.get(), buf.data(), buf.size(), ZMQ_DONTWAIT);

                if (nread < 0 || errno == EAGAIN) {
                    have_more = false;
                    continue;
                }

                if (static_cast<std::size_t>(nread) < sizeof(int64_t)) {
                    d_logger->warn(
                        "Received message too short: {} bytes where {} are expected",
                        nread,
                        buf.size());
                    continue;
                }

                int64_t timestamp_ns = 0;
                for (std::size_t n = 0; n < buf.size(); ++n) {
                    timestamp_ns *= 256;
                    timestamp_ns += reinterpret_cast<unsigned char&>(buf[n]);
                }

                std::map<std::string, pmtv::pmt> msg = {
                    { tag::TRIGGER_NAME, trigger_name },
                    { tag::TRIGGER_OFFSET, trigger_offset },
                    { tag::TRIGGER_TIME, timestamp_ns }
                };

                d_msg_out->post(std::move(msg));
            }
        }
    }
};

} // namespace gr::digitizers
