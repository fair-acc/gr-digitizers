#pragma once

#include "tags.h"

#include <gnuradio/digitizers/edge_trigger.h>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/noncopyable.hpp>

using boost::asio::ip::udp;

namespace gr::digitizers {

class udp_sender : private boost::noncopyable {
private:
    boost::asio::io_service &d_io_service;
    udp::socket              d_socket;
    udp::endpoint            d_endpoint;
    std::string              d_host;
    std::string              d_port;

public:
    udp_sender(boost::asio::io_service &io_service,
            const std::string &host, const std::string &port)
        : d_io_service(io_service), d_socket(io_service, udp::endpoint(udp::v4(), 0)), d_host(host), d_port(port) {
        udp::resolver           resolver(d_io_service);
        udp::resolver::query    query(udp::v4(), d_host, d_port);
        udp::resolver::iterator iter = resolver.resolve(query);
        d_endpoint                   = *iter;
    }

    ~udp_sender() {
        d_socket.close();
    }

    std::string host_and_port() {
        return d_host + ":" + d_port;
    }

    void send(const std::string &msg) {
        d_socket.send_to(boost::asio::buffer(msg, msg.size()), d_endpoint);
    }
};

template<class T>
class edge_trigger_cpu : public edge_trigger<T> {
public:
    edge_trigger_cpu(const typename edge_trigger<T>::block_args &args);

    work_return_t work(work_io &wio) override;

    bool          start() override;

private:
    bool                                     d_actual_state = false;
    acq_info_t                               d_acq_info;

    boost::asio::io_service                  d_io_service;
    std::vector<std::shared_ptr<udp_sender>> d_receivers;

    boost::circular_buffer<wr_event_t>       d_wr_events;
    boost::circular_buffer<uint64_t>         d_triggers;       // offsets
    boost::circular_buffer<uint64_t>         d_detected_edges; // offsets
};

} // namespace gr::digitizers
