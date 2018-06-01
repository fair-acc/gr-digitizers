/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
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
      int d_winsize;
      int d_nbins;
      std::vector< float >  d_window_function;
      
      void goertzel(const float* data, const long data_len, float Ts, float frequency, int filter_size, float &real, float &imag);
      void dft(const float* data, const long data_len, float Ts, float frequency, float& real, float& imag);

     public:

      stft_goertzl_dynamic_impl(double samp_rate, int winsize, int nbins, std::vector<int> in_sig);

      ~stft_goertzl_dynamic_impl();

      int work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items);

      void set_samp_rate(double samp_rate);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_STFT_GOERTZL_DYNAMIC_IMPL_H */

