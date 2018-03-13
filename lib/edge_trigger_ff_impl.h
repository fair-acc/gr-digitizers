/* -*- c++ -*- */
/* 
 * Copyright 2018 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_DIGITIZERS_EDGE_TRIGGER_FF_IMPL_H
#define INCLUDED_DIGITIZERS_EDGE_TRIGGER_FF_IMPL_H

#include <digitizers/edge_trigger_ff.h>
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
      boost::asio::io_service& io_service_;
      udp::socket socket_;
      udp::endpoint endpoint_;
      std::string d_host;
      std::string d_port;

     public:
      udp_sender(boost::asio::io_service& io_service,
          const std::string& host, const std::string& port)
         : io_service_(io_service),
           socket_(io_service, udp::endpoint(udp::v4(), 0)),
           d_host(host),
           d_port(port)
      {
        udp::resolver resolver(io_service_);
        udp::resolver::query query(udp::v4(), d_host, d_port);
        udp::resolver::iterator iter = resolver.resolve(query);
        endpoint_ = *iter;
      }

      ~udp_sender()
      {
          socket_.close();
      }

      std::string host_and_port()
      {
        return d_host + ":" + d_port;
      }

      void send(const std::string& msg) {
        socket_.send_to(boost::asio::buffer(msg, msg.size()), endpoint_);
      }
    };


    class edge_trigger_ff_impl : public edge_trigger_ff
    {
     private:
      std::vector<tag_t> d_tags;

      float d_sampling_rate;
      float d_lo_threshold;
      float d_hi_threshold;
      float d_actual_state;

      int64_t d_last_timing_event;
      float d_last_timing_event_delay;
      uint64_t d_samples_since_last_timing_event;

      // We need to keep two triggers because a given trigger might have pretrigger samples
      acq_info_t d_trigger;
      acq_info_t d_trigger_pending;

      boost::asio::io_service d_io_service;
      std::vector<boost::shared_ptr<udp_sender>> d_receivers;

      bool d_send_udp_packet;
      bool d_send_udp_on_raising_edge;
      bool d_sent;  // tels if we've already detected an edge for the last timing event

     public:
      edge_trigger_ff_impl(float sampling, float lo, float hi,
              float initial_state, bool send_udp, std::string host_list, bool send_udp_on_raising_edge);

      ~edge_trigger_ff_impl();

      void forecast(int noutput_items, gr_vector_int &ninput_items_required) override;

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items) override;

      void set_send_udp(bool send_state) override;

     private:
      void send_udp_message(bool detected_rising_edge, const float value);

      void add_edge_and_delay_tags(uint64_t offset, bool detected_rising_edge);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_EDGE_TRIGGER_FF_IMPL_H */

