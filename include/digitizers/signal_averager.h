/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_SIGNAL_AVERAGER_H
#define INCLUDED_DIGITIZERS_SIGNAL_AVERAGER_H

#include <digitizers/api.h>
#include <gnuradio/sync_decimator.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief Average samples in window and generate a single sample.
     *
     * The block decimates the data with the step of window size.
     *
     * Tags are attached to the correspondent decimated sample, timebase and acq_info tags
     * are fixed with the decimation factor and user provided delay. If more than one acq_info
     * tags falls into the same output offset then the tags get merged.
     *
     * It supports multiple input signals. Does the same on each input port.
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API signal_averager : virtual public gr::sync_decimator
    {
     public:
      typedef std::shared_ptr<signal_averager> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::signal_averager.
       *
       * \param num_inputs Number of input signals
       * \param window_size The decimation factor.
       * \param samp_rate sample rate on input
       */
      static sptr make(int num_inputs, int window_size, float samp_rate);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_SIGNAL_AVERAGER_H */

