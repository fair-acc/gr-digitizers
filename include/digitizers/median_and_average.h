/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_MEDIAN_AND_AVERAGE_H
#define INCLUDED_DIGITIZERS_MEDIAN_AND_AVERAGE_H

#include <digitizers/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief This block filters the signal by first passing it into a median filter
     * that takes care of high signal spikes, and then averages the result to result
     * in a smooth estimation of the signals comonents. Operates with vectors, for
     * easier transmission of the full spectrum we're estimating at a given point.
     *
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API median_and_average : virtual public gr::block
    {
     public:
      typedef std::shared_ptr<median_and_average> sptr;

      /*!
       * \brief Creates a block that filters the input signal.
       *
       * \param vec_len length of the input vector
       * \param n_med size of the median filter window.
       * \param n_lp size of the averaging filter window.
       */
      static sptr make(int vec_len, int n_med, int n_lp);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_MEDIAN_AND_AVERAGE_H */

