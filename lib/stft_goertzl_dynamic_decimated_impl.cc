/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "stft_goertzl_dynamic_decimated_impl.h"

namespace gr {
  namespace digitizers {

    stft_goertzl_dynamic_decimated::sptr
    stft_goertzl_dynamic_decimated::make(double samp_rate, double delta_t, int window_size, int nbins, int bounds_decimation)
    {
      return gnuradio::get_initial_sptr
        (new stft_goertzl_dynamic_decimated_impl(samp_rate, delta_t, window_size, nbins, bounds_decimation));
    }

    /*
     * The private constructor
     */
    stft_goertzl_dynamic_decimated_impl::stft_goertzl_dynamic_decimated_impl(double samp_rate,
        double delta_t,
        int window_size,
        int nbins,
        int bounds_decimation)
      : gr::hier_block2("stft_goertzl_dynamic_decimated",
              gr::io_signature::make(3, 3, sizeof(float)),
              gr::io_signature::make(3, 3, sizeof(float) * nbins))
    {
      double samp_rate_decimated = samp_rate / (1.0 * bounds_decimation);
      d_str2vec_sig = stream_to_vector_overlay_ff::make(window_size, samp_rate , delta_t);
      d_str2vec_min = stream_to_vector_overlay_ff::make(1, samp_rate_decimated , delta_t);
      d_str2vec_max = stream_to_vector_overlay_ff::make(1, samp_rate_decimated , delta_t);
      d_stft = stft_goertzl_dynamic::make(samp_rate, window_size, nbins);

      connect(self(), 0, d_str2vec_sig, 0);
      connect(self(), 1, d_str2vec_min, 0);
      connect(self(), 2, d_str2vec_max, 0);
      connect(d_str2vec_sig, 0, d_stft, 0);
      connect(d_str2vec_min, 0, d_stft, 1);
      connect(d_str2vec_max, 0, d_stft, 2);

      connect(d_stft, 0, self(), 0);
      connect(d_stft, 1, self(), 1);
      connect(d_stft, 2, self(), 2);
    }

    stft_goertzl_dynamic_decimated_impl::~stft_goertzl_dynamic_decimated_impl()
    {
    }


    void
    stft_goertzl_dynamic_decimated_impl::set_samp_rate(double samp_rate)
    {
      d_stft->set_samp_rate(samp_rate);
    }



  } /* namespace digitizers */
} /* namespace gr */

