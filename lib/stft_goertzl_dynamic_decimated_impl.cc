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
#include "stft_goertzl_dynamic_decimated_impl.h"

namespace gr {
  namespace digitizers {

    stft_goertzl_dynamic_decimated::sptr
    stft_goertzl_dynamic_decimated::make(double samp_rate,
        double delta_t,
        int window_size,
        int nbins,
        double time_of_transition,
        std::vector<double> center_fq,
        std::vector<double> width_fq)
    {
      return gnuradio::get_initial_sptr
        (new stft_goertzl_dynamic_decimated_impl(samp_rate,
            delta_t,
            window_size,
            nbins,
            time_of_transition,
            center_fq,
            width_fq));
    }

    /*
     * The private constructor
     */
    stft_goertzl_dynamic_decimated_impl::stft_goertzl_dynamic_decimated_impl(double samp_rate,
        double delta_t,
        int window_size,
        int nbins,
        double time_of_transition,
        std::vector<double> center_fq,
        std::vector<double> width_fq)
      : gr::hier_block2("stft_goertzl_dynamic_decimated",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(3, 3, sizeof(float) * nbins))
    {
      d_str2vec = stream_to_vector_overlay_ff::make(window_size, samp_rate * delta_t);
      d_vec2str = blocks::vector_to_stream::make(sizeof(float), window_size);
      d_stft = stft_goertzl_dynamic::make(samp_rate, delta_t, window_size, nbins, time_of_transition, center_fq, width_fq);

      connect(self(), 0, d_str2vec, 0);
      connect(d_str2vec, 0, d_vec2str, 0);
      connect(d_vec2str, 0, d_stft, 0);

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

    void
    stft_goertzl_dynamic_decimated_impl::update_bounds(double time_of_transition,
      std::vector<double> center_fq,
      std::vector<double> width_fq)
    {
      d_stft->update_bounds(time_of_transition, center_fq, width_fq);
    }


  } /* namespace digitizers */
} /* namespace gr */

