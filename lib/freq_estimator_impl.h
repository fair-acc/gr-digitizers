/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_FREQ_ESTIMATOR_IMPL_H
#define INCLUDED_DIGITIZERS_FREQ_ESTIMATOR_IMPL_H

#include <digitizers/freq_estimator.h>
#include "utils.h"
namespace gr {
  namespace digitizers {

    class freq_estimator_impl : public freq_estimator
    {
     private:
      average_filter<float> d_sig_avg;
      average_filter<double> d_freq_avg;
      float d_samp_rate;
      float d_avg_freq;
      double d_prev_zero_dist;
      float d_old_sig_avg;
      int d_decim;
      int d_counter;
     public:
      freq_estimator_impl(float samp_rate, int signal_window_size, int averager_window_size, int decim);
      ~freq_estimator_impl();

      void forecast(int noutput_items,
          gr_vector_int &ninput_items_required);


      int general_work(int noutput_items,
         gr_vector_int &ninput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);

      void update_design(int decim);

    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_FREQ_ESTIMATOR_IMPL_H */

