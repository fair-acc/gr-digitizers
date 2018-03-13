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

#ifndef INCLUDED_DIGITIZERS_STFT_GOERTZL_DYNAMIC_DECIMATED_IMPL_H
#define INCLUDED_DIGITIZERS_STFT_GOERTZL_DYNAMIC_DECIMATED_IMPL_H

#include <digitizers/stft_goertzl_dynamic_decimated.h>
#include <digitizers/stft_goertzl_dynamic.h>
#include <digitizers/stream_to_vector_overlay_ff.h>
#include <gnuradio/blocks/vector_to_stream.h>


namespace gr {
  namespace digitizers {

    class stft_goertzl_dynamic_decimated_impl : public stft_goertzl_dynamic_decimated
    {
     private:
      stft_goertzl_dynamic::sptr d_stft;
      stream_to_vector_overlay_ff::sptr d_str2vec;
      blocks::vector_to_stream::sptr d_vec2str;

     public:
      stft_goertzl_dynamic_decimated_impl(double samp_rate,
          double delta_t,
          int window_size,
          int nbins,
          double time_of_transition,
          std::vector<double> center_fq,
          std::vector<double> width_fq);

      ~stft_goertzl_dynamic_decimated_impl();

      void set_samp_rate(double samp_rate);

      void update_bounds(double time_of_transition,
        std::vector<double> center_fq,
        std::vector<double> width_fq);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_STFT_GOERTZL_DYNAMIC_DECIMATED_IMPL_H */

