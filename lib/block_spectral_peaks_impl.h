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

#ifndef INCLUDED_DIGITIZERS_BLOCK_SPECTRAL_PEAKS_IMPL_H
#define INCLUDED_DIGITIZERS_BLOCK_SPECTRAL_PEAKS_IMPL_H

#include <digitizers/block_spectral_peaks.h>
#include <digitizers/median_and_average.h>
#include <digitizers/peak_detector.h>
#include <gnuradio/blocks/vector_to_stream.h>

namespace gr {
  namespace digitizers {

    class block_spectral_peaks_impl : public block_spectral_peaks
    {
     private:
      // Nothing to declare in this block.
      median_and_average::sptr d_med_avg;
      peak_detector::sptr d_peaks;
      blocks::vector_to_stream::sptr d_vec2str;
     public:
      block_spectral_peaks_impl(double samp_rate,
          int vec_len,
          std::vector<int> out_sig,
          double low_freq,
          double up_freq,
          int n_med,
          int n_avg,
          int n_prox);

      ~block_spectral_peaks_impl();

      // Where all the action really happens
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_BLOCK_SPECTRAL_PEAKS_IMPL_H */

