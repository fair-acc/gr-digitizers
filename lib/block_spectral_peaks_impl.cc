/* -*- c++ -*- */
/* 
 * Copyright 2018 <+YOU OR YOUR COMPANY+>.
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
#include "block_spectral_peaks_impl.h"
#include <gnuradio/filter/firdes.h>

namespace gr {
  namespace digitizers {

    block_spectral_peaks::sptr
    block_spectral_peaks::make(double samp_rate,
        int vec_len,
        double low_freq,
        double up_freq,
        int n_med,
        int n_avg,
        int n_prox)
    {
      std::vector<int> out_sig;
      out_sig.push_back(vec_len * sizeof(float));
      out_sig.push_back(sizeof(float));
      out_sig.push_back(sizeof(float));
      return gnuradio::get_initial_sptr
        (new block_spectral_peaks_impl(samp_rate, vec_len, out_sig, low_freq, up_freq, n_med, n_avg, n_prox));
    }

    /*
     * The private constructor
     */
    block_spectral_peaks_impl::block_spectral_peaks_impl(double samp_rate,
        int vec_len,
        std::vector<int> out_sig,
        double low_freq,
        double up_freq,
        int n_med,
        int n_avg,
        int n_prox)
      : gr::hier_block2("block_spectral_peaks",
              gr::io_signature::make(1, 1, sizeof(float) * vec_len),
              gr::io_signature::makev(3, 3, out_sig))
    {
      int lower_bin = 2.0*low_freq/samp_rate * vec_len;
      int upper_bin = 2.0*up_freq/samp_rate * vec_len;
      d_med_avg = digitizers::median_and_average::make(vec_len, n_med, n_avg);
      d_peaks = digitizers::peak_detector::make(samp_rate, vec_len, lower_bin, upper_bin, n_prox);
      connect(self(), 0, d_med_avg, 0);
      connect(self(), 0, d_peaks, 0);
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

