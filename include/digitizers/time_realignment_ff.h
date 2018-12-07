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

    /*! \brief Write precise WR-Timestamp into trigger tag
     * This block assumes that the WR-Tag always is received before the related WR-Tag
     * Incoming Trigger-Tag will get wr-stamp of oldest incoming wr-event
     * If there is no WR-Event to process, an error will be flagged in the status of the Trigger-Tag
     * If the time difference between trigger and WR-event is to big, an error will be flagged in the status of the Trigger-Tag
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
       * \param user_delay user defined delay in seconds
       */
      static sptr make(float user_delay=0.0f, float triggerstamp_matching_tolerance=0.03f);

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

      /*!
       * \brief Sets maximum triggerstamp_matching_tolerance between WR-Tag(converted to UTC) and incoming Trigger Stamp at the scope (UTC)
       */
      virtual void set_triggerstamp_matching_tolerance(float triggerstamp_matching_tolerance) = 0;

      virtual float get_triggerstamp_matching_tolerance() = 0;

      /*!
       * \brief Add information about the timing event.
       *
       * \param event_id An arbitrary event descriptor
       * \param wr_trigger_stamp event timestamp, TAI ns
       * \param wr_trigger_stamp_utc event timestamp UTC
       */
      virtual void add_timing_event(const std::string &event_id, int64_t wr_trigger_stamp, int64_t wr_trigger_stamp_utc) = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_TIME_REALIGNMENT_FF_H */

