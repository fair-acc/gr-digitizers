/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
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
    time_realignment_ff::make(float samp_rate, float user_delay)
    {
      return gnuradio::get_initial_sptr
        (new time_realignment_ff_impl(samp_rate, user_delay));
    }

    /*
     * The private constructor
     */
    time_realignment_ff_impl::time_realignment_ff_impl(float samp_rate, float user_delay)
      : gr::sync_block("time_realignment_ff",
              gr::io_signature::make(1, 3, sizeof(float)),
              gr::io_signature::make(1, 2, sizeof(float))),
       d_samp_rate(samp_rate),
       d_user_delay(user_delay),
       d_last_trigger_offset(0),
       d_last_wr_event(),
       d_pending_triggers(),
       d_pending_events()
    {
      set_tag_propagation_policy(tag_propagation_policy_t::TPP_DONT);
    }

    /*
     * Our virtual destructor.
     */
    time_realignment_ff_impl::~time_realignment_ff_impl()
    {
    }

    void
    time_realignment_ff_impl::set_user_delay(float user_delay)
    {
      d_user_delay = user_delay;
    }

    float
    time_realignment_ff_impl::get_user_delay()
    {
      return d_user_delay;
    }

    int
    time_realignment_ff_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      assert(input_items.size() >= 1);
      assert(input_items.size() >= output_items.size());

      // Just copy over all the data
      memcpy(output_items.at(0), input_items.at(0), noutput_items * sizeof(float));
      if (input_items.size() > 1 && output_items.size() > 1) {
        memcpy(output_items.at(1), input_items.at(1), noutput_items * sizeof(float));
      }

      const uint64_t samp0_count = nitems_read(0);

      // Get all the tags, for performance reason a member variable is used
      std::vector<gr::tag_t> tags;
      get_tags_in_range(tags, 0, samp0_count, samp0_count + noutput_items);

      // Dedicated input for sharing tags...
      if (input_items.size() == 3) {
        std::vector<gr::tag_t> share_tags;
        get_tags_in_range(share_tags, 2, samp0_count, samp0_count + noutput_items);
        tags.insert(tags.end(), share_tags.begin(), share_tags.end());
      }

      for (auto tag: tags) {

        if (tag.key == pmt::string_to_symbol(trigger_tag_name)) {
          push_and_update_last(tag.offset);
          add_item_tag(0, tag); // forward trigger tags
        }
        else if (tag.key == pmt::string_to_symbol(wr_event_tag_name)) {
          push_and_update_last(decode_wr_event_tag(tag));
          add_item_tag(0, tag); // forward WR tags
        }
        else if (tag.key == pmt::string_to_symbol(acq_info_tag_name)) {
          auto acq_info = decode_acq_info_tag(tag);

          // Keep original stamp for later context tracking in FESA
          acq_info.trigger_timestamp = d_last_wr_event.timestamp;

          // correct timestamp (note user delay is accounted for in calc timestamp method)
          acq_info.timestamp = calculate_timestamp(acq_info.offset, acq_info.timestamp);
          acq_info.last_beam_in_timestamp = d_last_wr_event.last_beam_in_timestamp;
          acq_info.user_delay += d_user_delay;
          acq_info.actual_delay += d_user_delay;

          auto updated_tag = make_acq_info_tag(acq_info);
          add_item_tag(0, updated_tag);
        }
        else {
          add_item_tag(0, tag); // forward by default
        }
      }

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

    bool
    time_realignment_ff_impl::start()
    {
      d_last_trigger_offset = 0;
      d_last_wr_event = wr_event_t {};
      d_last_wr_event.timestamp = -1;
      d_last_wr_event.last_beam_in_timestamp = -1;

      d_pending_events.clear();
      d_pending_triggers.clear();

      return true;
    }

    void
    time_realignment_ff_impl::push_and_update_last(const wr_event_t &event)
    {
      if (d_pending_events.size() > 1024) {
        GR_LOG_WARN(d_logger, "Large number of pending WR events detected... Make sure to configure"
                "flowgraph such that the same number of triggers & WR events is generated");
        d_pending_events.clear();
        d_pending_triggers.clear();
      }

      d_pending_events.push_back(event);
      update_last_if_needed();
    }

    void
    time_realignment_ff_impl::push_and_update_last(uint64_t trigger)
    {
      if (d_pending_triggers.size() > 1024) {
        GR_LOG_WARN(d_logger, "Large number of pending trigger events detected... Make sure to configure"
                "flowgraph such that the same number of triggers & WR events is generated");
        d_pending_events.clear();
        d_pending_triggers.clear();
      }

      d_pending_triggers.push_back(trigger);
      update_last_if_needed();
    }

    inline void
    time_realignment_ff_impl::update_last_if_needed()
    {
      while (!d_pending_triggers.empty() && !d_pending_events.empty()) {
        d_last_trigger_offset = d_pending_triggers.front();
        d_pending_triggers.pop_front();

        d_last_wr_event = d_pending_events.front();
        d_pending_events.pop_front();
      }
    }

    int64_t
    time_realignment_ff_impl::calculate_timestamp(uint64_t offset, int64_t fallback_timestamp) const
    {
      auto timestamp = fallback_timestamp;

      if (d_last_wr_event.timestamp > -1) {
        auto delta_samples = offset - d_last_wr_event.offset;
        auto delta_ns = (static_cast<float>(delta_samples) / d_samp_rate) * 1000000000.0;
        timestamp = d_last_wr_event.timestamp + static_cast<int64_t>(delta_ns);
      }

      // Add user delay
      timestamp += get_user_delay_ns();

      return timestamp;
    }

    int64_t
    time_realignment_ff_impl::get_user_delay_ns() const
    {
      return static_cast<int64_t>(d_user_delay * 1000000000.0);
    }

  } /* namespace digitizers */
} /* namespace gr */

