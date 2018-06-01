/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
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
      stream_to_vector_overlay_ff::sptr d_str2vec_sig;
      stream_to_vector_overlay_ff::sptr d_str2vec_min;
      stream_to_vector_overlay_ff::sptr d_str2vec_max;

     public:
      stft_goertzl_dynamic_decimated_impl(double samp_rate, double delta_t, int window_size, int nbins, int bounds_decimation);

      ~stft_goertzl_dynamic_decimated_impl();

      void set_samp_rate(double samp_rate);

    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_STFT_GOERTZL_DYNAMIC_DECIMATED_IMPL_H */

