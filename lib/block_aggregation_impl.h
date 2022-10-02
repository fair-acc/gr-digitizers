/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_BLOCK_AGGREGATION_IMPL_H
#define INCLUDED_DIGITIZERS_BLOCK_AGGREGATION_IMPL_H

#include <digitizers/block_aggregation.h>
#include <digitizers/block_custom_filter.h>
#include <digitizers/aggregation_helper.h>
#include <digitizers/signal_averager.h>
#include <digitizers/decimate_and_adjust_timebase.h>
#include <gnuradio/blocks/delay.h>
#include <gnuradio/blocks/multiply.h>
#include "digitizers/status.h"

namespace gr {
  namespace digitizers {

  /**
   * \brief Block aggregation implementation
   *
   * User can adjust the parameters in runtime, but the algorithmID has to be chosen
   * beforehand.
   */
    class block_aggregation_impl : public block_aggregation
    {
     private:
      block_custom_filter::sptr d_fil0, d_fil1, d_fil2, d_fil3;
      aggregation_helper::sptr d_helper;
      blocks::delay::sptr d_del;
      blocks::multiply_ff::sptr d_sqr;
      // the reason of using this helper block is to fix the timebase_info tag
      decimate_and_adjust_timebase::sptr d_keep_nth;
      signal_averager::sptr d_avg;
      double d_samp_rate;
      bool d_averaging;
     public:

      block_aggregation_impl(algorithm_id_t alg_id,
          int decim,
          int delay,
          const std::vector<float> &fir_taps,
          double low_freq,
          double up_freq,
          double tr_width,
          const std::vector<double> &fb_user_taps,
          const std::vector<double> &fw_user_taps,
          double samp_rate);

      ~block_aggregation_impl();

      void update_design(int decim,
          const std::vector<float> &fir_taps,
          double low_freq,
          double up_freq,
          double tr_width,
          const std::vector<double> &fb_user_taps,
          const std::vector<double> &fw_user_taps,
          double samp_rate) override;

      void update_design(float delay,
          const std::vector<double> &fb_user_taps,
          const std::vector<double> &fw_user_taps,
          double gain,
          double low_cutoff_freq,
          double high_cutoff_freq,
          double transition_width,
          gr::fft::window::win_type,
          double beta) override;

      void update_out_delay(int delay);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_BLOCK_AGGREGATION_IMPL_H */

