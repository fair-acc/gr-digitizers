/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
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
        d_samp_rate(samp_rate),
        d_averaging(false)
    {
      if(alg_id == algorithm_id_t::AVERAGE){
        d_averaging = true;
        d_avg = signal_averager::make(2, decim, samp_rate);
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
        double delay_approx = d_fil0->get_delay_approximation();

        // Note, d_keep_nth is the only path where the tags are propagated
        d_keep_nth = gr::digitizers::decimate_and_adjust_timebase::make(decim, delay_approx,samp_rate);
        d_sqr = gr::blocks::multiply_ff::make(1);
        d_del = gr::blocks::delay::make(sizeof(float), delay);

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

    void
    block_aggregation_impl::update_design(float delay,
        const std::vector<double> &fb_user_taps,
        const std::vector<double> &fw_user_taps,
        double gain,
        double low_cutoff_freq,
        double high_cutoff_freq,
        double transition_width,
        gr::filter::firdes::win_type win_type,
        double beta = 6.76)
    {
      if(!d_averaging) {
        d_helper->update_design((high_cutoff_freq - low_cutoff_freq) / d_samp_rate);

        auto fir_taps = gr::filter::firdes::low_pass(gain, d_samp_rate, high_cutoff_freq, transition_width, win_type, beta);

        d_fil0->update_design(fir_taps, low_cutoff_freq, high_cutoff_freq, transition_width, fb_user_taps, fw_user_taps, d_samp_rate);
        d_fil1->update_design(fir_taps, low_cutoff_freq, high_cutoff_freq, transition_width, fb_user_taps, fw_user_taps, d_samp_rate);
        d_fil2->update_design(fir_taps, low_cutoff_freq, high_cutoff_freq, transition_width, fb_user_taps, fw_user_taps, d_samp_rate);
        d_fil3->update_design(fir_taps, low_cutoff_freq, high_cutoff_freq, transition_width, fb_user_taps, fw_user_taps, d_samp_rate);
        auto delay_samples = static_cast<int>(d_samp_rate * delay);
        d_del->set_dly(delay_samples);
      }
    }

  } /* namespace digitizers */
} /* namespace gr */

