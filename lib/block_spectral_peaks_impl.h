/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
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
          std::vector<int> in_sig,
          int n_med,
          int n_avg,
          int n_prox);

      ~block_spectral_peaks_impl();

      // Where all the action really happens
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_BLOCK_SPECTRAL_PEAKS_IMPL_H */

