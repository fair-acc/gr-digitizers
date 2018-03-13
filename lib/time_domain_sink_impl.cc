/* -*- c++ -*- */
/* 
 * Copyright 2017 <+YOU OR YOUR COMPANY+>.
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
#include <digitizers/status.h>
#include "time_domain_sink_impl.h"

#include <string>
#include <algorithm>
#include <cmath>

namespace gr {
  namespace digitizers {

    time_domain_sink::sptr
    time_domain_sink::make(std::string name, std::string unit, size_t buffer_size, time_sink_mode_t mode)
    {
      return gnuradio::get_initial_sptr
        (new time_domain_sink_impl(name, unit, buffer_size, mode));
    }

    /*
     * The private constructor
     */
    time_domain_sink_impl::time_domain_sink_impl(std::string name, std::string unit, size_t buffer_size, time_sink_mode_t mode)
      : gr::sync_block("time_domain_sink",
              gr::io_signature::make(1, 2, sizeof(float)),
              gr::io_signature::make(0, 2, sizeof(float))),
        d_sink_mode(mode),
        d_buffer_size(buffer_size),
        d_size(0),
        d_timestamp(-1),
        d_status(0),
        d_acq_info(),
        d_metadata(),
        d_callback(nullptr),
        d_user_data(nullptr),
        d_samples_left(0),
        d_collecting_samples(false),
        d_waiting_readout(false)
    {
      if (mode == TIME_SINK_MODE_SNAPSHOT) {
        d_buffer_size = buffer_size = 1;
      }

      d_buffer_values.resize(buffer_size),
      d_buffer_errors.resize(buffer_size),

      memset(&d_buffer_errors[0], 0, sizeof(float) * buffer_size);

      d_acq_info.timestamp = -1;
      d_acq_info.trigger_timestamp = -1;

      d_metadata.name = name;
      d_metadata.unit = unit;
    }

    /*
     * Our virtual destructor.
     */
    time_domain_sink_impl::~time_domain_sink_impl()
    {
    }

    int
    time_domain_sink_impl::work(int ninput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      if(d_sink_mode == time_sink_mode_t::TIME_SINK_MODE_TRIGGERED) {
        return work_fast_data(ninput_items, input_items, output_items);
      }
      else if (d_sink_mode == time_sink_mode_t::TIME_SINK_MODE_SNAPSHOT) {
        return work_snapshot_data(ninput_items, input_items, output_items);
      }
      else {
        return work_slow_data(ninput_items, input_items, output_items);
      }
    }

    int
    time_domain_sink_impl::work_slow_data(int ninput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      assert(d_sink_mode != time_sink_mode_t::TIME_SINK_MODE_TRIGGERED);

      {
        boost::mutex::scoped_lock lock(d_mutex);

        assert(d_size <= d_buffer_size);

        if (d_waiting_readout || (d_size == d_buffer_size)) {
          return 0;
        }

        if (d_size == 0) {
          // Starting fresh... Set status to the last acq_info status and calculate
          // the timestamp of the first sample.
          d_status = d_acq_info.status;

          if (d_acq_info.timestamp < 0) {
             d_timestamp = -1; // timestamp is invalid
          }
          else {
            auto delta = d_acq_info.timebase * (nitems_read(0) - d_acq_info.offset);
            d_timestamp = d_acq_info.timestamp + static_cast<uint64_t>(delta);
          }
        }

        // Take only as much items we can store in the buffer
        ninput_items = std::min(ninput_items, static_cast<int>(d_buffer_size - d_size));

        // copy values and errors
        memcpy(&d_buffer_values[d_size], input_items[0], ninput_items * sizeof(float));
        if (output_items.size() > 0) {
          memcpy(output_items[0], input_items[0], ninput_items * sizeof(float));
        }
        if (input_items.size() > 1) {
            memcpy(&d_buffer_errors[d_size], input_items[1], ninput_items * sizeof(float));
        }
        if (output_items.size() > 1) {
          memcpy(output_items[1], input_items[1], ninput_items * sizeof(float));
        }

        // Note!!! Tags should be decoded before d_size is incremented...
        handle_acq_info_tags(ninput_items);

        d_size += ninput_items;

        if (d_size == d_buffer_size)
        {
          d_waiting_readout = true;
        }

        // Release lock before we call a callback
      }

      if (d_size == d_buffer_size && d_callback != nullptr) {
        data_available_event_t args;
        args.trigger_timestamp = d_timestamp;
        args.signal_name = d_metadata.name;
        d_callback(&args, d_user_data);
      }

      return ninput_items;
    }

    int
    time_domain_sink_impl::work_fast_data(int ninput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
        assert(d_sink_mode == time_sink_mode_t::TIME_SINK_MODE_TRIGGERED);

      // Note, it is ok to drop non-triggered data while the user is reading
      // out the data from the buffer.
      {
        boost::mutex::scoped_lock lock(d_mutex);
        if (d_collecting_samples && d_waiting_readout) {
          return 0;
        }
      }

      if (d_collecting_samples) {
        ninput_items = std::min(d_samples_left, ninput_items);

        memcpy(&d_buffer_values[d_size], input_items[0], ninput_items * sizeof(float));
        if (output_items.size() > 0) {
          memcpy(output_items[0], input_items[0], ninput_items * sizeof(float));
        }
        if (input_items.size() > 1) {
          memcpy(&d_buffer_errors[d_size], input_items[1], ninput_items * sizeof(float));
        }
        if (output_items.size() > 1) {
          memcpy(output_items[1], input_items[1], ninput_items * sizeof(float));
        }

        d_samples_left -= ninput_items;
        d_size += ninput_items;

        if (d_samples_left == 0) {
          d_collecting_samples = false;

          {
            boost::mutex::scoped_lock lock(d_mutex);
            d_waiting_readout = true;
          }

          if (d_callback != nullptr) {
            data_available_event_t args;
            args.trigger_timestamp = d_acq_info.trigger_timestamp;
            args.signal_name = d_metadata.name;
            d_callback(&args, d_user_data);
          }
        }

        return ninput_items;
      }
      else {
        // We'are waiting for a trigger tag. Note d_tags is a member variable
        // for performance reasons. In this case it would be sufficient to use
        // a single element but this is not how the GR works.
        const uint64_t samp0_count = nitems_read(0);

        d_tags.clear();

        get_tags_in_range(d_tags, 0, samp0_count,
              samp0_count + ninput_items, pmt::string_to_symbol("acq_info"));
        // get rid of non-trigger tags
        d_tags.erase(std::remove_if(d_tags.begin(), d_tags.end(), [](gr::tag_t& x)
        {
          return !contains_triggered_data(x);
        }), d_tags.end());

        if (d_tags.size()) {
          // Handle a single trigger at once
          d_acq_info = decode_acq_info_tag(d_tags.at(0));

          d_samples_left = std::min((d_acq_info.pre_samples + d_acq_info.samples),  d_buffer_size);
          d_collecting_samples = true;

          // Ignore non-trigger data
          ninput_items = static_cast<int>(d_tags.at(0).offset - samp0_count);
        }

        return ninput_items;
      }
    }

    int
    time_domain_sink_impl::work_snapshot_data(int ninput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      assert(d_sink_mode == time_sink_mode_t::TIME_SINK_MODE_SNAPSHOT);

      handle_acq_info_tags(ninput_items);

      // no timing information available, or no user callback supplied
      // TODO: jgolob remove d_callback check for testing purposes
      if (d_acq_info.timestamp == -1
              || d_acq_info.last_beam_in_timestamp == -1 || d_callback == nullptr) {
        return ninput_items;
      }

      std::vector<int64_t> points_ns;
      {
        boost::mutex::scoped_lock lock(d_mutex);

        if (d_waiting_readout) {
          return 0;
        }

        if (d_size == 0) {
          // Starting fresh... Set status to the last acq_info status and calculate
          // the timestamp of the first sample.
          d_status = d_acq_info.status;
        }

        points_ns = d_snapshot_points;
      }

      // calculate timestamp of the first sample
      auto delta = nitems_read(0) > d_acq_info.offset
              ? static_cast<double>(nitems_read(0) - d_acq_info.offset)
              : static_cast<double>(d_acq_info.offset - nitems_read(0)) * -1.0;
      auto count0_timestamp = d_acq_info.timestamp + static_cast<int64_t>(
              delta * d_acq_info.timebase * 1000000000.0);
      // one sample is added in order to work with a full range and to avoid
      // gaps between iterations
      auto countn_timestamp = count0_timestamp + static_cast<int64_t>(
              (ninput_items + 1) * d_acq_info.timebase * 1000000000.0);

      auto it = std::find_if(points_ns.begin(), points_ns.end(),
              [count0_timestamp, countn_timestamp, this] (int64_t p)
      {
        auto abs_p_timestamp = d_acq_info.last_beam_in_timestamp + p;
        return abs_p_timestamp >= count0_timestamp && abs_p_timestamp < countn_timestamp;
      });

      if (it == points_ns.end()) {
          return ninput_items;
      }

      // found a snapshot
      auto timestamp = d_acq_info.last_beam_in_timestamp + *it;
      assert(timestamp >= count0_timestamp && timestamp < countn_timestamp);
      auto index = static_cast<int>(round(static_cast<double>(countn_timestamp - count0_timestamp)
              / static_cast<double>(timestamp - count0_timestamp)));
      assert(index >= 0 && index < ninput_items);

      {
        boost::mutex::scoped_lock lock(d_mutex);

        d_buffer_values.at(0) = static_cast<const float*>(input_items.at(0))[index];
          if (input_items.size() > 0) {
            d_buffer_values.at(0) = static_cast<const float*>(input_items.at(0))[index];
        }

        d_timestamp = timestamp;
        d_size = 1;
        d_waiting_readout = true;
      }

      assert(d_callback != nullptr);
      data_available_event_t args;
      args.trigger_timestamp = d_timestamp;
      args.signal_name = d_metadata.name;
      d_callback(&args, d_user_data);

      return ninput_items;
    }

    void
    time_domain_sink_impl::handle_acq_info_tags(int ninput_items)
    {
      assert(d_sink_mode != time_sink_mode_t::TIME_SINK_MODE_TRIGGERED);

      // Sample zero index
      const uint64_t samp0_count = this->nitems_read(0);

      // Acquisition info, store the last one, merge status
      d_tags.clear();
      get_tags_in_range(d_tags, 0, samp0_count,
            samp0_count + ninput_items, pmt::string_to_symbol("acq_info"));

      for(const auto& elem: d_tags) {
        auto acq_info = decode_acq_info_tag(elem);

        // Acquisition info is attached to the first sample we are about
        // to store in the buffer. Update status with the provided values
        // directly without merging anything.
        if ((d_size == 0) && (acq_info.offset == samp0_count)) {
          d_status = acq_info.status;
          d_timestamp = acq_info.timestamp;
        }
        else {
          d_status |= acq_info.status;
        }

        d_acq_info = acq_info;
      }
    }

    void
    time_domain_sink_impl::set_snapshot_points(const std::vector<float> &points)
    {
      boost::mutex::scoped_lock lock(d_mutex);

      d_snapshot_points.clear();

      for (auto p: points) {
        d_snapshot_points.push_back(static_cast<int64_t>(p * 1000000000.0));
      }
    }

    signal_metadata_t
    time_domain_sink_impl::get_metadata()
    {
      return d_metadata;
    }

    size_t
    time_domain_sink_impl::get_items_count()
    {
      boost::mutex::scoped_lock lock(d_mutex);
      return d_size;
    }

    size_t
    time_domain_sink_impl::get_items(size_t nr_items_to_read,
            float *values, float *errors, measurement_info_t *info)
    {
      if (d_sink_mode == time_sink_mode_t::TIME_SINK_MODE_TRIGGERED) {
        return get_fast_data_items(nr_items_to_read, values, errors, info);
      }
      else {
        return get_slow_data_items(nr_items_to_read, values, errors, info);
      }
    }

    size_t
    time_domain_sink_impl::get_slow_data_items(size_t nr_items_to_read,
            float *values, float *errors, measurement_info_t *info)
    {
      assert(d_sink_mode != time_sink_mode_t::TIME_SINK_MODE_TRIGGERED);

      boost::mutex::scoped_lock lock(d_mutex);

      if (!d_size) {
        d_waiting_readout = false;
        return 0;
      }

      assert(d_size);

      nr_items_to_read = std::min(nr_items_to_read, static_cast<size_t>(d_size));

      info->timebase = d_acq_info.timebase;
      info->user_delay = d_acq_info.user_delay;
      info->actual_delay = d_acq_info.actual_delay;
      info->timestamp = d_timestamp;
      info->trigger_timestamp = d_timestamp;
      info->status = d_status;
      info->pre_trigger_samples = 0;
      info->post_trigger_samples = d_size;

      // Transfer values and errors
      memcpy(values, &d_buffer_values[0], nr_items_to_read * sizeof(float));
      memcpy(errors, &d_buffer_errors[0], nr_items_to_read * sizeof(float));

      // Update state, note if user does not read-out all samples at once
      // the data will be lost
      d_size = 0;
      d_waiting_readout = false;

      return nr_items_to_read;
    }

    size_t
    time_domain_sink_impl::get_fast_data_items(size_t nr_items_to_read,
            float *values, float *errors, measurement_info_t *info)
    {
      assert(d_sink_mode == time_sink_mode_t::TIME_SINK_MODE_TRIGGERED);

      boost::mutex::scoped_lock lock(d_mutex);

      if (!d_waiting_readout) {
        return 0;
      }

      assert(d_size);

      nr_items_to_read = std::min(nr_items_to_read, static_cast<size_t>(d_size));

      if (nr_items_to_read < (d_acq_info.pre_samples + d_acq_info.samples)) {
        d_acq_info.status |= CHANNEL_STATUS_NOT_ALL_DATA_EXTRACTED;
      }

      info->timebase = d_acq_info.timebase;
      info->user_delay = d_acq_info.user_delay;
      info->actual_delay = d_acq_info.actual_delay;
      info->trigger_timestamp = d_acq_info.trigger_timestamp;
      info->timestamp = d_acq_info.timestamp;
      info->status = d_acq_info.status;
      info->pre_trigger_samples = d_acq_info.pre_samples;
      info->post_trigger_samples = d_acq_info.samples;

      memcpy(values, &d_buffer_values[0], nr_items_to_read * sizeof(float));
      memcpy(errors, &d_buffer_errors[0], nr_items_to_read * sizeof(float));

      // Update state, note if user does not read-out all samples at once
      // the unread samples are simply lost...
      d_size = 0;
      d_waiting_readout = false;

      return nr_items_to_read;
    }

    void
    time_domain_sink_impl::set_items_available_callback(data_available_cb_t callback, void *ptr)
    {
      d_callback = callback;
      d_user_data = ptr;
    }

    size_t
    time_domain_sink_impl::get_buffer_size()
    {
      return d_buffer_size;
    }

    time_sink_mode_t
    time_domain_sink_impl::get_sink_mode()
    {
      return d_sink_mode;
    }

    bool
    time_domain_sink_impl::is_data_rdy()
    {
      boost::mutex::scoped_lock lock(d_mutex);
      return (d_size == d_buffer_size);
    }

    void
    time_domain_sink_impl::notify_data_ready()
    {
      {
        boost::mutex::scoped_lock lock(d_mutex);
        if (!d_size) {
          return;
        }
        else {
          d_waiting_readout = true;
        }
      }

      if (d_callback != nullptr) {
        data_available_event_t args;
        args.signal_name = d_metadata.name;
        if (d_sink_mode == time_sink_mode_t::TIME_SINK_MODE_TRIGGERED) {
          args.trigger_timestamp = d_acq_info.trigger_timestamp;
        }
        else {
          args.trigger_timestamp = d_timestamp;
        }
        d_callback(&args, d_user_data);
      }
    }

  } /* namespace digitizers */
} /* namespace gr */

