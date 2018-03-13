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
#include "post_mortem_sink_impl.h"

#include <string>
#include <algorithm>

namespace gr {
  namespace digitizers {

    post_mortem_sink::sptr
    post_mortem_sink::make(std::string name, std::string unit, size_t buffer_size)
    {
      return gnuradio::get_initial_sptr
        (new post_mortem_sink_impl(name, unit, buffer_size));
    }

    post_mortem_sink_impl::post_mortem_sink_impl(std::string name, std::string unit, size_t buffer_size)
      : gr::sync_block("post_mortem_sink",
              gr::io_signature::make(1, 2, sizeof(float)),
              gr::io_signature::make(1, 2, sizeof(float))),
        d_buffer_values(buffer_size),
        d_buffer_errors(buffer_size),
        d_buffer_size(buffer_size),
        d_write_index(0),
        d_acq_info(),
        d_metadata(),
        d_frozen(false)
    {
        // This allows us to simply copy zero values to the client
        memset(&d_buffer_errors[0], 0, buffer_size * sizeof(float));

        d_acq_info.timestamp = -1;

        d_metadata.name = name;
        d_metadata.unit = unit;
    }

    post_mortem_sink_impl::~post_mortem_sink_impl()
    {
    }

    int
    post_mortem_sink_impl::work(int ninput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      boost::mutex::scoped_lock lock(d_mutex);

      auto reading_errors = input_items.size() > 1;

      // Data is frozen, copy data to outputs and return
      if (d_frozen) {
          memcpy(output_items[0], input_items[0], ninput_items * sizeof(float));
          if (reading_errors) {
            memcpy(output_items[1], input_items[1], ninput_items * sizeof(float));
          }
          return ninput_items;
      }

      // For simplicity perform only a single memcpy at once
      assert(d_write_index < d_buffer_size);
      ninput_items = std::min(ninput_items, static_cast<int>(d_buffer_size - d_write_index));

      memcpy(&d_buffer_values[d_write_index], input_items[0], ninput_items * sizeof(float));
      memcpy(output_items[0], input_items[0], ninput_items * sizeof(float));
      if (reading_errors) {
        memcpy(&d_buffer_errors[d_write_index], input_items[1], ninput_items * sizeof(float));
        memcpy(output_items[1], input_items[1], ninput_items * sizeof(float));
      }

      // Update write index and wrap around if needed
      d_write_index += ninput_items;
      if (d_write_index == d_buffer_size) {
        d_write_index = 0;
      }

      // Acq info tag
      decode_tags(ninput_items);

      return ninput_items;
    }

    void
    post_mortem_sink_impl::decode_tags(int ninput_items)
    {
      // Sample zero index
      const uint64_t samp0_count = this->nitems_read(0);
      std::vector<gr::tag_t> tags;

      // Acquisition info, store the last one
      get_tags_in_range(tags, 0, samp0_count,
            samp0_count + ninput_items, pmt::string_to_symbol("acq_info"));

      if(tags.size()) {
        d_acq_info = decode_acq_info_tag(tags.at(tags.size() - 1));
      }
    }

    signal_metadata_t
    post_mortem_sink_impl::get_metadata()
    {
      boost::mutex::scoped_lock lock(d_mutex);
      return d_metadata;
    }

    size_t
    post_mortem_sink_impl::get_buffer_size()
    {
      return d_buffer_size;
    }

    void
    post_mortem_sink_impl::freeze_buffer()
    {
      boost::mutex::scoped_lock lock(d_mutex);
      d_frozen = true;
    }

    size_t
    post_mortem_sink_impl::get_items(size_t nr_items_to_read,
            float *values, float *errors, measurement_info_t *info)
    {
      boost::mutex::scoped_lock lock(d_mutex);

      auto count = nitems_read(0);
      if (count < uint64_t{d_buffer_size}) {
        nr_items_to_read = std::min(nr_items_to_read, static_cast<size_t>(count));
      }
      else {
        nr_items_to_read = std::min(nr_items_to_read, d_buffer_size);
      }

      auto transfered = size_t{0};
      auto index = static_cast<int64_t>(d_write_index) - static_cast<int64_t>(nr_items_to_read);

      if (index < 0) {
        auto from = static_cast<int64_t>(d_buffer_size) + index;
        auto size = index * (-1);
        memcpy(values, &d_buffer_values[from], (size) * sizeof(float));
        memcpy(errors, &d_buffer_errors[from], (size) * sizeof(float));
        transfered += size;
      }

      auto size = nr_items_to_read - transfered;
      auto from = d_write_index - size;
      memcpy(&values[transfered], &d_buffer_values[from], (size) * sizeof(float));
      memcpy(&errors[transfered], &d_buffer_errors[from], (size) * sizeof(float));

      // For now we simply copy over the last status

      info->timebase = d_acq_info.timebase;
      info->user_delay = d_acq_info.user_delay;
      info->actual_delay = d_acq_info.actual_delay;
      info->trigger_timestamp = d_acq_info.trigger_timestamp;
      info->status = d_acq_info.status;
      info->pre_trigger_samples = 0;
      info->post_trigger_samples = nr_items_to_read;


      // Calculate timestamp
      if (d_acq_info.timestamp < 0) {
        info->timestamp = -1; // timestamp is invalid
      }
      else {
        auto offset_first_sample = nitems_read(0) - nr_items_to_read;
        if (offset_first_sample >= d_acq_info.offset) {
          auto delta = d_acq_info.timebase
                  * (offset_first_sample - d_acq_info.offset) * 1000000000.0;
          info->timestamp = d_acq_info.timestamp + static_cast<uint64_t>(delta);
        }
        else {
          auto delta = d_acq_info.timebase
                  * (d_acq_info.offset - offset_first_sample) * 1000000000.0;
          info->timestamp = d_acq_info.timestamp - static_cast<uint64_t>(delta);
        }
      }

      d_frozen = false;

      return nr_items_to_read;
    }

  } /* namespace digitizers */
} /* namespace gr */

