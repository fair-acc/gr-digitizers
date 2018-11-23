/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include <gnuradio/io_signature.h>
#include <digitizers/tags.h>
#include "time_realignment_ff_impl.h"
#include <digitizers/status.h>

#define NUMBER_OF_PENDING_TRIGGERS_WARNING 100
#define NUMBER_OF_PENDING_TRIGGERS_ERROR 1000

namespace gr {
  namespace digitizers {

    time_realignment_ff::sptr
    time_realignment_ff::make(float user_delay, float timeout)
    {
      return gnuradio::get_initial_sptr
        (new time_realignment_ff_impl(user_delay, timeout));
    }

    /*
     * The private constructor
     */
    time_realignment_ff_impl::time_realignment_ff_impl(float user_delay, float triggerstamp_matching_tolerance)
      : gr::sync_block("time_realignment_ff",
              gr::io_signature::make(1, 3, sizeof(float)),
              gr::io_signature::make(1, 2, sizeof(float))),
       d_user_delay(user_delay),
       d_pending_events()
    {
      set_triggerstamp_matching_tolerance(triggerstamp_matching_tolerance);
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

    void time_realignment_ff_impl::set_triggerstamp_matching_tolerance(float triggerstamp_matching_tolerance)
    {
        //std::cout << "set_timeout: " << triggerstamp_matching_tolerance << std::endl;
        d_triggerstamp_matching_tolerance_ns = int64_t(triggerstamp_matching_tolerance * 1000000000 );
        //std::cout << "d_timeout_ns: " << d_triggerstamp_matching_tolerance_ns << std::endl;
    }

    float time_realignment_ff_impl::get_triggerstamp_matching_tolerance()
    {
        return float(d_triggerstamp_matching_tolerance_ns / 1000000000);
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

      // Dedicated input for sharing tags...
      if (input_items.size() == 3)
      {
        std::vector<gr::tag_t> share_tags;
        get_tags_in_range(share_tags, 2, samp0_count, samp0_count + noutput_items);
        for (auto tag: share_tags)
        {
            if (tag.key == pmt::string_to_symbol(wr_event_tag_name))
            {
              //std::cout << "wr tag incoming" << std::endl;
              push_wr_tag(decode_wr_event_tag(tag));
              // wr-tags are not forwarded !
            }
        }
      }

      // Get all the tags, for performance reason a member variable is used
      std::vector<gr::tag_t> tags;
      get_tags_in_range(tags, 0, samp0_count, samp0_count + noutput_items);
      for (auto tag: tags)
      {
        if (tag.key == pmt::string_to_symbol(trigger_tag_name))
        {
          //std::cout << "trigger tag incoming. Offset: " << tag.offset << std::endl;
          trigger_t trigger_tag_data = decode_trigger_tag(tag);

          fill_wr_stamp(trigger_tag_data);
          add_item_tag(0, make_trigger_tag(trigger_tag_data,tag.offset)); // add tag to port 0
        }
        else if (tag.key == pmt::string_to_symbol(wr_event_tag_name))
        {
          //std::cout << "wr tag incoming" << std::endl;
          push_wr_tag(decode_wr_event_tag(tag));
          // wr-tags are not forwarded !
        }
        else
        {
            //std::cout << "unknown tag incoming" << std::endl;
          add_item_tag(0, tag); // forward all others by default
        }
      }

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

    bool
    time_realignment_ff_impl::start()
    {
      d_pending_events.clear();
      return true;
    }

    void
    time_realignment_ff_impl::fill_wr_stamp(trigger_t &trigger_tag_data)
    {
        //std::cout << "start search" << std::endl;
        for (auto pending_event = d_pending_events.begin(); pending_event!= d_pending_events.end();++pending_event)
        {
            int64_t delta_t = abs( trigger_tag_data.timestamp - pending_event->wr_trigger_stamp_utc );
            if(  delta_t < d_triggerstamp_matching_tolerance_ns )
            {
                //std::cout << "found !!! " << std::endl;
                //std::cout << "delta_t [sec]                      : " << delta_t/1000000000.f << std::endl;
                trigger_tag_data.timestamp = pending_event->wr_trigger_stamp;
                d_pending_events.erase(pending_event);
                return;
            }
            else
            {
//                std::cout << "pending_event.wr_trigger_stamp_utc : " << pending_event->wr_trigger_stamp_utc << std::endl;
//                std::cout << "trigger_tag_data.timestamp         : " << trigger_tag_data.timestamp << std::endl;
//                std::cout << "delta_t [sec]                      : " << delta_t/1000000000.f << std::endl;
//                std::cout << "timeout[s]                         : " << d_triggerstamp_matching_tolerance_ns/1000000000.f << std::endl;
//                GR_LOG_WARN(d_logger, "Time Realigment: Matching tolerance for trigger tag matching exceeded by '" + std::to_string(delta_t) + "' ns.");
            }
        }

        GR_LOG_WARN(d_logger, "No WR-Tag found for trigger tag. This should not happen.");
        // all events (if any) were to old ... seems like something went wrong
        trigger_tag_data.status |= channel_status_t::CHANNEL_STATUS_TIMEOUT_WAITING_WR_OR_REALIGNMENT_EVENT;

        //std::cout << "queuesize: " << d_pending_events.size() << std::endl;
        //std::cout << "end search" << std::endl;
    }

    void
    time_realignment_ff_impl::push_wr_tag(const wr_event_t &event)
    {
        if (d_pending_events.size() > NUMBER_OF_PENDING_TRIGGERS_WARNING && d_pending_events.size() % NUMBER_OF_PENDING_TRIGGERS_WARNING == 0) {
          GR_LOG_WARN(d_logger, d_name + ": " + std::to_string(d_pending_events.size()) + " pending WR events detected (possibly WR-Trigger cable missing ?) Make sure to configure flowgraph such that the same number of triggers & WR events is generated");
        }
      if (d_pending_events.size() > NUMBER_OF_PENDING_TRIGGERS_ERROR) {
        GR_LOG_ERROR(d_logger, "Large number of pending WR events detected (possibly WR-Trigger cable missing ?) Make sure to configure flowgraph such that the same number of triggers & WR events is generated");
        d_pending_events.clear();
      }
      //std::cout << "Pushed WR Tag: " << std::endl;
      d_pending_events.push_back(event);
    }

    int64_t
    time_realignment_ff_impl::get_user_delay_ns() const
    {
      return static_cast<int64_t>(d_user_delay * 1000000000.0);
    }

  } /* namespace digitizers */
} /* namespace gr */

