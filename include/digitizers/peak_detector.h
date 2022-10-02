/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_PEAK_DETECTOR_H
#define INCLUDED_DIGITIZERS_PEAK_DETECTOR_H

#include <digitizers/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief Detects peaks of the filtered input and attaches a tag to the
     * corresponding maximal value of the actual signal in the proximity of
     * the peak found in the filtered signal. The block expects the full
     * double-spectrum as the FFT calculates ir on the actual input,
     * and just the first half on the spectrum on the filtered input.
     * This is because the filter block that calculates this averaged
     * median filtered signal is time-complex and the information on the
     * upper half of the spectrum is redundant.
     *
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API peak_detector : virtual public gr::block
    {
     public:
      typedef std::shared_ptr<peak_detector> sptr;

      /*!
       * \brief Creates a peak detector block with the given parameters.
       *
       * \param samp_rate The sample rate of the signal acquisition.
       * \param vec_len size of the input vector(actual input). The size of filtered input is (vec_len/2) for efficiency reasons.
       * \param proximity the proximity window for actual peak detection.
       */
      static sptr make(double samp_rate, int vec_len, int proximity);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_PEAK_DETECTOR_H */

