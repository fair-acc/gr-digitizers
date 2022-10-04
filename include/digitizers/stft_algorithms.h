/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_STFT_ALGORITHMS_H
#define INCLUDED_DIGITIZERS_STFT_ALGORITHMS_H

#include <digitizers/api.h>
#include <gnuradio/hier_block2.h>
#include <gnuradio/filter/firdes.h>


namespace gr {
  namespace digitizers {

      /*!
       * \brief STFT mode
       * \ingroup digitizers
       */
      enum DIGITIZERS_API stft_algorithm_id_t
      {
        FFT          = 0,
        GOERTZEL     = 1,
		DFT          = 2
      };

    /*!
     * \brief Calculate the stft of the signal. This is a hier block
     * wired as it was described in the proposed wiring diagram.
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API stft_algorithms : virtual public gr::hier_block2
    {
     public:
      typedef std::shared_ptr<stft_algorithms> sptr;

      /*!
       * \brief Make stft block instance
       *
       * \param samp_rate sampling rate of the signal.
       * \param delta_t The time in seconds between each signal analysis
       * \param window_size size of the stft window
       * \param wintype window type to remove noise in the freq. response.
       * \param alg_id (fft = 0, goertzel = 1, dft = 2)
       * \param fq_low lower frequency for the goertzel based f-response
       * \param fq_hi upper frequency for the goertzel based f-response
       * \param nbins number of bins for the goertzel basded f-response
       */
      static sptr make(double samp_rate, double delta_t, int window_size, int wintype, stft_algorithm_id_t alg_id, double fq_low, double fq_hi, int nbins);

      /**
       * \brief Set a new sample rate.
       *
       * Interrupts the block, and fixes all the underlaying blocks.
       *
       * \param samp_rate The desired new sample rate
       */
      virtual void set_samp_rate(double samp_rate) = 0;

      /**
       * \brief Set a new window type.
       *
       * One of enum in firdes::window
       * Interrupts the block, and fixes all the underlaying blocks.
       *
       * \param win_type the enum value you wish to apply
       */
      virtual void set_window_type(int win_type) = 0;

      /**
       * \brief Set up frequency window(applicable when alg_id = 1(Goertzel)
       *
       * Interrupts the block, and fixes all the underlaying blocks.
       *
       * \param fq_low The lower frequency of the desired window
       * \param fq_hi The upper frequency of the desired window.
       */
      virtual void set_freqs(double fq_low, double fq_hi) = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_STFT_ALGORITHMS_H */

