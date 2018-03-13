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

#ifndef INCLUDED_DIGITIZERS_TIME_REALIGNMENT_FF_IMPL_H
#define INCLUDED_DIGITIZERS_TIME_REALIGNMENT_FF_IMPL_H

#include <digitizers/time_realignment_ff.h>
#include <boost/optional.hpp>

#include "utils.h"

namespace gr {
  namespace digitizers {

    struct timing_event_t
    {
      std::string event_id;      // Any string
      int64_t timestamp;         // UTC, nanoseconds since UNIX epoch
      int64_t beam_in_timestamp;  // UTC, nanoseconds since UNIX epoch
      bool time_sync_only;       // for time synchronization only
      bool realignment_required;
    };

    struct realignment_event_t
    {
      int64_t actual_timestamp;   // UTC, nanoseconds since UNIX epoch
      int64_t beam_in_timestamp;  // UTC, nanoseconds since UNIX epoch
    };

    class time_realignment_ff_impl : public time_realignment_ff
    {
     public:
      time_realignment_ff_impl(float user_delay, bool timing_available, bool ingnore_realignment);
      ~time_realignment_ff_impl();

      void add_timing_event(const std::string &event_id, int64_t event_timestamp, int64_t beam_in_timestamp,
              bool time_sync_only=true, bool realignment_required=false) override;

      void add_realignment_event(int64_t actual_event_timestamp,
              int64_t beam_in_timestamp) override;

      void clear_timing_event_queue() override;

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items) override;

      bool start() override;

     private:
      boost::optional<timing_event_t> get_timing_event();
      boost::optional<realignment_event_t> get_realignment_event(int64_t timestamp);

      int64_t get_timestamp(uint64_t offset, double timebase);

     private:
      float d_user_delay;
      // precise timestamp (UTC ns) without user delay applied
      uint64_t d_timestamp_offset;
      int64_t d_timestamp;
      int64_t d_beam_in_timestamp;
      concurrent_queue<timing_event_t> d_timing_event_queue;
      concurrent_queue<realignment_event_t> d_realignment_queue;
      bool d_timing_available;
      bool d_ignore_realignment;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_TIME_REALIGNMENT_FF_IMPL_H */

