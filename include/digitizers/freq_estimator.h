/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_FREQ_ESTIMATOR_H
#define INCLUDED_DIGITIZERS_FREQ_ESTIMATOR_H

#include <digitizers/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief Estimates a base frequency of the signal by counting the number of samples between
     * events where signal goes through zero, and computing the time that took, given the sampling rate.
     * This is just a rough estimate of the frequency. To avoid false positives because of noisse and
     * other undesired components of the signal, the signal is passed through a running average of size
     * given by the user. The result itself is then also averaged with the same averaging window for a
     * more stable result.
     *
     * Meaning the estimate of the frequency is an inverse of the average of the last
     * n time intervals it took for the averaged signal(to remove noise) to pass through zero.
     *
     * IMPORTANT:
     * The specified window must be much smaller than the number of samples in one cycle of the signal!
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API freq_estimator : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<freq_estimator> sptr;

      /*!
       * \brief Create new instance
       *
       * \param samp_rate Frequency of data acquisition
       * \param signal_window_size Size of the signal averager window
       * \param averager_window_size Size of the frequency estimation averager window
       * \param decim factor
       */
      static sptr make(float samp_rate, int signal_window_size, int averager_window_size, int decim);

      /*!
       * \brief Update parameters for decimation
       *
       * \param decim decimation factor
       */
      virtual void update_design(int decim) = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_FREQ_ESTIMATOR_H */

