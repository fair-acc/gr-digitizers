/* -*- c++ -*- */
/*
 * Copyright 2021 fair.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DIGITIZERS_39_MAINS_FREQUENCY_CALC_IMPL_H
#define INCLUDED_DIGITIZERS_39_MAINS_FREQUENCY_CALC_IMPL_H

#include <digitizers_39/mains_frequency_calc.h>

namespace gr {
  namespace digitizers_39 {

    class mains_frequency_calc_impl : public mains_frequency_calc
    {
     private:
      float d_lo, d_hi, d_last_state;
      int no_low, no_high;

     public:
      mains_frequency_calc_impl(double expected_sample_rate=2000000.0, float low_threshold=-5, float high_threshold=5);
      ~mains_frequency_calc_impl();

      void mains_threshold(bool* threshold_out, const float* frequenzy_in, int noutput_items);
      // Where all the action really happens
      int work(
              int noutput_items,
              gr_vector_const_void_star &input_items,
              gr_vector_void_star &output_items
      );
    };

  } // namespace digitizers_39
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_39_MAINS_FREQUENCY_CALC_IMPL_H */

