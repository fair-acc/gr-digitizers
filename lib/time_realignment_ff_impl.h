/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_TIME_REALIGNMENT_FF_IMPL_H
#define INCLUDED_DIGITIZERS_TIME_REALIGNMENT_FF_IMPL_H

#include <digitizers/time_realignment_ff.h>
#include <digitizers/tags.h>

#include <deque>

#include "utils.h"

namespace gr {
  namespace digitizers {


    class time_realignment_ff_impl : public time_realignment_ff
    {
     private:
      float d_samp_rate;
      float d_user_delay;

      // Absolute sample count where the last timing event appeared
      uint64_t d_last_trigger_offset;
      wr_event_t d_last_wr_event;

      // Incoming triggers & events. Queues are used because ordering of triggers and events is not
      // guaranteed, meaning a tag representing White Rabbit event might come down the stream before
      // the trigger tag.
      std::deque<uint64_t> d_pending_triggers;
      std::deque<wr_event_t> d_pending_events;

     public:
      time_realignment_ff_impl(float samp_rate, float user_delay);
      ~time_realignment_ff_impl();

      void set_user_delay(float user_delay) override;
      float get_user_delay() override;

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items) override;

      bool start() override;

     private:
      void push_and_update_last(const wr_event_t &event);

      void push_and_update_last(uint64_t trigger);

      inline void update_last_if_needed();

      // Calculates timestamp of a given sample. If timing is not available, that is
      // d_last_wr_event.timestamp == -1, a fallback timestamp is returned
      int64_t calculate_timestamp(uint64_t offset, int64_t fallback_timestamp) const;

      int64_t get_user_delay_ns() const;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_TIME_REALIGNMENT_FF_IMPL_H */

