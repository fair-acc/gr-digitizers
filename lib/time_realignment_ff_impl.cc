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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include <digitizers/tags.h>
#include "time_realignment_ff_impl.h"

namespace gr {
  namespace digitizers {

    time_realignment_ff::sptr
    time_realignment_ff::make(float user_delay, bool timing_available, bool ingnore_realignment)
    {
      return gnuradio::get_initial_sptr
        (new time_realignment_ff_impl(user_delay, timing_available, ingnore_realignment));
    }

    /*
     * The private constructor
     */
    time_realignment_ff_impl::time_realignment_ff_impl(float user_delay, bool timing_available, bool ignore_realignment)
      : gr::sync_block("time_realignment_ff",
              gr::io_signature::make(1, 2, sizeof(float)),
              gr::io_signature::make(1, 2, sizeof(float))),
       d_user_delay(user_delay),
       d_timestamp_offset(0),
       d_timestamp(-1),
       d_beam_in_timestamp(-1),
       d_timing_available(timing_available),
       d_ignore_realignment(ignore_realignment)
    {
      set_tag_propagation_policy(tag_propagation_policy_t::TPP_DONT);
    }

    /*
     * Our virtual destructor.
     */
    time_realignment_ff_impl::~time_realignment_ff_impl()
    {
    }

    int
    time_realignment_ff_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      assert(input_items.size() == output_items.size());

      // Just copy over all the data
      memcpy(output_items.at(0), input_items.at(0), noutput_items * sizeof(float));
      if (input_items.size() > 1) {
        memcpy(output_items.at(1), input_items.at(1), noutput_items * sizeof(float));
      }

      const uint64_t samp0_count = nitems_read(0);

      // Get all the tags, for performance reason a member variable is used
      std::vector<gr::tag_t> tags;
      get_tags_in_range(tags, 0, samp0_count, samp0_count + noutput_items);

      for (auto tag: tags) {
        if (tag.key == pmt::string_to_symbol("acq_info")) {

          auto acq_info = decode_acq_info_tag(tag);

          if (d_timing_available && acq_info.triggered_data) {
            auto event = get_timing_event();
            if (event) {
              // time synchronization
              d_timestamp = event->timestamp;
              d_timestamp_offset = acq_info.offset;
              d_beam_in_timestamp = event->beam_in_timestamp;

              // update tag
              acq_info.trigger_timestamp = event->timestamp;
              acq_info.last_beam_in_timestamp = event->beam_in_timestamp;

              // realignment
              if (!d_ignore_realignment && event->realignment_required) {
                auto realignment = get_realignment_event(event->timestamp);
                if (realignment) {
                  d_beam_in_timestamp = realignment->beam_in_timestamp;

                  // convert realignment delay to seconds
                  auto delta = realignment->actual_timestamp - event->timestamp;
                  auto rdelay = static_cast<double>(delta) / 1000000000.0;
                  acq_info.actual_delay += rdelay;
                }
                else {
                  acq_info.trigger_timestamp = -1;
                  GR_LOG_ERROR(d_logger, "timeout receiving realignment event");
                }
              }

              if (event->time_sync_only) {
                continue; // drop this tag
              }
            }
            else {
              acq_info.trigger_timestamp = -1;
              GR_LOG_ERROR(d_logger, "timeout receiving timing event");
            }
          }

          // get timestamp of the first sample
          if (d_timestamp != -1) {
              acq_info.timestamp = get_timestamp(acq_info.offset, acq_info.timebase);
          }

          // add user delay
          if (acq_info.timestamp != -1) {
            acq_info.timestamp -= static_cast<int64_t>(d_user_delay * 1000000000.0);
          }
          acq_info.user_delay += d_user_delay;
          acq_info.actual_delay += d_user_delay;

          if (d_beam_in_timestamp != -1) {
            acq_info.last_beam_in_timestamp = d_beam_in_timestamp;
          }

          tag = make_acq_info_tag(acq_info);
        }

        add_item_tag(0, tag);
      }

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

    bool
    time_realignment_ff_impl::start()
    {
      d_timestamp_offset = 0;
      d_timestamp = -1;

      return true;
    }

    int64_t
    time_realignment_ff_impl::get_timestamp(uint64_t offset, double timebase)
    {
      assert(d_timing_available);
      assert(d_timestamp > 0);
      assert(timebase > 0.0);

      if (offset >= d_timestamp_offset) {
        auto offset_delta = offset - d_timestamp_offset;
        auto time_delta = static_cast<double>(offset_delta) * (timebase * 1000000000.0);
        return d_timestamp + static_cast<int64_t>(time_delta);
      }
      else {
        auto offset_delta = d_timestamp_offset - offset;
        auto time_delta = static_cast<double>(offset_delta) * (timebase * 1000000000.0);
        return d_timestamp - static_cast<int64_t>(time_delta);
      }
    }

    void
    time_realignment_ff_impl::add_timing_event(const std::string &event_id, int64_t event_timestamp,
            int64_t beam_in_timestamp, bool time_sync_only, bool realignment_required)
    {
      if (!d_timing_available) {
        return;
      }

      if (d_timing_event_queue.size() > 1024) {
        GR_LOG_WARN(d_logger, "timing event queue full, consider disabling timing!");
      }

      timing_event_t event;
      event.event_id = event_id;
      event.timestamp = event_timestamp;
      event.beam_in_timestamp = beam_in_timestamp;
      event.time_sync_only = time_sync_only;
      event.realignment_required = realignment_required;

      d_timing_event_queue.push(event);
    }

    void
    time_realignment_ff_impl::clear_timing_event_queue()
    {
      d_timing_event_queue.clear();
    }

    void
    time_realignment_ff_impl::add_realignment_event(int64_t actual_event_timestamp, int64_t beam_in_timestamp)
    {
        if (!d_timing_available) {
          return;
        }

        if (d_realignment_queue.size() > 1024) {
          GR_LOG_WARN(d_logger, "realignment queue full, consider disabling timing!");
        }

        realignment_event_t event;
        event.actual_timestamp = actual_event_timestamp;
        event.beam_in_timestamp = beam_in_timestamp;

        d_realignment_queue.push(event);
    }

    boost::optional<timing_event_t>
    time_realignment_ff_impl::get_timing_event()
    {
      assert(d_timing_available);

      timing_event_t event;
      auto status = d_timing_event_queue.wait_and_pop(event, std::chrono::milliseconds(250));
      if (!status) {
        return boost::none;
      }
      return event;
    }

    boost::optional<realignment_event_t>
    time_realignment_ff_impl::get_realignment_event(int64_t timestamp)
    {
      assert(d_timing_available);

      while (true) {
        realignment_event_t realignment;
        auto retval = d_realignment_queue.wait_and_pop(realignment, std::chrono::milliseconds(250));
        if (!retval) {
          return boost::none;
        }

        // Ignore all delays with "actual beam-in" timestamp smaller than the trigger stamp
        if (realignment.actual_timestamp < timestamp) {
          continue;
        }

        return realignment;
      }
    }

  } /* namespace digitizers */
} /* namespace gr */

