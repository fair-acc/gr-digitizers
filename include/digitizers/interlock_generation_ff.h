/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_INTERLOCK_GENERATION_FF_H
#define INCLUDED_DIGITIZERS_INTERLOCK_GENERATION_FF_H

#include <digitizers/api.h>
#include <gnuradio/sync_block.h>
#include <digitizers/sink_common.h>

namespace gr {
  namespace digitizers {

    /*!
     * Callback specifier. User must implement this type of function, and register it with
     * the block, via set_interlock_callback, each time an interlock is issued.
     */
    typedef void (*interlock_cb_t)(int64_t timestamp, void *userdata);

    /*!
     * \brief Issues interlock if the signal is out of bounds. Bounds are specified by using
     * two input signals, namely min and max.
     *
     * All three input signals should have the same sampling rate.
     *
     * Note this block is not aware about the timing and it simply compares the input signal by
     * using the following formula:
     * (( y(t)≤ ymin(t )) ∨ (y(t) ≥ ymax(t))) {... issue interlock ...}
     *
     * Interlock detection can be disabled by driving the two reference signals higher (or lower
     * in case of min) than the maximum expected value. E.g. if max_max argument is specified to
     * be 10 and the max input signal rises above 10 then the upper bounds checking is disable,
     * namely the following part: (y(t) ≥ ymax(t)).
     *
     * \ingroup digitizers
     */
    class DIGITIZERS_API interlock_generation_ff : virtual public gr::sync_block
    {
     public:
      typedef std::shared_ptr<interlock_generation_ff> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::stft_goertzl_dynamic.
       *
       * \param max_min lower bound of the min interlock boundary
       * \param max_max upper bound of the max interlock boundary.
       */
      static sptr make(float max_min, float max_max);

      /*!
       * \brief Register a callable, called for each detected interlock.
       *
       * \param callback user callback
       * \param ptr a void pointer that is passed back to the gr::digitizers::interlock_cb_t function
       */
      virtual void set_callback(interlock_cb_t callback, void *ptr) = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_INTERLOCK_GENERATION_FF_H */

