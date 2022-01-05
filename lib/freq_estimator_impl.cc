/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "freq_estimator_impl.h"

namespace gr {
  namespace digitizers {

    freq_estimator::sptr
    freq_estimator::make(float samp_rate, int signal_window_size, int averager_window_size, int decim)
    {
      return gnuradio::get_initial_sptr
        (new freq_estimator_impl(samp_rate, signal_window_size, averager_window_size, decim));
    }

    freq_estimator_impl::freq_estimator_impl(float samp_rate, int signal_window_size, int averager_window_size, int decim)
      : gr::block("freq_estimator",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(1, 1, sizeof(float))),
              d_sig_avg(signal_window_size),
              d_freq_avg(averager_window_size),
              d_samp_rate(samp_rate),
              d_avg_freq(0.0),
              d_prev_zero_dist(0.0),
              d_old_sig_avg(0.0),
              d_decim(decim),
              d_counter(decim)
    {
        set_tag_propagation_policy(TPP_CUSTOM);
    }

    freq_estimator_impl::~freq_estimator_impl()
    {
    }

    void
    freq_estimator_impl::forecast(int noutput_items,
      gr_vector_int &ninput_items_required)
    {
      ninput_items_required[0] = d_decim * noutput_items;
    }

    int
    freq_estimator_impl::general_work(int noutput_items,
        gr_vector_int &ninput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      int n_in = ninput_items[0];
      int n_out = 0;
      const float *in = (const float *) input_items[0];
      float *out = (float *) output_items[0];
      float new_sig_avg;
      const auto samp0_count = nitems_read(0);

      for(int i = 0; i < n_in; i++) {
        //average the signal to get rid of noise
        new_sig_avg = d_sig_avg.add(in[i]);

        d_prev_zero_dist += 1;

        //previous running average is differently signed as this one -> signal went through zero
        if(! ((new_sig_avg < 0.0 && d_old_sig_avg < 0.0) || (new_sig_avg >= 0.0 && d_old_sig_avg >= 0.0))){

          //interpolate where the averaged signal passed through zero
          double x = (-new_sig_avg) / (new_sig_avg - d_old_sig_avg);

          //estimate of the frequency is an inverse of the distances between zero values.
          d_avg_freq = d_freq_avg.add(d_samp_rate / (2.0 * (d_prev_zero_dist + x)));

          //starter offset for next estimate.
          d_prev_zero_dist = -x;
        }
        //for the next iteration
        d_old_sig_avg = new_sig_avg;

        //decimate and post
        d_counter--;
        if(d_counter <= 0) {
          d_counter = d_decim;
          out[n_out] = d_avg_freq;
          n_out++;
        }
      }

      // add tags with fixed offset to the output stream
      std::vector<gr::tag_t> trigger_tags;
      get_tags_in_range(trigger_tags, 0, samp0_count, samp0_count + n_in);
      for (const auto &trigger_tag : trigger_tags) {
          tag_t new_tag = trigger_tag;
          if(d_decim != 0)
              new_tag.offset = uint64_t(trigger_tag.offset / d_decim);
          else
              new_tag.offset = uint64_t(trigger_tag.offset);
          add_item_tag(0, new_tag);
      }

      consume_each(n_in);
      return n_out;
    }

    void
    freq_estimator_impl::update_design(int decim)
    {
      d_decim = decim;
    }

  } /* namespace digitizers */
} /* namespace gr */

