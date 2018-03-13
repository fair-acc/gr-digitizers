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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "block_aggregation_impl.h"
#include "digitizers/status.h"

namespace gr {
  namespace digitizers {

    block_aggregation::sptr
    block_aggregation::make(int alg_id,
        int decim,
        int delay,
        const std::vector<float> &fir_taps,
        double low_freq,
        double up_freq,
        double tr_width,
        const std::vector<double> &fb_user_taps,
        const std::vector<double> &fw_user_taps,
        double samp_rate)
    {
      return gnuradio::get_initial_sptr
        (new block_aggregation_impl(algorithm_id_t(alg_id), decim, delay, fir_taps, low_freq, up_freq, tr_width, fb_user_taps, fw_user_taps, samp_rate));
    }

    /*
     * The private constructor
     */
    block_aggregation_impl::block_aggregation_impl(algorithm_id_t alg_id,
            int decim,
            int delay,
            const std::vector<float> &fir_taps,
            double low_freq,
            double up_freq,
            double tr_width,
            const std::vector<double> &fb_user_taps,
            const std::vector<double> &fw_user_taps,
            double samp_rate)
      : gr::hier_block2("block_aggregation",
              gr::io_signature::make(2, 2, sizeof(float)),
              gr::io_signature::make(2, 2, sizeof(float))),
              d_averaging(false)
    {
      if(alg_id == algorithm_id_t::AVERAGE){
        d_averaging = true;
        d_avg = signal_averager::make(2, decim);
        connect(self(), 0, d_avg, 0);
        connect(self(), 1, d_avg, 1);

        connect(d_avg, 0, self(), 0);
        connect(d_avg, 1, self(), 1);
      }
      else{
        d_helper = aggregation_helper::make(decim, (up_freq-low_freq)/samp_rate);
        d_fil0 = block_custom_filter::make(alg_id, 1, fir_taps, low_freq, up_freq, tr_width, fb_user_taps, fw_user_taps, samp_rate);
        d_fil1 = block_custom_filter::make(alg_id, 1, fir_taps, low_freq, up_freq, tr_width, fb_user_taps, fw_user_taps, samp_rate);
        d_fil2 = block_custom_filter::make(alg_id, 1, fir_taps, low_freq, up_freq, tr_width, fb_user_taps, fw_user_taps, samp_rate);
        d_fil3 = block_custom_filter::make(alg_id, 1, fir_taps, low_freq, up_freq, tr_width, fb_user_taps, fw_user_taps, samp_rate);
        d_keep_nth = gr::digitizers::decimate_and_adjust_timebase::make(decim);
        d_sqr = gr::blocks::multiply_ff::make(1);
        d_del = gr::blocks::delay::make(4, delay);

        //connections:

        //in
        connect(self(), 0, d_fil0, 0);

        //upper branch connecting to out
        connect(d_fil0, 0, d_del, 0);
        connect(d_del, 0, d_keep_nth, 0);
        connect(d_keep_nth, 0, self(), 0);

        //first branch square of filtered in
        connect(d_fil0, 0, d_sqr, 0);
        connect(d_fil0, 0, d_sqr, 1);
        connect(d_sqr, 0, d_fil1, 0);

        //second branch of filtered in
        connect(d_fil0, 0, d_fil2, 0);

        //sigma in
        connect(self(), 1, d_fil3, 0);

        //helper circuit
        connect(d_fil1, 0, d_helper, 0);
        connect(d_fil2, 0, d_helper, 1);
        connect(d_fil3, 0, d_helper, 2);

        //sigma
        connect(d_helper, 0, self(), 1);
      }
    }

    /*
     * Our virtual destructor.
     */
    block_aggregation_impl::~block_aggregation_impl()
    {
    }


    void
    block_aggregation_impl::update_design(int delay,
        const std::vector<float> &fir_taps,
        double low_freq,
        double up_freq,
        double tr_width,
        const std::vector<double> &fb_user_taps,
        const std::vector<double> &fw_user_taps,
        double samp_rate)
    {
      if(!d_averaging) {
        d_helper->update_design((up_freq-low_freq)/samp_rate);
        d_fil0->update_design(fir_taps, low_freq, up_freq, tr_width, fb_user_taps, fw_user_taps, samp_rate);
        d_fil1->update_design(fir_taps, low_freq, up_freq, tr_width, fb_user_taps, fw_user_taps, samp_rate);
        d_fil2->update_design(fir_taps, low_freq, up_freq, tr_width, fb_user_taps, fw_user_taps, samp_rate);
        d_fil3->update_design(fir_taps, low_freq, up_freq, tr_width, fb_user_taps, fw_user_taps, samp_rate);
        d_del->set_dly(delay);
      }
    }

  } /* namespace digitizers */
} /* namespace gr */

