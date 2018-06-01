/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_PEAK_DETECTOR_IMPL_H
#define INCLUDED_DIGITIZERS_PEAK_DETECTOR_IMPL_H

#include <digitizers/peak_detector.h>

namespace gr {
  namespace digitizers {

    class peak_detector_impl : public peak_detector
    {
     private:
      int d_vec_len;
      int d_prox;
      double d_freq;

     public:
      peak_detector_impl(double samp_rate,
          int vec_len,
          int proximity);
      ~peak_detector_impl();

      // Where all the action really happens
      void forecast(int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_PEAK_DETECTOR_IMPL_H */

