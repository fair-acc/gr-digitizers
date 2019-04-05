/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
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

#include "utils.h"

namespace gr {
  namespace digitizers {

    edge_trigger_ff::sptr
    edge_trigger_ff::make(float sampling, float lo, float hi, float initial_state,
            bool send_udp, const std::string host_list, bool send_udp_on_raising_edge, float timeout)
    {
      return gnuradio::get_initial_sptr
        (new edge_trigger_ff_impl(sampling, lo, hi, initial_state, send_udp, host_list,
                send_udp_on_raising_edge, timeout));
    }

    // To be on the safe side allocate big circular buffers
    static const size_t CIRC_BUFFER_SIZE = 1024;
    static const size_t EDGE_CIRC_BUFFER_SIZE = 4096;

    /*
     * The private constructor
     */
    edge_trigger_ff_impl::edge_trigger_ff_impl(float sampling, float lo, float hi,
            float initial_state, bool send_udp, const std::string host_list,
            bool send_udp_on_raising_edge, float timeout)
      : gr::block("edge_trigger_ff",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(1, 1, sizeof(float))),
        d_sampling_rate(sampling),
        d_lo_threshold(lo),
        d_hi_threshold(hi),
        d_actual_state(initial_state < hi ? false : true),
        d_timeout_samples(timeout * sampling),
        d_wr_events(CIRC_BUFFER_SIZE),
        d_triggers(CIRC_BUFFER_SIZE),
        d_detected_edges(EDGE_CIRC_BUFFER_SIZE),
        d_send_udp_packet(send_udp),
        d_send_udp_on_raising_edge(send_udp_on_raising_edge)
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

      set_tag_propagation_policy(tag_propagation_policy_t::TPP_DONT);
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

    void
    edge_trigger_ff_impl::forecast(int noutput_items, gr_vector_int &ninput_items_required)
    {
      for (auto & required : ninput_items_required) {
        required = noutput_items;
      }
    }

    void
    edge_trigger_ff_impl::send_edge_detect_info(uint64_t trigger, uint64_t detected_edge, const wr_event_t &wr_event, bool make_tags)
    {
      int64_t trigger_event_time_stamp = wr_event.wr_trigger_stamp;
      uint64_t trigger_delay = static_cast<uint64_t>(
                ((detected_edge - trigger) / (double)d_sampling_rate * 1e9) + d_acq_info.user_delay * 1e9);
      int64_t retrigger_event_time_stamp = wr_event.wr_trigger_stamp < 0 ? -1
                : wr_event.wr_trigger_stamp + trigger_delay;

      // prepare UDP packet
      edge_detect_t ed;
      ed.is_raising_edge = d_send_udp_on_raising_edge;
      ed.value = d_send_udp_on_raising_edge;
      ed.delay_since_last_timing_event = trigger_delay;
      ed.retrigger_event_timestamp = retrigger_event_time_stamp;
      ed.samples_since_last_timing_event = (detected_edge - trigger);
      ed.timing_event_timestamp = trigger_event_time_stamp;

      if (d_send_udp_packet && !d_receivers.empty()) {
        auto message = encode_edge_detect(ed);

        for (auto &receiver : d_receivers) {
          receiver->send(message);
        }
      }

      // not nice, prevents tag generation in case no output port is connected
      if (make_tags) {
        auto tag = make_edge_detect_tag(ed);
        auto tag_offset = std::max(detected_edge, nitems_written(0));
        tag.offset = tag_offset;
        add_item_tag(0, tag);
      }
    }

    bool edge_trigger_ff_impl::start()
    {
      d_wr_events.clear();
      d_triggers.clear();
      d_detected_edges.clear();

      return true;
    }

    int
    edge_trigger_ff_impl::general_work(int noutput_items, gr_vector_int &ninput_items,
          gr_vector_const_void_star &input_items, gr_vector_void_star &output_items)
    {
      const float *in = (const float *)input_items.at(0);

      auto count0 = nitems_read(0);

      // Consume all the WR events
      std::vector<tag_t> tags;

      get_tags_in_range(tags, 0, count0, count0 + noutput_items, pmt::mp(wr_event_tag_name));
      for (const auto &tag : tags) {
        d_wr_events.push_back(decode_wr_event_tag(tag));
      }

      // Detect all triggers
      tags.clear();
      get_tags_in_range(tags, 0, count0, count0 + noutput_items, pmt::mp(trigger_tag_name));
      for (const auto &tag : tags) {
        d_triggers.push_back(tag.offset);
      }

      // Acq info tags are only needed to get info about the user delay
      tags.clear();
      get_tags_in_range(tags, 0, count0, count0 + noutput_items, pmt::mp(acq_info_tag_name));
      if (!tags.empty()) {
        d_acq_info = decode_acq_info_tag(tags.back());
      }

      const bool outputing = !output_items.empty();

      // Do signal processing
      float *out = nullptr;
      if (outputing) {
        out = static_cast<float *>(output_items.at(0));
      }

      for(int i = 0; i < noutput_items; i++) {

        bool edge_detected = false;

        if (d_actual_state == false) {
          // actual_state is '0'
          if (in[i] > d_hi_threshold) {
            edge_detected = d_send_udp_on_raising_edge;
            d_actual_state = true;
          }
        }
        else {
          // actual_state is '1'
          if (in[i] < d_lo_threshold) {
            edge_detected = !d_send_udp_on_raising_edge;
            d_actual_state = false;
          }
        }

        if (outputing) {
          out[i] = static_cast<float>(d_actual_state);
        }

        if (edge_detected) {
          d_detected_edges.push_back((count0 + i));
        }
      }

      // Trigger detection logic
      size_t triggers_consumed = 0;

      for (const auto trigger: d_triggers) {

        // To detect timeout
        const auto samples_since_trigger = count0 <= trigger ? 0 : count0 - trigger;

        // It is assumed that first WR event belongs to the first trigger. We cannot really
        // detect sporadic or misaligned events.
        if (d_wr_events.empty() ) {
          if (samples_since_trigger > d_timeout_samples) {
            GR_LOG_ERROR(d_logger, "Timeout receiving WR event for trigger at offset: "
                    + std::to_string(trigger));
            triggers_consumed++;
            continue;
          }

          break; // wait another work iteration to receive WR event
        }

       // const auto &wr_event = d_wr_events.front();

        // Edge detection is done only for events that require realignment
     //   if (!wr_event.realignment_required) {
          d_wr_events.pop_front();
          triggers_consumed++;
     //   }
//        else {
//          // find first relevant detected edge
//          bool detected = false;
//          uint64_t detected_edge = 0;
//
//          while (!d_detected_edges.empty()) {
//            detected_edge = d_detected_edges.front();
//            d_detected_edges.pop_front();
//
//            if (detected_edge >= trigger) {
//              detected = true;
//              break;
//            }
//          }

//          if (detected) {
//            send_edge_detect_info(trigger, detected_edge, wr_event, !output_items.empty());
//            d_wr_events.pop_front();
//            triggers_consumed++;
//          }
//          else if (samples_since_trigger > d_timeout_samples) {
//            GR_LOG_ERROR(d_logger, "Timeout detecting edge for trigger at offset: "
//                    + std::to_string(trigger));
//            triggers_consumed++;
//          }
//        }
      }

      for (size_t i = 0; i < triggers_consumed; i++) {
        d_triggers.pop_front();
      }

      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each(noutput_items);

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }
  } /* namespace digitizers */
} /* namespace gr */

