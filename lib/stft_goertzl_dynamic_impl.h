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

#ifndef INCLUDED_DIGITIZERS_STFT_GOERTZL_DYNAMIC_IMPL_H
#define INCLUDED_DIGITIZERS_STFT_GOERTZL_DYNAMIC_IMPL_H

#include <digitizers/stft_goertzl_dynamic.h>
#include <tuple>
#include <mutex>

namespace gr {
  namespace digitizers {

    class stft_goertzl_dynamic_impl : public stft_goertzl_dynamic
    {
     private:
      double d_samp_length;
      double d_delta_t;
      int d_nbins;
      long d_last_tag_offset;
      int d_vectors_passed;
      std::vector<double> d_lows;
      std::vector<double> d_highs;
      std::vector<double> d_times;
      std::mutex d_fq_access;

      std::vector<double> calc_bin_freqs();

     public:

      stft_goertzl_dynamic_impl(double samp_rate,
        double delta_t,
        int window_size,
        int nbins,
        double window_effectiveness,
        std::vector<double> lower_bounds,
        std::vector<double> upper_bounds);

      ~stft_goertzl_dynamic_impl();

      int work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items);

      void set_samp_rate(double samp_rate);

      void update_bounds(double window_effectiveness,
        std::vector<double> center,
        std::vector<double> width);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_STFT_GOERTZL_DYNAMIC_IMPL_H */

