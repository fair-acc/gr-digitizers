/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_STFT_GOERTZL_DYNAMIC_DECIMATED_H
#define INCLUDED_DIGITIZERS_STFT_GOERTZL_DYNAMIC_DECIMATED_H

#include <digitizers/api.h>
#include <gnuradio/hier_block2.h>

namespace gr {
  namespace digitizers {

  /*!
       * \brief This block is a hier block, that decimates, analyzes and returns
       * the frequency response(magnitudes and phases), and the frequencies of the analysis.
       * The block expects the bounds decimated, but it still decimates them to the corresponding delta t.
       * The block returns the responses from lower frequency to the upper frequency.
       *
       * The block samples the signal every delta_t seconds, and calculates the frequency response for it.
       * \ingroup digitizers
       *
       */
    class DIGITIZERS_API stft_goertzl_dynamic_decimated : virtual public gr::hier_block2
    {
     public:
      typedef std::shared_ptr<stft_goertzl_dynamic_decimated> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::stft_goertzl_dynamic.
       *
       * \param samp_rate sampling rate of the signal.
       * \param delta_t time between each analysis of signal
       * \param window_size size of the stft window
       * \param nbins number of bins for the goertzel basded f-response
       * \param bounds_decimation pre decimation factor of boundary inputs
       */
      static sptr make(double samp_rate,double delta_t, int window_size, int nbins, int bounds_decimation = 1);

      /**
       * \brief Set a new sample rate.
       *
       * Interrupts the block, and fixes all the blocks
       *
       * \param samp_rate The desired new sample rate
       */
      virtual void set_samp_rate(double samp_rate) = 0;

    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_STFT_GOERTZL_DYNAMIC_DECIMATED_H */

