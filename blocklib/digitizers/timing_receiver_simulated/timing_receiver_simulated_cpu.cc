#include "timing_receiver_simulated_cpu.h"
#include "timing_receiver_simulated_cpu_gen.h"

#include <gnuradio/tag.h>

namespace gr::digitizers {

timing_receiver_simulated_cpu::timing_receiver_simulated_cpu(const block_args& args)
    : INHERITED_CONSTRUCTORS, d_stop_requested(false)
{
    if (d_thread.joinable()) {
        d_stop_requested = true;
        d_thread.join();
    }
}

bool timing_receiver_simulated_cpu::start()
{
    d_stop_requested = false;
    const auto trigger_name = pmtf::get_as<std::string>(*this->param_trigger_name);
    const auto interval = std::chrono::milliseconds{ pmtf::get_as<int64_t>(
        *this->param_tag_time_interval) };
    const auto offset = pmtf::get_as<int64_t>(*this->param_trigger_offset);
    d_thread = std::jthread(
        [this, trigger_name, interval, offset] { loop(trigger_name, interval, offset); });
    return block::start();
}

bool timing_receiver_simulated_cpu::stop()
{
    if (d_thread.joinable()) {
        d_stop_requested = true;
        d_thread.join();
    }
    return block::stop();
}

void timing_receiver_simulated_cpu::loop(std::string trigger_name,
                                         std::chrono::milliseconds interval,
                                         double trigger_offset)
{
    while (!d_stop_requested) {
        const auto now = std::chrono::system_clock::now();
        const auto now_ns_since_epoch =
            std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch())
                .count();
        std::map<std::string, pmtf::pmt> msg = {
            { tag::TRIGGER_NAME, trigger_name },
            { tag::TRIGGER_OFFSET, trigger_offset },
            { tag::TRIGGER_TIME, static_cast<int64_t>(now_ns_since_epoch) }
        };
        d_msg_out->post(std::move(msg));

        if (d_stop_requested) {
            return;
        }

        std::this_thread::sleep_for(interval);
    }
}

} // namespace gr::digitizers
