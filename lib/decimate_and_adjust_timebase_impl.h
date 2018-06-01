/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_DECIMATE_AND_ADJUST_TIMEBASE_IMPL_H
#define INCLUDED_DIGITIZERS_DECIMATE_AND_ADJUST_TIMEBASE_IMPL_H

#include <digitizers/decimate_and_adjust_timebase.h>

namespace gr {
  namespace digitizers {

    class decimate_and_adjust_timebase_impl : public decimate_and_adjust_timebase
    {
     private:
      double d_delay;

     public:
      decimate_and_adjust_timebase_impl(int decimation, double delay);
      ~decimate_and_adjust_timebase_impl();

      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_DECIMATE_AND_ADJUST_TIMEBASE_IMPL_H */

