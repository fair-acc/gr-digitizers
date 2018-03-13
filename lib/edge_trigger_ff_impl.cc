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
#include "edge_trigger_ff_impl.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/tokenizer.hpp>
#include <digitizers/edge_trigger_utils.h>
#include <algorithm>
#include "utils.h"

namespace gr {
  namespace digitizers {

    edge_trigger_ff::sptr
    edge_trigger_ff::make(float sampling, float lo, float hi, float initial_state,
            bool send_udp, const std::string host_list, bool send_udp_on_raising_edge)
    {
      return gnuradio::get_initial_sptr
        (new edge_trigger_ff_impl(sampling, lo, hi, initial_state, send_udp, host_list, send_udp_on_raising_edge));
    }

    /*
     * The private constructor
     */
    edge_trigger_ff_impl::edge_trigger_ff_impl(float sampling, float lo, float hi,
            float initial_state, bool send_udp, const std::string host_list, bool send_udp_on_raising_edge)
      : gr::block("edge_trigger_ff",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(0, 1, sizeof(float))),
        d_sampling_rate(sampling),
        d_lo_threshold(lo),
        d_hi_threshold(hi),
        d_actual_state(initial_state),
        d_last_timing_event(-1),
        d_last_timing_event_delay(0.0),
        d_samples_since_last_timing_event(0),
        d_send_udp_packet(send_udp),
        d_send_udp_on_raising_edge(send_udp_on_raising_edge),
        d_sent(false)
    {
      // parse receiving host names and ports
      std::vector<std::string> hosts;
      boost::tokenizer<boost::char_separator<char>> tokens(host_list, boost::char_separator<char>(", "));
      for(auto& t: tokens) {
        std::string host = t;
        host.erase(std::remove(host.begin(), host.end(), '"'), host.end());
        hosts.push_back(host);
      }

      for (const auto &host : hosts) {
        std::vector<std::string> parts;
        boost::algorithm::split(parts, host, [] (char c) { return c == ':'; });

        auto new_client = boost::make_shared<udp_sender>(d_io_service, parts.at(0), parts.at(1));
        d_receivers.push_back(new_client);

        GR_LOG_DEBUG(d_logger, "edge_trigger_ff::registered host: '" + new_client->host_and_port() + "'");
      }
    }

    /*
     * Our virtual destructor.
     */
    edge_trigger_ff_impl::~edge_trigger_ff_impl()
    {
    }

    // Callback to set whether should send udp packets or not (required for online debugging)
    void
    edge_trigger_ff_impl::set_send_udp(bool send_state) {
      d_send_udp_packet = send_state;
    }

    // send PMD and UDP packet
    void
    edge_trigger_ff_impl::send_udp_message(bool rising_edge, const float value)
    {
      if (!d_send_udp_packet || d_receivers.empty()) {
        return;
      }

      int64_t trigger_event_time_stamp = d_last_timing_event;
      uint64_t trigger_delay = (uint64_t)((d_samples_since_last_timing_event / (double)d_sampling_rate * 1e9)
              + d_last_timing_event_delay * 1e9);
      int64_t retrigger_event_time_stamp = d_last_timing_event > 0
              ? d_last_timing_event + trigger_delay : -1;

      // prepare UDP packet
      edge_detect_t ed;
      ed.is_raising_edge = rising_edge;
      ed.value = value;
      ed.delay_since_last_timing_event = trigger_delay;
      ed.retrigger_event_timestamp = retrigger_event_time_stamp;
      ed.samples_since_last_timing_event = d_samples_since_last_timing_event;
      ed.timing_event_timestamp = trigger_event_time_stamp;

      auto message = encode_edge_detect(ed);

      for (auto &receiver : d_receivers) {
        receiver->send(message);
      }
    }

    void
    edge_trigger_ff_impl::forecast(int noutput_items, gr_vector_int &ninput_items_required)
    {
      // default implementation:  1:1
      for (auto &inrequired : ninput_items_required) {
        inrequired = noutput_items;
      }
    }

    void
    edge_trigger_ff_impl::add_edge_and_delay_tags(uint64_t offset, bool detected_rising_edge)
    {
      // edge tag
      tag_t edge_tag;
      edge_tag.offset = offset;
      edge_tag.key = pmt::mp("edgeDetect");
      edge_tag.value = pmt::from_bool(detected_rising_edge);
      add_item_tag(0, edge_tag);

      // add time delay tag
      tag_t sample_offset_tag;
      sample_offset_tag.offset = offset;
      sample_offset_tag.key = pmt::mp("sampleOffsetToTiming");
      sample_offset_tag.value = pmt::from_long(d_samples_since_last_timing_event);
      add_item_tag(0, sample_offset_tag);
    }

    int
    edge_trigger_ff_impl::general_work(int noutput_items, gr_vector_int &ninput_items,
          gr_vector_const_void_star &input_items, gr_vector_void_star &output_items)
    {
      const float *in = (const float *)input_items.at(0);

      // Just in case
      int items_to_read = ninput_items.at(0);
      if (!output_items.empty()) {
          items_to_read = std::min(items_to_read, noutput_items);
      }

      auto count0 = nitems_read(0);

      // detect timing event
      d_tags.clear();
      get_tags_in_range(d_tags,
            0, // Port 0
            count0,
            count0 + items_to_read,
            pmt::mp("acq_info")
      );
      // get rid of non-trigger tags
      d_tags.erase(std::remove_if(d_tags.begin(), d_tags.end(), [](gr::tag_t& x)
      {
        return !contains_triggered_data(x);
      }), d_tags.end());

      if (d_tags.size()) {
        d_trigger_pending = decode_acq_info_tag(d_tags.at(0));

        // ignore pre-trigger samples sice we cannot work with negative delays
        d_trigger_pending.offset += static_cast<uint64_t>(d_trigger_pending.pre_samples);
        d_trigger_pending.pre_samples = 0;

        // consume all the items before the newly detected trigger
        if (d_trigger_pending.offset > count0) {
          items_to_read = std::min(items_to_read,
                  static_cast<int>(d_trigger_pending.offset - count0));
        }
      }

      // decode timebase tag
      d_tags.clear();
      get_tags_in_range(d_tags, 0, count0,
              count0 + items_to_read, pmt::string_to_symbol("timebase_info"));

      // Note we simply take the very last tag and ignore others. Update
      // rate shouldn't change during the run.
      if(d_tags.size()) {
        d_sampling_rate = 1.0 / decode_timebase_info_tag(d_tags.at(d_tags.size() - 1));
      }

      for(int i = 0; i < items_to_read; i++) {

        if (d_trigger_pending.offset == (count0 + static_cast<uint64_t>(i))) {
          d_trigger = d_trigger_pending;
          d_sampling_rate = 1.0 / d_trigger_pending.timebase;

          d_last_timing_event = d_trigger.trigger_timestamp;
          d_last_timing_event_delay = d_trigger.user_delay;
          d_samples_since_last_timing_event = 0;
          d_sent = false;
        }
        else {
          d_samples_since_last_timing_event++;
        }

        bool detected_rising_edge;

        if (d_actual_state < 0.5) {
          // actual_state is '0'
          if (in[i] > d_hi_threshold) {
            detected_rising_edge = true;
            d_actual_state = 1;

            if (!output_items.empty()) {
              add_edge_and_delay_tags(count0 + i, detected_rising_edge);
            }
            if (d_send_udp_on_raising_edge && !d_sent) {
              send_udp_message(detected_rising_edge, in[i]);
              d_sent = true;
            }
          }
        }
        else {
          // actual_state is '1'
          if (in[i] < d_lo_threshold) {
            detected_rising_edge = false;
            d_actual_state = 0;

            if (!output_items.empty()) {
             add_edge_and_delay_tags(count0 + i, detected_rising_edge);
            }
            if (!d_send_udp_on_raising_edge && !d_sent) {
              send_udp_message(detected_rising_edge, in[i]);
              d_sent = true;
            }
          }
        }

        if (!output_items.empty()) {
          float *out = static_cast<float *>(output_items.at(0));
          out[i] = d_actual_state;
        }
      }

      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each(items_to_read);

      // Tell runtime system how many output items we produced.
      return items_to_read;
    }
  } /* namespace digitizers */
} /* namespace gr */

