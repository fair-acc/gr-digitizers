/* -*- c++ -*- */
/*
 * Copyright 2021 fair.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DIGITIZERS_39_MAINS_FREQUENCY_CALC_H
#define INCLUDED_DIGITIZERS_39_MAINS_FREQUENCY_CALC_H

#include <digitizers_39/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace digitizers_39 {

    /*!
     * \brief <+description of block+>
     * \ingroup digitizers_39
     *
     */
    class DIGITIZERS_39_API mains_frequency_calc : virtual public gr::sync_block
    {
     public:
      typedef std::shared_ptr<mains_frequency_calc> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers_39::mains_frequency_calc.
       *
       * To avoid accidental use of raw pointers, digitizers_39::mains_frequency_calc's
       * constructor is in a private implementation
       * class. digitizers_39::mains_frequency_calc::make is the public interface for
       * creating new instances.
       */
      static sptr make(double expected_sample_rate, float low_threshold, float high_threshold);
    };

  } // namespace digitizers_39
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_39_MAINS_FREQUENCY_CALC_H */

