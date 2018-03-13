/* -*- c++ -*- */
/* 
 * Copyright 2017 <+YOU OR YOUR COMPANY+>.
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

#ifndef INCLUDED_DIGITIZERS_BLOCK_AGGREGATION_IMPL_H
#define INCLUDED_DIGITIZERS_BLOCK_AGGREGATION_IMPL_H

#include <digitizers/block_aggregation.h>
#include <digitizers/block_custom_filter.h>
#include <digitizers/aggregation_helper.h>
#include <digitizers/signal_averager.h>
#include <digitizers/decimate_and_adjust_timebase.h>
#include <gnuradio/blocks/delay.h>
#include <gnuradio/blocks/multiply_ff.h>
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
          double samp_rate);

      void update_out_delay(int delay);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_BLOCK_AGGREGATION_IMPL_H */

