/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_EDGE_TRIGGER_RECEIVER_F_IMPL_H
#define INCLUDED_DIGITIZERS_EDGE_TRIGGER_RECEIVER_F_IMPL_H

#include <digitizers/edge_trigger_receiver_f.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/algorithm/string/split.hpp>
#include <digitizers/edge_trigger_utils.h>
#include <utils.h>

using boost::asio::ip::udp;

namespace gr {
  namespace digitizers {

    void handle_receive_redicator(const boost::system::error_code& ec, std::size_t length, void* recv);

    class udp_receiver : private boost::noncopyable
    {
     private:
      const int MAX_LENGTH = 4096;
      boost::asio::io_service& d_io_service;
      udp::socket d_socket;
      boost::scoped_ptr<boost::thread> d_thread;

      std::vector<char> d_data;
      boost::system::error_code d_ec;
      int d_length;
      concurrent_queue<std::string> d_messages;

     public:

      udp_receiver(boost::asio::io_service& io_service, udp::endpoint endpoint)
         : d_io_service(io_service),
           d_socket(d_io_service, endpoint),
           d_data(MAX_LENGTH),
           d_ec(),
           d_length(-1)
      {
        d_socket.async_receive(
          boost::asio::buffer(d_data, MAX_LENGTH),
          boost::bind(&udp_receiver::handle_receive_from, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
        d_thread.reset(new boost::thread(
          boost::bind(&boost::asio::io_service::run, &d_io_service)));
      }

      ~udp_receiver()
      {
        d_socket.close();
        d_io_service.stop();
        d_thread->join();
      }

      void
      handle_receive_from(const boost::system::error_code& error,
            size_t bytes_recvd)
      {
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
      get_msg()
      {
        std::string message = "";
        if(d_messages.size() > 0) {
          d_messages.wait_and_pop(message, boost::chrono::milliseconds(0));
        }
        return message;
      }
    };


    class edge_trigger_receiver_f_impl : public edge_trigger_receiver_f
    {
     private:
      std::string d_client_list;
      boost::asio::io_service d_io_service;
      udp_receiver * d_udp_receive;

      std::queue<std::string> d_queue;

     public:

      edge_trigger_receiver_f_impl(std::string addr, int port);

      ~edge_trigger_receiver_f_impl();

      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_EDGE_TRIGGER_RECEIVER_F_IMPL_H */

