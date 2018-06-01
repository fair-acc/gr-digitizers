/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_SIGNAL_AVERAGER_IMPL_H
#define INCLUDED_DIGITIZERS_SIGNAL_AVERAGER_IMPL_H

#include <digitizers/signal_averager.h>

namespace gr {
  namespace digitizers {

    class signal_averager_impl : public signal_averager
    {
     private:
      int d_num_ports;

     public:
      signal_averager_impl(int num_inputs, int window_size);
      ~signal_averager_impl();

      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_SIGNAL_AVERAGER_IMPL_H */

