/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_BLOCK_AMPLITUDE_AND_PHASE_H
#define INCLUDED_DIGITIZERS_BLOCK_AMPLITUDE_AND_PHASE_H

#include <digitizers/api.h>
#include <gnuradio/hier_block2.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief Estimates signals frequency and amplitude, given a reference
     * signal. Amplitude is calculated as a coefficient of(signal/reference),
     * while the phase shift is in degrees between the signal and rhe
     * reference signal. The estimates are passed as seperate outputs for the
     * different estimations for each individual sample.
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API block_amplitude_and_phase : virtual public gr::hier_block2
    {
     public:
      typedef std::shared_ptr<block_amplitude_and_phase> sptr;

      /*!
       * \brief Creates an amplitude phase and frequency estimator.
       *
       * \param samp_rate The sample rate of the signal acquisition
       * \param delay The delay of the output
       * \param decim the decimating factor
       * \param gain multiplier of the signal
       * \param up_freq upper frequency of the window
       * \param tr_width transition width from full response to zero.
       * \param hilbert_window window selection.
       */
      static sptr make(double samp_rate,
        double delay,
        int decim,
        double gain,
        double up_freq,
        double tr_width,
        int hilbert_window);

      /*!
       * \brief Updates the parameters of the amplitude phase and frequency
       * estimator.
       *
       * \param delay The delay of the output
       * \param gain multiplier of the signal
       * \param up_freq upper frequency of the window
       * \param tr_width transition width from full response to zero.
       */
      virtual void update_design(double delay,
        double gain,
        double up_freq,
        double tr_width) = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_BLOCK_AMPLITUDE_AND_PHASE_H */

