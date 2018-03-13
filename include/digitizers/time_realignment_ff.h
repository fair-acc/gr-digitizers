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


#ifndef INCLUDED_DIGITIZERS_TIME_REALIGNMENT_FF_H
#define INCLUDED_DIGITIZERS_TIME_REALIGNMENT_FF_H

#include <digitizers/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief Synchronizes samples to the timing and applies user defined delays.
     *
     * Time synchronization is achieved by updating the following two fields of the acq_info tag:
     * a) timestamp, and
     * b) trigger_timestamp
     *
     * The time realignment block expects to receives a precise timestamp (i.e. from FESA class)
     * whenever a trigger tag is detected (acq_info tag with triggered_data field set to true).
     * Trigger tag is updated with this received timestamp, which is also refereed to as a trigger
     * timestamp (trigger_timestamp field). If needed this block also waits for a realignment delay
     * which is summed with user defined delays. This delay is used when extracting triggered data
     * i.e. by the B.2 Demux block.
     *
     * At the same time a precise timestamp is calculated, based on the trigger timestamp and user
     * defined delays, for the first sample in the collection (timestamp field) like this:
     * <p>
     * timestamp = trigger_timestamp - (<pre-trigger samples> * timebase) + user_delay
     * <p>
     * Note this block only modifies the two timestamps of the acq_info tag. No additional tags are
     * generated.
     *
     * Note also, trigger timestamp is not modified, e.g. by applying a user defined delay, because
     * it is used to find an associated multiplexing context on the FESA side and by B.2 Demux block
     * to extract triggered data.
     *
     * Once a first timing event is received other, non-triggered acq_info tags are adjusted as well
     * by using sample counting.
     *
     * Summing of user and realignment delays
     *
     * actual_delay += realignment_delay + user_delay
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
       * \param user_delay user defined offset in seconds
       * \param timing_available indicates weather timing is available or not
       * \param ingnore_realignment do not wait for realignment delay
       */
      static sptr make(float user_delay=0.0, bool timing_available=false, bool ingnore_realignment=false);

      /*!
       * \brief Add information about the timing event.
       *
       * This allows the module to add meaningful information (i.e. precise time) to tags,
       * or more precise to the acq_info tags passing trough this block.
       *
       * Refer to digitizers/tags.h header file for the definition of the acq_info tag.
       *
       * Note time synchronization acq_info tags are dropped.
       *
       * \param event_id An arbitrary event descriptor
       * \param event_timestamp event timestamp, UTC ns
       * \param beam_in_distance delay in ns since last actual beam-in event, -1 if delay is unknown
       * \param time_sync_only indicates weather the event should be used for time synchronization only
       * \param realignment_required should wait for realignment event
       */
      virtual void add_timing_event(const std::string &event_id, int64_t event_timestamp, int64_t beam_in_timestamp,
              bool time_sync_only=true, bool realignment_required=false) = 0;

      virtual void add_realignment_event(int64_t actual_event_timestamp, int64_t beam_in_timestamp) = 0;

      /*!
       * \brief Removes all elements from the timing event queue.
       */
      virtual void clear_timing_event_queue() = 0;

    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_TIME_REALIGNMENT_FF_H */

