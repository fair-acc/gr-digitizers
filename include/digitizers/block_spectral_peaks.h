/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_BLOCK_SPECTRAL_PEAKS_H
#define INCLUDED_DIGITIZERS_BLOCK_SPECTRAL_PEAKS_H

#include <digitizers/api.h>
#include <gnuradio/hier_block2.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief This block receives a vector that corresponds to the frequency repsonse of the signal,
     * the lower frequency, and the upper frequency where a valid peak should be located. It then
     * proceeds to filter and smoothen the actual response so the result is more stable. It finds
     * a peak in the filtered response, and then finds the actual peak in the proximity of the filtered
     * peak.when the peak is found, it is posted as a result. In addition the standard deviation is
     * approximated for the peak, by means of the FWHM approximation method. The filtered response,
     * ofund peak frequency and the calculated stdev are then posted for each batch of input values.
     *
     *\verbatim
     *
     * Wiring diagram:
     *
     *                   +-----------------+       +------------------+
     *  f RESPONSE       |                 |       |                  |   FILTERED f REPSONSE
     *  +============+=> |  MEDIAN FILTER  +=====> |  AVERAGE FILTER  +====+================>
     *               ||  |                 |       |                  |   ||
     *  f_min        ||  +-----------------+       +------------------+   ||           PEAK f
     *  +-------+    ||  +=================================================+   +------------>
     *          |    ||  ||                                                    |
     *  f_max   |    ||  ||      +---------------+                             | PEAK f STDEV
     *  +---+   |    ||  +=====> |               +-----------------------------+   +-------->
     *      |   |    +=========> |  PEAK FINDER  |                                 |
     *      |   +--------------> |               +---------------------------------+
     *      +------------------> |               |
     *                           +---------------+
     *
     *
     *
     * Logic representation:
     *
     *  f_resp ^
     *         |                   /\
     *         |                  /  \
     *         |                 |    |
     *         |                 |    \
     *         |---\    ________/      |
     *         |    |  /               |
     *         |    |  |               \_____/\
     *         |    \__/                       \__________/\__
     *         +----------x-------- ^--------x----------------x>
     *         0          f_min     |        f_max            acq_f/2
     *                              |
     *                              |PEAK f
     *                           <--|--> PEAK f STDEV
     *\endverbatim
     *
     *
     *
     */
    class DIGITIZERS_API block_spectral_peaks : virtual public gr::hier_block2
    {
     public:
      typedef std::shared_ptr<block_spectral_peaks> sptr;

      /*!
       * \brief Creates a hier block that attaches tags to spectral peaks
       * of the input vector with the defined parameters.
       *
       * \param samp_rate The sample rate of the signal acquisition
       * \param fft_window Size of FFT window(preferably 2^n)
       * \param n_med median filter window size
       * \param n_avg averaging filter window size
       * \param n_prox size of proximity window
       */
      static sptr make(double samp_rate,
          int fft_window,
          int n_med,
          int n_avg,
          int n_prox);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_BLOCK_SPECTRAL_PEAKS_H */

