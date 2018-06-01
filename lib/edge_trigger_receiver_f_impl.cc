/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include "edge_trigger_receiver_f_impl.h"

namespace gr {
  namespace digitizers {

    edge_trigger_receiver_f::sptr
    edge_trigger_receiver_f::make(std::string addr, int port)
    {
      edge_trigger_receiver_f::sptr largo = gnuradio::get_initial_sptr
          (new edge_trigger_receiver_f_impl(addr, port));
      return largo;
    }

    edge_trigger_receiver_f_impl::edge_trigger_receiver_f_impl(std::string addr, int port)
      : gr::sync_block("edge_trigger_receiver_f",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(1, 1, sizeof(float)))
    {
      udp::resolver resolver(d_io_service);
      udp::resolver::query query(udp::v4(), addr, std::to_string(port));
      udp::resolver::iterator iter = resolver.resolve(query);
      udp::endpoint endpoint = *iter;
      d_udp_receive = new udp_receiver(d_io_service, endpoint);
    }

    edge_trigger_receiver_f_impl::~edge_trigger_receiver_f_impl()
    {
      delete d_udp_receive;
    }

    int
    edge_trigger_receiver_f_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      std::string message = d_udp_receive->get_msg();
      if (message != "") {
        edge_detect_t edge;
        if(decode_edge_detect(message, edge)) {
          tag_t edge_tag = make_edge_detect_tag(edge);
          edge_tag.offset = nitems_written(0);
          add_item_tag(0, edge_tag);
        }
        else {
          GR_LOG_ERROR(d_logger, "Decoding UDP datagram failed:" + message);
        }
      }

      memset(output_items[0], 0, noutput_items * sizeof(float));

      return noutput_items;
    }

  } /* namespace digitizers */
} /* namespace gr */

