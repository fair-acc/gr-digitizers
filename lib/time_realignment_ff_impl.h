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

      // stored in ns to allow fast comparioson
      int64_t d_triggerstamp_matching_tolerance_ns;

      // 'received' timestamp and related WR-Event

      std::vector< wr_event_t > d_pending_events;

     public:
      time_realignment_ff_impl(float user_delay, float triggerstamp_matching_tolerance);
      ~time_realignment_ff_impl();

      void set_user_delay(float user_delay) override;
      float get_user_delay() override;
      void set_triggerstamp_matching_tolerance(float triggerstamp_matching_tolerance);
      float get_triggerstamp_matching_tolerance();
      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items) override;

      bool start() override;

     private:

      int64_t get_timestamp_utc_ns();

      void push_wr_tag(const wr_event_t &event);

      void fill_wr_stamp(trigger_t &trigger_tag_data);

      int64_t get_user_delay_ns() const;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_TIME_REALIGNMENT_FF_IMPL_H */

