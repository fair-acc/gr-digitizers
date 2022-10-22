#pragma once

#include <gnuradio/digitizers/edge_trigger_receiver.h>

#include "edge_trigger_utils.h"
#include "utils.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include <thread>

using boost::asio::ip::udp;

namespace gr::digitizers {

class udp_receiver : private boost::noncopyable {
private:
    const int                        MAX_LENGTH = 4096;
    boost::asio::io_service         &d_io_service;
    udp::socket                      d_socket;
    boost::scoped_ptr<boost::thread> d_thread;

    std::vector<char>                d_data;
    boost::system::error_code        d_ec;
    int                              d_length;
    concurrent_queue<std::string>    d_messages;

public:
    udp_receiver(boost::asio::io_service &io_service, udp::endpoint endpoint)
        : d_io_service(io_service), d_socket(d_io_service, endpoint), d_data(MAX_LENGTH), d_ec(), d_length(-1) {
        d_socket.async_receive(
                boost::asio::buffer(d_data, MAX_LENGTH),
                boost::bind(&udp_receiver::handle_receive_from, this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
        d_thread.reset(new boost::thread(
                boost::bind(&boost::asio::io_service::run, &d_io_service)));
    }

    ~udp_receiver() {
        d_socket.close();
        d_io_service.stop();
        d_thread->join();
    }

    void
    handle_receive_from(const boost::system::error_code &error,
            size_t                                       bytes_recvd) {
        if (!error && bytes_recvd > 0) {
            d_messages.push(std::string(d_data.data(), bytes_recvd));
        }
        d_socket.async_receive(
                boost::asio::buffer(d_data, MAX_LENGTH),
                boost::bind(&udp_receiver::handle_receive_from, this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
    }

    std::string
    get_msg() {
        std::string message = "";
        if (d_messages.size() > 0) {
            d_messages.wait_and_pop(message, boost::chrono::milliseconds(0));
        }
        return message;
    }
};

class edge_trigger_receiver_cpu : public edge_trigger_receiver {
public:
    explicit edge_trigger_receiver_cpu(const block_args &args);

    work_return_t work(work_io &wio) override;

private:
    std::string                   d_client_list;
    boost::asio::io_service       d_io_service;
    std::unique_ptr<udp_receiver> d_udp_receive;

    std::queue<std::string>       d_queue;
};

} // namespace gr::digitizers
