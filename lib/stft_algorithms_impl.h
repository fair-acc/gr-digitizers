/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_STFT_ALGORITHMS_IMPL_H
#define INCLUDED_DIGITIZERS_STFT_ALGORITHMS_IMPL_H

#include <digitizers/stft_algorithms.h>
#include <digitizers/stream_to_vector_overlay_ff.h>
#include <gnuradio/blocks/vector_to_stream.h>
#include <gnuradio/blocks/stream_to_vector.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/fft/fft_vfc.h>
#include <gnuradio/fft/goertzel_fc.h>
#include <gnuradio/blocks/vector_to_stream.h>
#include <gnuradio/blocks/keep_m_in_n.h>
#include <gnuradio/blocks/complex_to_magphase.h>
#include <gnuradio/blocks/streams_to_vector.h>
#include <gnuradio/filter/firdes.h>

namespace gr {
  namespace digitizers {

    //alg_id = 0
    class fft_impl : public stft_algorithms
    {
    private:

      filter::firdes::win_type d_wintype;
      stream_to_vector_overlay_ff::sptr d_str2vec;
      blocks::complex_to_magphase::sptr d_com2magphase;
      fft::fft_vfc::sptr d_fft;
      blocks::vector_to_stream::sptr d_vec2str;
      blocks::keep_m_in_n::sptr d_keep_m;
      blocks::stream_to_vector::sptr d_half_str2vec;
      blocks::vector_source_f::sptr d_freqs;
      double d_samp_rate;
      int d_window_size;

    public:

      fft_impl(double samp_rate,
          double delta_t,
          int window_size,
          filter::firdes::win_type wintype,
          double fq_low,
          double fq_hi,
          int nbins);

      ~fft_impl();

      void set_samp_rate(double samp_rate);

      void set_window_size(int window_size);

      void set_freqs(double fq_low, double fq_hi);

      void set_window_type(int wintype);

    };

    //alg_id = 1, 2
    class goertzel_impl : public stft_algorithms
    {
    private:
      stream_to_vector_overlay_ff::sptr d_str2vec;
      blocks::vector_to_stream::sptr d_vec2str;
      blocks::complex_to_magphase::sptr d_com2magphase;
      std::vector<fft::goertzel_fc::sptr> d_goe;
      blocks::streams_to_vector::sptr d_strs2vec;
      blocks::vector_source_f::sptr d_freqs;
      double d_samp_rate;
      int d_window_size;
      double d_fq_lo;
      double d_fq_hi;
      int d_nbins;
      bool d_range_fixed;

    public:

      goertzel_impl(double samp_rate,
          double delta_t,
          int window_size,
          double fq_low,
          double fq_hi,
          int nbins,
          bool range_fixed);

      ~goertzel_impl();

      void set_samp_rate(double samp_rate);

      void set_window_size(int window_size);

      void set_freqs(double fq_low, double fq_hi);

      void set_window_type(int wintype);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_STFT_ALGORITHMS_IMPL_H */

