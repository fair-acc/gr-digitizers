/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_EDGE_TRIGGER_FF_IMPL_H
#define INCLUDED_DIGITIZERS_EDGE_TRIGGER_FF_IMPL_H

#include <digitizers/edge_trigger_ff.h>
#include <digitizers/edge_trigger_utils.h>
#include <digitizers/tags.h>

#include <gnuradio/blocks/pdu.h>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <stdio.h>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

#include "utils.h"

using boost::asio::ip::udp;

namespace gr {
  namespace digitizers {

    class udp_sender : private boost::noncopyable
    {
     private:
      boost::asio::io_service& d_io_service;
      udp::socket d_socket;
      udp::endpoint d_endpoint;
      std::string d_host;
      std::string d_port;

     public:
      udp_sender(boost::asio::io_service& io_service,
          const std::string& host, const std::string& port)
         : d_io_service(io_service),
           d_socket(io_service, udp::endpoint(udp::v4(), 0)),
           d_host(host),
           d_port(port)
      {
        udp::resolver resolver(d_io_service);
        udp::resolver::query query(udp::v4(), d_host, d_port);
        udp::resolver::iterator iter = resolver.resolve(query);
        d_endpoint = *iter;
      }

      ~udp_sender()
      {
          d_socket.close();
      }

      std::string host_and_port()
      {
        return d_host + ":" + d_port;
      }

      void send(const std::string& msg) {
        d_socket.send_to(boost::asio::buffer(msg, msg.size()), d_endpoint);
      }
    };


    class edge_trigger_ff_impl : public edge_trigger_ff
    {
     private:
      float d_sampling_rate;
      float d_lo_threshold;
      float d_hi_threshold;
      bool d_actual_state;  // low or high
      uint64_t d_timeout_samples;

      acq_info_t d_acq_info;

      boost::asio::io_service d_io_service;
      std::vector<boost::shared_ptr<udp_sender>> d_receivers;

      boost::circular_buffer<wr_event_t> d_wr_events;
      boost::circular_buffer<uint64_t> d_triggers;       // offsets
      boost::circular_buffer<uint64_t> d_detected_edges; // offsets

      bool d_send_udp_packet;
      bool d_send_udp_on_raising_edge;

     public:
      edge_trigger_ff_impl(float sampling, float lo, float hi,
              float initial_state, bool send_udp, std::string host_list,
              bool send_udp_on_raising_edge, float timeout);

      ~edge_trigger_ff_impl();

      void forecast(int noutput_items, gr_vector_int &ninput_items_required) override;

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items) override;

      void set_send_udp(bool send_state) override;

      bool start () override;

     private:
      /*!
       * This method will try to send UPD datagram but that is only if edge has been detected and
       * WR event/tag received.
       */
      void send_edge_detect_info(uint64_t trigger, uint64_t detected_edge, const wr_event_t &wr_event, bool make_tags);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_EDGE_TRIGGER_FF_IMPL_H */

