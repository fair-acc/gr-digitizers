/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_STFT_GOERTZL_DYNAMIC_H
#define INCLUDED_DIGITIZERS_STFT_GOERTZL_DYNAMIC_H

#include <digitizers/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief The block returns the response from lower frequency to the upper frequency.
     * Both bounds are read from inputs, with the correspondent vector of samples. The vector
     * of samples needs to be of length winsize. The block creates nbins different frequency
     * repsonses, that are linearly interpolated between the lower and upper bound. The
     * boundries are considered as part of the window, except in the case where there is only
     * one bin, in which case the analysis frequency is the average of fMin and fMax,
     * ie. (fMin+fMax)/2. The block returns magnitudes, phase shifts, and frequencies of the
     * analysis, all of length nbins.
     *
     * ^
     * |          < - - - - - - - >
     * |          |       |       |
     * |          b0      b1      b2
     * L----------*---------------*------------*>
     * 0          fMin            fMax         f_samp*
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API stft_goertzl_dynamic : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<stft_goertzl_dynamic> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::stft_goertzl_dynamic.
       *
       * \param samp_rate sampling rate of the signal.
       * \param winsize size of the stft window
       * \param nbins number of bins for the goertzel basded f-response
       */
      static sptr make(double samp_rate, int winsize, int nbins);

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

#endif /* INCLUDED_DIGITIZERS_STFT_GOERTZL_DYNAMIC_H */

