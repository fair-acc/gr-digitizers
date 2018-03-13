/* -*- c++ -*- */
/* 
 * Copyright 2018 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
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
      typedef boost::shared_ptr<peak_detector> sptr;

      /*!
       * \brief Creates a peak detector block with the given parameters.
       *
       * \param samp_rate The sample rate of the signal acquisition
       * \param vec_len size of the input vector(actual input). The size of filtered input is (vec_len/2) for efficiency reasons.
       * \param start_bin lower bound of the detector in the filtered signal.
       * \param end_bin upper bound of the detector in the filtered signal.
       * \param proximity the proximity window for actual peak detection.
       */
      static sptr make(double samp_rate, int vec_len, int start_bin, int end_bin, int proximity);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_PEAK_DETECTOR_H */

