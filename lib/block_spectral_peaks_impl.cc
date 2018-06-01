/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "block_spectral_peaks_impl.h"
#include <gnuradio/filter/firdes.h>

namespace gr {
  namespace digitizers {

    block_spectral_peaks::sptr
    block_spectral_peaks::make(double samp_rate,
        int vec_len,
        int n_med,
        int n_avg,
        int n_prox)
    {
      std::vector<int> out_sig;
      out_sig.push_back(vec_len * sizeof(float));
      out_sig.push_back(sizeof(float));
      out_sig.push_back(sizeof(float));
      std::vector<int> in_sig = {
          static_cast<int>(sizeof(float) * vec_len),
          static_cast<int>(sizeof(float)),
          static_cast<int>(sizeof(float))
      };
      return gnuradio::get_initial_sptr
        (new block_spectral_peaks_impl(samp_rate, vec_len, out_sig, in_sig, n_med, n_avg, n_prox));
    }

    /*
     * The private constructor
     */
    block_spectral_peaks_impl::block_spectral_peaks_impl(double samp_rate,
        int vec_len,
        std::vector<int> out_sig,
        std::vector<int> in_sig,
        int n_med,
        int n_avg,
        int n_prox)
      : gr::hier_block2("block_spectral_peaks",
              gr::io_signature::makev(3, 3, in_sig),
              gr::io_signature::makev(3, 3, out_sig))
    {
      d_med_avg = digitizers::median_and_average::make(vec_len, n_med, n_avg);
      d_peaks = digitizers::peak_detector::make(samp_rate, vec_len, n_prox);
      connect(self(), 0, d_med_avg, 0);
      connect(self(), 0, d_peaks, 0);
      connect(self(), 1, d_peaks, 2);
      connect(self(), 2, d_peaks, 3);
      connect(d_med_avg, 0, d_peaks, 1);
      connect(d_med_avg, 0, self(), 0);
      connect(d_peaks, 0, self(), 1);
      connect(d_peaks, 1, self(), 2);
    }

    block_spectral_peaks_impl::~block_spectral_peaks_impl()
    {
    }


  } /* namespace digitizers */
} /* namespace gr */

