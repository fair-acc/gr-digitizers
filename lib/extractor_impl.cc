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
#include "extractor_impl.h"
#include <algorithm>

namespace gr {
  namespace digitizers {

    /**********************************************************************
     * Factory
     *********************************************************************/

    extractor::sptr
    extractor::make(float post_trigger_window, float pre_trigger_window)
    {
      return gnuradio::get_initial_sptr
        (new extractor_impl(post_trigger_window, pre_trigger_window));
    }

    /**********************************************************************
     * Structors
     *********************************************************************/

    extractor_impl::extractor_impl(float post_trigger_window, float pre_trigger_window)
      : gr::block("extractor",
            gr::io_signature::make(1, 2, sizeof(float)),
            gr::io_signature::make(1, 2, sizeof(float))),
        d_pre_trigger_window(pre_trigger_window),
        d_post_trigger_window(post_trigger_window),
        d_state()
    {
      set_tag_propagation_policy(TPP_DONT);
    }

    /*
     * Our virtual destructor.
     */
    extractor_impl::~extractor_impl()
    {
    }

    /**********************************************************************
     * Worker stuff & helpers
     **********************************************************************/

    bool
    extractor_impl::start()
    {
      d_state.trigger_samples = 0;
      d_state.start_trigger_offset = 0;

      return true;
    }

    void
    extractor_impl::forecast(int noutput_items, gr_vector_int &ninput_items_required)
    {
      for (auto & required : ninput_items_required) {
        required = noutput_items;
      }
    }

    int
    extractor_impl::general_work(int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      if (input_items.size() != output_items.size()) {
        throw std::runtime_error("non-symmetrical configuration is not supported");
      }

      // We can only consume this much data
      int nr_input_items = *std::min_element(ninput_items.begin(), ninput_items.end());
      nr_input_items = std::min(nr_input_items, noutput_items);
      if (!nr_input_items) {
        return 0;
      }

      const uint64_t samp0_count = nitems_read(0);

      // In order to simplify the code, only consume items by the end of the trigger
      // (if reading one)
      if (d_state.trigger_samples
              && samp0_count < (d_state.start_trigger_offset + d_state.trigger_samples))
      {
        nr_input_items = std::min(nr_input_items,
           static_cast<int>((d_state.start_trigger_offset + d_state.trigger_samples) - samp0_count));
      }

      // Get all the tags in reading range
      std::vector<gr::tag_t> tags;
      get_tags_in_range(tags, 0, samp0_count, samp0_count + nr_input_items);

      // Check if an input vector contains trigger tags, we expect to get them on the first
      // input only. Only perform check if the previous trigger was read completely.
      if (samp0_count >= (d_state.start_trigger_offset + d_state.trigger_samples)) {

        auto it = std::find_if(tags.begin(), tags.end(), [] (const gr::tag_t &tag)
        {
          return tag.key == pmt::string_to_symbol("acq_info") && contains_triggered_data(tag);
        });

        // All the hassle just to apply user delay and perform realignment (if needed)
        if (it != tags.end()) {
          auto trigger_tag = decode_acq_info_tag(*it);

          auto all_available_samples = static_cast<int>(trigger_tag.samples + trigger_tag.pre_samples);

          const auto desired_pre_samples = static_cast<int>(d_pre_trigger_window / trigger_tag.timebase);
          const auto desired_samples = static_cast<int>(d_post_trigger_window / trigger_tag.timebase);
          const auto realignment_delay = static_cast<int>(trigger_tag.actual_delay / trigger_tag.timebase);

          // Relative offset of the first sample
          int offset = (int)trigger_tag.pre_samples - desired_pre_samples + realignment_delay;

          if ((offset < 0) || (offset + desired_pre_samples + desired_samples) > all_available_samples) {
            GR_LOG_ERROR(d_logger, "not enough samples to perform extraction");
            GR_LOG_ERROR(d_logger, "  available samples: "
                      + std::to_string(trigger_tag.pre_samples) + " + "
                      + std::to_string(trigger_tag.samples));
            GR_LOG_ERROR(d_logger, "  desired samples: "
                      + std::to_string(desired_pre_samples) + " + "
                      + std::to_string(desired_samples));
            GR_LOG_ERROR(d_logger, "  offset: "
                      + std::to_string(offset) + ", realignment:  "
                      + std::to_string(realignment_delay));

            // User defined delay is not applied nor is realignment
            trigger_tag.status |= (channel_status_t::CHANNEL_STATUS_REALIGNMENT_ERROR);

            d_state.start_trigger_offset = trigger_tag.offset;
            d_state.trigger_samples = all_available_samples;
          }
          else {
            d_state.start_trigger_offset = trigger_tag.offset + static_cast<uint64_t>(offset);
            d_state.trigger_samples = desired_pre_samples + desired_samples;

            trigger_tag.pre_samples = desired_pre_samples;
            trigger_tag.samples = desired_samples;

            // calculate timestamp of the first pre-sample
            trigger_tag.timestamp += ((trigger_tag.timebase * static_cast<double>(offset)) / 1000000000.0);
          }

          d_state.acq_info = trigger_tag;

          // Update number of items to be consumed in the first iteration. Only this many items
          // is consumed becase it simplifies the code (searching for triggers done only in the
          // beginning)
          nr_input_items = std::min(nr_input_items,
                static_cast<int>((d_state.start_trigger_offset + d_state.trigger_samples) - samp0_count));
        }
      }

      if (!d_state.trigger_samples) {
        consume_each(nr_input_items);
        return 0;
      }

      assert(nr_input_items > 0);

      const bool reading_errors = input_items.size() > 1;

      auto *dst = static_cast<float *>(output_items.at(0));
      auto *dst_err = static_cast<float *>(reading_errors ? output_items.at(1) : nullptr);
      auto *src = static_cast<const float *>(input_items.at(0));
      auto *src_err = static_cast<const float *>(reading_errors ? input_items.at(1) : nullptr);

      int dst_idx = 0;

      for (int i=0; i<nr_input_items; i++) {

        if (d_state.start_trigger_offset == samp0_count + i) {
          auto tag = make_acq_info_tag(d_state.acq_info);
          tag.offset = nitems_written(0) + static_cast<uint64_t>(dst_idx);
          add_item_tag(0, tag);
        }
        else if ((samp0_count + i) < d_state.start_trigger_offset
            || (samp0_count + i) >= (d_state.start_trigger_offset + d_state.trigger_samples)) {
          continue;
        }

        dst[dst_idx] = src[i];
        if (reading_errors) {
          dst_err[dst_idx] = src_err[i];
        }

        dst_idx++; // don't forget
      }

      consume_each(nr_input_items);
      return dst_idx;
    }

  } /* namespace digitizers */
} /* namespace gr */

