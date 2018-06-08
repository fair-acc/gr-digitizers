/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "stft_algorithms_impl.h"
#include <gnuradio/block.h>

namespace gr {
  namespace digitizers {

    stft_algorithms::sptr
    stft_algorithms::make(double samp_rate, double delta_t, int window_size, int wintype, stft_algorithm_id_t alg_id, double fq_low, double fq_hi, int nbins)
    {
      switch(alg_id) {
      case FFT:
        return gnuradio::get_initial_sptr
            (new fft_impl(samp_rate, delta_t, window_size, static_cast<filter::firdes::win_type>(wintype),fq_low, fq_hi, nbins));
        break;
      case GOERTZEL:
        return gnuradio::get_initial_sptr
            (new goertzel_impl(samp_rate, delta_t, window_size, fq_low, fq_hi, nbins, false));
        break;
      case DFT:
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
        gr::io_signature::make(3, 3, sizeof(float)*window_size)),
      d_wintype(wintype),
      d_samp_rate(samp_rate),
      d_window_size(window_size)

    {


      std::vector<float> freqs;
      freqs.resize(d_window_size);
      if(d_window_size > 1) {
        double freq_window = fq_hi - fq_low;
        for(int i = 0; i < d_window_size; i++) {
          freqs.at(i) = fq_low + ((static_cast<double>(i) / static_cast<double>(d_window_size -1)) * freq_window);
        }
      }
      else {
        freqs.at(0) = (fq_low + fq_hi) / 2.0;
      }
      d_str2vec = stream_to_vector_overlay_ff::make(d_window_size * 2, samp_rate, delta_t);
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
      d_freqs = blocks::vector_source_f::make(freqs, true, d_window_size);

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
      connect(d_freqs, 0, self(), 2);
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
        gr::io_signature::make(3, 3, sizeof(float)*nbins)),
      d_samp_rate(samp_rate),
      d_window_size(window_size),
      d_fq_lo(fq_low),
      d_fq_hi(fq_hi),
      d_nbins(nbins),
      d_range_fixed(range_fixed)
    {

      std::vector<float> freqs;
      freqs.resize(d_nbins);
      if(d_nbins > 1) {
        double freq_window = fq_hi - fq_low;
        for(int i = 0; i < d_nbins; i++) {
          freqs.at(i) = fq_low + ((static_cast<double>(i) / static_cast<double>(d_nbins -1)) * freq_window);
        }
      }
      else {
        freqs.at(0) = (fq_low + fq_hi) / 2.0;
      }
      d_str2vec = stream_to_vector_overlay_ff::make(d_window_size, samp_rate, delta_t);
      d_vec2str = blocks::vector_to_stream::make(sizeof(float), d_window_size);
      d_com2magphase = blocks::complex_to_magphase::make(d_nbins);
      d_strs2vec = blocks::streams_to_vector::make(sizeof(gr_complex), d_nbins);
      d_goe.clear();
      d_goe.reserve(d_nbins);
      d_freqs = blocks::vector_source_f::make(freqs, true, d_nbins);

      /* Connections */
      //input
      connect(self(), 0, d_str2vec, 0);
      connect(d_str2vec, 0, d_vec2str, 0);
      for(int i = 0; i < d_nbins; i++) {
        d_goe.push_back(
            fft::goertzel_fc::make(d_samp_rate, d_window_size, freqs.at(i))
        );
        connect(d_vec2str, 0, d_goe.at(i), 0);
        connect(d_goe.at(i), 0, d_strs2vec, i);

        if (i >= 1) {
            d_goe.at(i)->set_tag_propagation_policy(gr::block::tag_propagation_policy_t::TPP_DONT);
        }

      }
      connect(d_strs2vec, 0, d_com2magphase, 0);

      //output
      connect(d_com2magphase, 0, self(), 0);
      connect(d_com2magphase, 1, self(), 1);
      connect(d_freqs, 0, self(), 2);
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
      if( !d_range_fixed ) {
        d_fq_lo = fq_low;
        d_fq_hi = fq_hi;
        std::vector<float> freqs(d_nbins);
        double fq_step = (d_fq_hi - d_fq_lo) / (1.0 * d_nbins);

        for(int i = 0; i < d_nbins; i++) {
          double ith_freq = d_fq_lo + (i * fq_step);
          d_goe.at(i)->set_freq(ith_freq);
          freqs.at(i) = ith_freq;
        }
        d_freqs->set_data(freqs);
      }
    }




  } /* namespace digitizers */
} /* namespace gr */

