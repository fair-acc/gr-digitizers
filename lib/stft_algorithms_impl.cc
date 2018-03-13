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
#include "stft_algorithms_impl.h"

namespace gr {
  namespace digitizers {

    stft_algorithms::sptr
    stft_algorithms::make(double samp_rate, double delta_t, int window_size, int wintype, int alg_id, double fq_low, double fq_hi, int nbins)
    {
      switch(alg_id) {
      case 0:
        return gnuradio::get_initial_sptr
            (new fft_impl(samp_rate, delta_t, window_size, static_cast<filter::firdes::win_type>(wintype),fq_low, fq_hi, nbins));
        break;
      case 1:
        return gnuradio::get_initial_sptr
            (new goertzel_impl(samp_rate, delta_t, window_size, fq_low, fq_hi, nbins, false));
        break;
      case 2:
        return gnuradio::get_initial_sptr
            (new goertzel_impl(samp_rate, delta_t, window_size, 0, samp_rate/2, window_size, true));
        break;
      default:
        std::cout<<"STFT alg_id must be either 0-FFT, 1-Goertzel, 2-DFT!\nis:"<<alg_id<<" -> Defaulting to FFT!\n";
        return gnuradio::get_initial_sptr
            (new fft_impl(samp_rate, delta_t, window_size, static_cast<filter::firdes::win_type>(wintype),fq_low, fq_hi, nbins));
        break;
      }
    }

    //alg_id = 0 (FFT)
    fft_impl::fft_impl(double samp_rate,
        double delta_t,
        int window_size,
        filter::firdes::win_type wintype,
        double fq_low,
        double fq_hi,
        int nbins)
    : gr::hier_block2("stft_algorithms",
        gr::io_signature::make(1, 1, sizeof(float)),
        gr::io_signature::make(2, 2, sizeof(float)*window_size)),
      d_samp_rate(samp_rate),
      d_window_size(window_size),
      d_wintype(wintype)
    {
      d_str2vec = stream_to_vector_overlay_ff::make(d_window_size * 2, delta_t * samp_rate);
      d_com2magphase = blocks::complex_to_magphase::make(d_window_size);
      d_fft = fft::fft_vfc::make(d_window_size * 2,
              true,
              filter::firdes::window(d_wintype, d_window_size * 2,
              6.76));
            d_vec2str = blocks::vector_to_stream::make(sizeof(gr_complex),
                d_window_size*2);
            d_keep_m = blocks::keep_m_in_n::make(sizeof(gr_complex),
                d_window_size,
                d_window_size * 2,
                0);
            d_half_str2vec = blocks::stream_to_vector::make(sizeof(gr_complex),
                d_window_size);

      /* Connections */
      //input
      connect(self(), 0, d_str2vec, 0);
      //stream->vector -> FFT -> keep front half -> post
      connect(d_str2vec,0, d_fft, 0);
      connect(d_fft, 0, d_vec2str, 0);
      connect(d_vec2str, 0, d_keep_m, 0);
      connect(d_keep_m, 0, d_half_str2vec, 0);
      connect(d_half_str2vec, 0, d_com2magphase, 0);
      //output
      connect(d_com2magphase, 0, self(), 0);
      connect(d_com2magphase, 1, self(), 1);
    }

    fft_impl::~fft_impl()
    {

    }

    void
    fft_impl::set_window_type(int wintype)
    {
      d_wintype = static_cast<filter::firdes::win_type>(wintype);
      d_fft->set_window(filter::firdes::window(d_wintype, d_window_size * 2, 6.76));
    }

    void
    fft_impl::set_samp_rate(double samp_rate)
    {
    }

    void
    fft_impl::set_freqs(double fq_low, double fq_hi)
    {
    }


    //alg_id = 1 (Goertzel)
    goertzel_impl::goertzel_impl(double samp_rate,
        double delta_t,
        int window_size,
        double fq_low,
        double fq_hi,
        int nbins,
        bool range_fixed)
    : gr::hier_block2("stft_algorithms",
        gr::io_signature::make(1, 1, sizeof(float)),
        gr::io_signature::make(2, 2, sizeof(float)*nbins)),
      d_samp_rate(samp_rate),
      d_window_size(window_size),
      d_fq_lo(fq_low),
      d_fq_hi(fq_hi),
      d_nbins(nbins),
      d_range_fixed(range_fixed)
    {
      d_str2vec = stream_to_vector_overlay_ff::make(d_window_size, samp_rate * delta_t);
      d_vec2str = blocks::vector_to_stream::make(sizeof(float), d_window_size);
      d_com2magphase = blocks::complex_to_magphase::make(d_nbins);
      d_strs2vec = blocks::streams_to_vector::make(sizeof(gr_complex), d_nbins);
      d_goe.clear();
      d_goe.reserve(d_nbins);
      double fq_step = (d_fq_hi - d_fq_lo) / (1.0 * d_nbins);

      /* Connections */
      //input
      connect(self(), 0, d_str2vec, 0);
      connect(d_str2vec, 0, d_vec2str, 0);
      for(int i = 0; i < d_nbins; i++) {
        double ith_freq = d_fq_lo + (i * fq_step);
        d_goe.push_back(
            fft::goertzel_fc::make(d_samp_rate, d_window_size, ith_freq)
        );
        connect(d_vec2str, 0, d_goe.at(i), 0);
        connect(d_goe.at(i), 0, d_strs2vec, i);
      }
      connect(d_strs2vec, 0, d_com2magphase, 0);

      //output
      connect(d_com2magphase, 0, self(), 0);
      connect(d_com2magphase, 1, self(), 1);
    }

    goertzel_impl::~goertzel_impl()
    {

    }

    void
    goertzel_impl::set_window_type(int wintype)
    {}

    void
    goertzel_impl::set_samp_rate(double samp_rate)
    {

      d_samp_rate = samp_rate;
      for(int i = 0; i < d_nbins; i++) {
        d_goe.at(i)->set_rate(d_samp_rate);
      }
      if(d_range_fixed){
        set_freqs(0.0, d_samp_rate/2);
      }
    }

    void
    goertzel_impl::set_freqs(double fq_low, double fq_hi)
    {
      if( (d_range_fixed && fq_low == 0.0 && fq_hi == d_samp_rate/2.0) ||
          !d_range_fixed ) {
        d_fq_lo = fq_low;
        d_fq_hi = fq_hi;
        double fq_step = (d_fq_hi - d_fq_lo) / (1.0 * d_nbins);

        for(int i = 0; i < d_nbins; i++) {
          double ith_freq = d_fq_lo + (i * fq_step);
          d_goe.at(i)->set_freq(ith_freq);
        }
      }
    }




  } /* namespace digitizers */
} /* namespace gr */

