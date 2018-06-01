/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include <digitizers/status.h>
#include "demux_ff_impl.h"

namespace gr {
  namespace digitizers {

    demux_ff::sptr
    demux_ff::make(float samp_rate, unsigned history, unsigned post_trigger_window, unsigned pre_trigger_window)
    {
      return gnuradio::get_initial_sptr
        (new demux_ff_impl(samp_rate, history, post_trigger_window, pre_trigger_window));
    }

    /*
     * The private constructor
     */
    demux_ff_impl::demux_ff_impl(float samp_rate, unsigned history, unsigned post_trigger_window, unsigned pre_trigger_window)
      : gr::block("demux_ff",
              gr::io_signature::make(1, 2, sizeof(float)),
              gr::io_signature::make(1, 2, sizeof(float))),
      d_samp_rate(samp_rate),
      d_my_history(std::max(history, pre_trigger_window + post_trigger_window)),
      d_pre_trigger_window(pre_trigger_window),
      d_post_trigger_window(post_trigger_window),
      d_state(extractor_state::WaitTrigger),
      d_last_trigger_offset(0),
      d_trigger_start_range(0),
      d_trigger_end_range(0),
      d_wr_events(1024), // reserve space for some absurd number of WR events
      d_realignment_events(1024),
      d_acq_info(4096)
    {
      // actual history size is in fact N - 1
      set_history(d_my_history + 1);

      // allows us to send a complete data chunk down the stream
      set_output_multiple(pre_trigger_window + post_trigger_window);

      set_tag_propagation_policy(TPP_DONT);
    }

    /*
     * Our virtual destructor.
     */
    demux_ff_impl::~demux_ff_impl()
    {
    }

    bool
    demux_ff_impl::start()
    {
      d_state = extractor_state::WaitTrigger;
      d_acq_info.clear();
      d_last_edge = edge_detect_t {};

      d_wr_events.clear();
      d_realignment_events.clear();

      return true;
    }

    void
    demux_ff_impl::forecast(int noutput_items, gr_vector_int &ninput_items_required)
    {
      for (auto & required : ninput_items_required) {
        required = noutput_items + history() - 1;
      }
    }

    int
    demux_ff_impl::general_work(int noutput_items,
                               gr_vector_int &ninput_items,
                               gr_vector_const_void_star &input_items,
                               gr_vector_void_star &output_items)
    {
      int retval = 0;
      const auto samp0_count = nitems_read(0);

      // Consume all WR event & edge detect tags
      {
      std::vector<gr::tag_t> tags;
      get_tags_in_range(tags, 0, samp0_count, samp0_count + noutput_items, pmt::string_to_symbol(wr_event_tag_name));
      for (const auto &tag : tags) {
        d_wr_events.push_back(decode_wr_event_tag(tag));
      }

      tags.clear();
      get_tags_in_range(tags, 0, samp0_count, samp0_count + noutput_items, pmt::string_to_symbol(edge_detect_tag_name));
      for (const auto &tag : tags) {
        d_realignment_events.push_back(decode_edge_detect_tag(tag));
      }
      }

      // Consume samples until a trigger tag is detected
      if (d_state == extractor_state::WaitTrigger) {

        std::vector<gr::tag_t> tags;
        get_tags_in_range(tags, 0, samp0_count, samp0_count + noutput_items, pmt::string_to_symbol(trigger_tag_name));

        for (const auto &tag : tags) {
          if (tag.offset > d_pre_trigger_window) {
            d_last_trigger_offset = tag.offset;
            d_state = extractor_state::WaitEvent;     // transition to WaitEvent

            d_last_wr_event = boost::none;
            d_last_edge = boost::none;

            GR_LOG_DEBUG(d_logger, "demux detected trigger at offset: " + std::to_string(d_last_trigger_offset));

            break;                                    // found it
          }
        }
      }

      // Trigger was detected, consume items until we receive WR timing event
      if (d_state == extractor_state::WaitEvent) {

        if (!d_wr_events.empty()) {

          d_last_wr_event = d_wr_events.front();
          d_wr_events.pop_front();

          GR_LOG_DEBUG(d_logger, "demux will use WR event from offset: " + std::to_string(d_last_wr_event->offset)
                  + ", realignment: " + std::to_string(d_last_wr_event->realignment_required)
                  + ", time sync: " + std::to_string(d_last_wr_event->time_sync_only));

          if (d_last_wr_event->time_sync_only) {
            GR_LOG_DEBUG(d_logger, "demux: time sync only from offset: "
                    + std::to_string(d_last_wr_event->offset));
            d_state = extractor_state::WaitTrigger; // Don't output anything
          }
          else if (d_last_wr_event->realignment_required) {
            d_state = extractor_state::WaitRealignmentTag;
          }
          else {
            d_state =  extractor_state::CalcOutputRange;
          }
        }
      }

      // Realignment delay can arrive even before the WR event tag but since we keep all the tags
      // in the circular buffer we can pop the first one once the WR event is received...
      if (d_state == extractor_state::WaitRealignmentTag) {

        if (!d_realignment_events.empty()) {
          d_last_edge = d_realignment_events.front();
          d_realignment_events.pop_front();

          GR_LOG_DEBUG(d_logger, "demux will use realignment event from offset: "
                  + std::to_string(d_last_edge->offset));

          d_state = extractor_state::CalcOutputRange;
        }
      }

      // Timeout, extract data without waiting for the event || realignment event
      if ((d_state == extractor_state::WaitRealignmentTag || d_state == extractor_state::WaitEvent)
              && ((int)d_my_history - noutput_items) < (int)(samp0_count - d_last_trigger_offset + d_pre_trigger_window)) {

        GR_LOG_DEBUG(d_logger, "demux timed out for trigger at offset: "
                + std::to_string(d_last_trigger_offset)
                + ", all the WR & edge events will be dropped");

        // Here we need to flush all the circular buffers in order to prevent misalignments
        d_wr_events.clear();
        d_realignment_events.clear();

        d_state = extractor_state::CalcOutputRange;
      }

      // Collect acq_info tags
      {
        std::vector<gr::tag_t> tags;
        get_tags_in_range(tags, 0, samp0_count, samp0_count + noutput_items, pmt::string_to_symbol(acq_info_tag_name));

        for (const auto &tag : tags) {
          d_acq_info.push_back(decode_acq_info_tag(tag));
        }
      }

      if (d_state == extractor_state::CalcOutputRange) {

        int realignment_delay_samples = 0;

        // Calculate realignment delay only if both edge & WR event tags are available and if
        // realignment is required at all
        if (d_last_wr_event && d_last_wr_event->realignment_required && d_last_edge) {
          if(d_last_edge->retrigger_event_timestamp < d_last_wr_event->timestamp) {
            GR_LOG_WARN(d_logger, "Negative realignment delay detected, continue!");
          }
          auto realignment_delay_ns = d_last_edge->retrigger_event_timestamp - d_last_wr_event->timestamp;
          realignment_delay_samples = realignment_delay_ns / 1000000000.0 * d_samp_rate;
        }

        int user_delay_samples = get_user_delay() * d_samp_rate;

        // delay could potentially be a negative number
        int delay_samples = realignment_delay_samples + user_delay_samples;

        // relative offset of the first sample based on count0
        int relative_trigger_offset =
                - static_cast<int>(samp0_count - d_last_trigger_offset)
                - d_pre_trigger_window
                + delay_samples;

        if (relative_trigger_offset < (-static_cast<int>(d_my_history))) {
          GR_LOG_ERROR(d_logger, "Can't extract data, not enough history available");

          d_state = extractor_state::WaitTrigger;

          consume_each(noutput_items);
          return 0;
        }

        d_trigger_start_range = nitems_read(0) + relative_trigger_offset;
        d_trigger_end_range = d_trigger_start_range + (d_pre_trigger_window + d_post_trigger_window);

        d_state = extractor_state::WaitAllData;
      }

      if (d_state == extractor_state::WaitAllData) {
        if (samp0_count < d_trigger_end_range) {
          noutput_items = std::min(static_cast<int>(d_trigger_end_range - samp0_count), noutput_items);
        }
        else {
          noutput_items = 0; // Don't consume anything in this iteration, output triggered data first
          d_state = extractor_state::OutputData;
        }
      }

      if (d_state == extractor_state::OutputData) {

        assert (samp0_count >= d_trigger_end_range);
        auto samples_2_copy = d_pre_trigger_window + d_post_trigger_window;
        assert(d_my_history > (samp0_count - d_trigger_end_range));
        auto start_index = d_my_history - (samp0_count - d_trigger_end_range) - samples_2_copy;
        assert(start_index >= 0 && start_index < (d_my_history + noutput_items));
        memcpy((char *)output_items.at(0),
               (char *)input_items.at(0) + start_index * sizeof(float),
               samples_2_copy * sizeof(float));
        if (input_items.size() > 1 && output_items.size() > 1) {
          memcpy((char *)output_items.at(1),
                 (char *)input_items.at(1) + start_index * sizeof(float),
                 samples_2_copy * sizeof(float));
        }

        retval = samples_2_copy;

        auto trigger_tag = make_trigger_tag();
        trigger_tag.offset = nitems_written(0) + d_pre_trigger_window;
        add_item_tag(0, trigger_tag);

        auto acq_info = calculate_acq_info_for_range(d_trigger_start_range,
                d_trigger_end_range, d_acq_info, d_samp_rate);
        acq_info.triggered_data = true;
        acq_info.offset = nitems_written(0);
        acq_info.pre_samples = d_pre_trigger_window;
        acq_info.samples = d_post_trigger_window;

        // trigger_timestamp is the timestamp of the original event, without realignment
        if (d_last_wr_event) {
          acq_info.trigger_timestamp = d_last_wr_event->timestamp;
          acq_info.last_beam_in_timestamp = d_last_wr_event->last_beam_in_timestamp;

          // WR event timestamp is a timestamp without any user delay applied
          acq_info.timestamp = d_last_wr_event->timestamp
                  - (d_pre_trigger_window / d_samp_rate * 1000000000.0);

          if (d_last_wr_event->realignment_required && d_last_edge) {
            auto realignment_delay_ns = (d_last_edge->retrigger_event_timestamp - d_last_wr_event->timestamp);
            acq_info.actual_delay += realignment_delay_ns / 1000000000.0;
          }
          else if (d_last_wr_event->realignment_required && !d_last_edge) {
            acq_info.status |= channel_status_t::CHANNEL_STATUS_TIMEOUT_WAITING_WR_OR_REALIGNMENT_EVENT;
          }
        }
        else {
          acq_info.status |= channel_status_t::CHANNEL_STATUS_TIMEOUT_WAITING_WR_OR_REALIGNMENT_EVENT;
          // In this case we simply keep the timestamp provided with the last acq_info tag. Trigger
          // timestamp is forced to -1 in order to indicate a realignment error.
          acq_info.trigger_timestamp = -1;
        }

        auto acq_info_tag = make_acq_info_tag(acq_info);
        add_item_tag(0, acq_info_tag);

        d_state = extractor_state::WaitTrigger;
      }

      consume_each(noutput_items);
      return retval;
    }

    double
    demux_ff_impl::get_user_delay() const
    {
      if (!d_acq_info.empty()) {
        return d_acq_info.back().user_delay;
      }

      return 0.0;
    }

  } /* namespace digitizers */
} /* namespace gr */

