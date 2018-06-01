/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_TIME_REALIGNMENT_FF_H
#define INCLUDED_DIGITIZERS_TIME_REALIGNMENT_FF_H

#include <digitizers/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief Synchronizes samples to the timing and appends user defined delay.
     *
     * Time synchronization is achieved by updating the following two fields of the acq_info tag:
     * a) timestamp (abs timestamp of the first sample in the collection), and
     * b) last beam in timestamp
     *
     * The time realignment block expects to receives a precise timestamp (i.e. from WR Receiver block)
     * whenever a trigger tag is detected (trigger tag). It should be noted that the time realignment
     * block does not wait to receive WR event tag but it simply passes all the data trough. This means
     * that the very first acq_info tag is not updated with the precise timestamp, but all others are.
     *
     * When calculating the timestamp of the very first sample in the collection, user suppled delay
     * is also accounted for according to the following formula:
     * <p>
     * timestamp = timestamp_of_the_first_sample] + user_delay
     * <p>
     *
     * Note this block only modifies the two timestamps of the acq_info tag. No additional tags are
     * generated. This block passes trough all the tags with 1:1 like policy (port 0 only).
     *
     * User delay is added to the acq_info tag like this:
     *
     * actual_delay += user_delay
     * user_delay += user_delay
     *
     * \ingroup digitizers
     */
    class DIGITIZERS_API time_realignment_ff : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<time_realignment_ff> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::time_realignment_ff.
       *
       * \param samp_rate sample rate in Hz
       * \param user_delay user defined delay in seconds
       */
      static sptr make(float samp_rate, float user_delay=0.0);

      /*!
       * \brief Sets user delay.
       *
       * This setting is not used to correct actual signal in any way. This delay is added to the
       * acq_info timestamp passed trough this block. See general description above.
       *
       * \param user_delay user defined delay in seconds
       */
      virtual void set_user_delay(float user_delay) = 0;

      /*!
       * \brief Gets user delay.
       */
      virtual float get_user_delay() = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_TIME_REALIGNMENT_FF_H */

