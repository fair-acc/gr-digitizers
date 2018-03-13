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


#ifndef INCLUDED_DIGITIZERS_BLOCK_SPECTRAL_PEAKS_H
#define INCLUDED_DIGITIZERS_BLOCK_SPECTRAL_PEAKS_H

#include <digitizers/api.h>
#include <gnuradio/hier_block2.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief This block recieves a stream of data, does a frequency transform,
     * and finds a maximal value of the filtered  response.
     *
     *\verbatim
     * f(x)  -FFT->  f(t)           -tag_attach->   f(t/2)
     *                 L-med_avg->  g(t/2)-^
     *\endverbatim
     *
     * Attaches tag to the actual peak, that has to be in the proximity of the
     * maximal filtered response between low_freq and up_freq.
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API block_spectral_peaks : virtual public gr::hier_block2
    {
     public:
      typedef boost::shared_ptr<block_spectral_peaks> sptr;

      /*!
       * \brief Creates a hier block that attaches tags to spectral peaks
       * of the input vector with the defined parameters.
       *
       * \param samp_rate The sample rate of the signal acquisition
       * \param fft_window Size of FFT window(preferably 2^n)
       * \param low_freq lower boundary of the search space for peaks
       * \param up_freq upper boundary of the search space for peaks
       * \param n_med median filter window size
       * \param n_avg averaging filter window size
       * \param n_prox size of proximity window
       */
      static sptr make(double samp_rate,
          int fft_window,
          double low_freq,
          double up_freq,
          int n_med,
          int n_avg,
          int n_prox);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_BLOCK_SPECTRAL_PEAKS_H */

