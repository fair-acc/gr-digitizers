/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_TIME_REALIGNMENT_FF_IMPL_H
#define INCLUDED_DIGITIZERS_TIME_REALIGNMENT_FF_IMPL_H

#include <digitizers/time_realignment_ff.h>
#include <digitizers/tags.h>

#include <mutex>

#include "utils.h"

namespace gr {
  namespace digitizers {


    class time_realignment_ff_impl : public time_realignment_ff
    {
     private:
      float d_user_delay;

      // stored in ns to allow fast comparioson
      int64_t d_triggerstamp_matching_tolerance_ns;

      // maximum time incoming triggers and samples will be buffered before forwarding them without realligment of the trigger tags
      int64_t d_max_buffer_time_ns;
      int64_t d_not_found_stamp_utc;

      // cyclic buffer of white rabbit events
      std::vector< wr_event_t > d_wr_events;
      size_t d_wr_events_size;

      std::vector< wr_event_t >::iterator d_wr_events_write_iter;
      std::vector< wr_event_t >::iterator d_wr_events_read_iter;

     public:
      time_realignment_ff_impl(const std::string id, float user_delay, float triggerstamp_matching_tolerance, float max_buffer_time);
      ~time_realignment_ff_impl();

      void set_user_delay(float user_delay) override;
      float get_user_delay() override;
      void set_triggerstamp_matching_tolerance(float triggerstamp_matching_tolerance);
      float get_triggerstamp_matching_tolerance();

      void set_max_buffer_time(float max_buffer_time);
      float get_max_buffer_time();

      //void forecast(int noutput_items, gr_vector_int &ninput_items_required) override;

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items) override;

      bool add_timing_event(const std::string &event_id, int64_t wr_trigger_stamp, int64_t wr_trigger_stamp_utc) override;

     private:

      void check_pending_event_size();

      void update_pending_events();

      bool fill_wr_stamp(trigger_t &trigger_tag_data);

      int64_t get_user_delay_ns() const;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_TIME_REALIGNMENT_FF_IMPL_H */

