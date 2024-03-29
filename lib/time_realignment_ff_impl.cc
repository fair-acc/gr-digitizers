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
    time_realignment_ff::make(const std::string id, float user_delay, float triggerstamp_matching_tolerance, float max_buffer_time)
    {
      return gnuradio::get_initial_sptr
        (new time_realignment_ff_impl(id, user_delay, triggerstamp_matching_tolerance, max_buffer_time));
    }

    /*
     * The private constructor
     */
    time_realignment_ff_impl::time_realignment_ff_impl(const std::string id, float user_delay, float triggerstamp_matching_tolerance, float max_buffer_time)
      : gr::block(id,
                  gr::io_signature::make(2, 3, sizeof(float)),
                  gr::io_signature::make(2, 2, sizeof(float))),
                  d_user_delay(user_delay),
                  d_wr_events_size(10) // Maximum buffer of 10 WR-Events
    {
      wr_event_t empty;
      for( size_t i = 0; i< d_wr_events_size; i++)
          d_wr_events.push_back(empty);
      d_wr_events_write_iter = d_wr_events.begin();
      d_wr_events_read_iter = d_wr_events.begin();
      d_not_found_stamp_utc = 0;
      set_triggerstamp_matching_tolerance(triggerstamp_matching_tolerance);
      set_max_buffer_time(max_buffer_time);
      // FIXME: Currently time_realiggment does nothing !!
      set_tag_propagation_policy(tag_propagation_policy_t::TPP_ONE_TO_ONE);
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
        return float(d_triggerstamp_matching_tolerance_ns / 1000000000.);
    }

    void time_realignment_ff_impl::set_max_buffer_time(float max_buffer_time)
    {
        d_max_buffer_time_ns = int64_t(max_buffer_time * 1000000000 );
    }

    float time_realignment_ff_impl::get_max_buffer_time()
    {
        return float(d_max_buffer_time_ns / 1000000000.);
    }

    int time_realignment_ff_impl::general_work(int noutput_items,
         gr_vector_int &ninput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items)
    {
// FIXME: Currently time_realiggment does nothing !!
      uint64_t ninput_items_min;
 //     uint64_t sample_to_start_processing_abs;
      bool errors_connected = input_items.size() > 1 && output_items.size() > 1;
      if (errors_connected)
      {
          ninput_items_min = std::min( ninput_items[0], ninput_items[1]);
//          sample_to_start_processing_abs = std::min( nitems_read(0), nitems_read(1));
      }
      else
      {
          ninput_items_min = ninput_items[0];
 //         sample_to_start_processing_abs = nitems_read(0);
      }
      int64_t copy_data_len = std::min( int(noutput_items), int(ninput_items_min));
//      uint64_t max_sample_to_end_processing_abs = sample_to_start_processing_abs + copy_data_len;
//
//      // Get all the tags, for performance reason a member variable is used
//      std::vector<gr::tag_t> tags;
//      get_tags_in_range(tags, 0, sample_to_start_processing_abs, max_sample_to_end_processing_abs);
//      for (auto tag: tags)
//      {
//        if (tag.key == pmt::string_to_symbol(trigger_tag_name))
//        {
//          //std::cout << "trigger tag incoming. Offset: " << tag.offset << std::endl;
//          //std::cout << "Trigger Tag incoming : " << get_timestamp_milli_utc() << std::endl;
//          trigger_t trigger_tag_data = decode_trigger_tag(tag);
//          //std::cout << "Trigger Stamp        : " << trigger_tag_data.timestamp / 1000000 <<  " ms" << std::endl;
//          if(fill_wr_stamp(trigger_tag_data))
//          {
//              add_item_tag(0, make_trigger_tag(trigger_tag_data,tag.offset)); // add tag to port 0
//          }
//          else
//          {
//              //std::cout << "tag.offset: " << tag.offset << std::endl;
//              //std::cout << "sample_to_start_processing_abs: " << sample_to_start_processing_abs << std::endl;
//
//              // No WR-Stamp availabe yet. Keep data on the input queue and leave. Better luck on next iteration
//              copy_data_len = tag.offset - sample_to_start_processing_abs - 1; // only copy all data before the tag
//              if(copy_data_len <= 0) // nothing more to do
//              {
//                  consume(0, 0);
//                  if (errors_connected)
//                      consume(1, 0);
//                  return 0;
//              }
//              //std::cout << "tag.offset: " << tag.offset << std::endl;
//              //std::cout << "sample_to_start_processing_abs: " << sample_to_start_processing_abs << std::endl;
//              break;
//          }
//        }
//        else
//        {
//            //std::cout << "unknown tag incoming" << std::endl;
//          add_item_tag(0, tag); // forward all others by default
//        }
//      }

      //std::cout << "memcpy: copy_data_len: " << copy_data_len << std::endl;

      //copy data
      memcpy(output_items.at(0), input_items.at(0), copy_data_len * sizeof(float));
      if (errors_connected)
        memcpy(output_items.at(1), input_items.at(1), copy_data_len * sizeof(float));

      //empty input queues
      consume(0, copy_data_len);
      if (errors_connected)
          consume(1, copy_data_len);
      // Tell runtime system how many output items we produced.
      return copy_data_len;
    }

    bool
    time_realignment_ff_impl::fill_wr_stamp(trigger_t &trigger_tag_data)
    {
        // we dont have a wr-event for this trigger tag yet
        if(d_wr_events_write_iter->wr_trigger_stamp == d_wr_events_read_iter->wr_trigger_stamp)
        {
            if( d_not_found_stamp_utc == 0)
                d_not_found_stamp_utc = get_timestamp_nano_utc();

            if( d_not_found_stamp_utc != 0 && abs( get_timestamp_nano_utc() - d_not_found_stamp_utc ) > d_max_buffer_time_ns )
            {
                d_not_found_stamp_utc = 0; //reset stamp
                GR_LOG_ERROR(d_logger, name() + ": No WR-Tag found for trigger tag after waiting " + std::to_string(get_max_buffer_time())+ "s. Trigger will be forwarded without realligment. Possibly max_buffer_time needs to be adjusted." );
                trigger_tag_data.status |= channel_status_t::CHANNEL_STATUS_TIMEOUT_WAITING_WR_OR_REALIGNMENT_EVENT;
                return true;
            }

            // this may happen often.. possibly better to dont print a warning
            //GR_LOG_WARN(d_logger, "We dont have a wr-event for this trigger tag yet ... buffering will be used");
            return false; // all trigger and samples before this trigger will be kept on input, better luck on the next work call
        }

        while(true)
        {
            int64_t delta_t = abs ( trigger_tag_data.timestamp - d_wr_events_read_iter->wr_trigger_stamp_utc );
            if(  delta_t > d_triggerstamp_matching_tolerance_ns )
            {
                GR_LOG_WARN(d_logger, name() + ": WR Stamps was out of matching tolerance by " + std::to_string(delta_t/1000)+ "µs. Will be ignored");
                GR_LOG_WARN(d_logger, name() + ": trigger_tag_data.timestamp                  " + std::to_string(trigger_tag_data.timestamp / 1000000)  + "ms");
                GR_LOG_WARN(d_logger, name() + ": d_wr_events_read_iter->wr_trigger_stamp_utc " + std::to_string(d_wr_events_read_iter->wr_trigger_stamp_utc /1000000) + "ms");

//                {
//                    time_t wrStampSeconds = trigger_tag_data.timestamp / 1000000000;
//                    struct tm * timeinfo;
//                    timeinfo = localtime (&wrStampSeconds);
//                    printf ("trigger_tag_data.timestamp: %s", asctime(timeinfo));
//                }
//                {
//                    time_t wrStampSeconds = d_wr_events_read_iter->wr_trigger_stamp_utc / 1000000000;
//                    struct tm * timeinfo;
//                    timeinfo = localtime (&wrStampSeconds);
//                    printf ("d_wr_events_read_iter->wr_trigger_stamp_utc: %s", asctime(timeinfo));
//                }
                trigger_tag_data.status |= channel_status_t::CHANNEL_STATUS_TIMEOUT_WAITING_WR_OR_REALIGNMENT_EVENT;
                d_wr_events_read_iter++;
                if(d_wr_events_read_iter == d_wr_events.end())
                    d_wr_events_read_iter = d_wr_events.begin();
                if(d_wr_events_write_iter->wr_trigger_stamp == d_wr_events_read_iter->wr_trigger_stamp)
                    return true; // Forward the trigger tag with bad status .. for some reason it did not match any of our wr-events
            }
            else
            {
                d_not_found_stamp_utc = 0; // reset stamp
                //std::cout << "delta_t [sec]                      : " << delta_t/1000000000.f << std::endl;
                trigger_tag_data.timestamp = d_wr_events_read_iter->wr_trigger_stamp;
                d_wr_events_read_iter++;
                if(d_wr_events_read_iter == d_wr_events.end())
                    d_wr_events_read_iter = d_wr_events.begin();
                return true;
            }
        }
    }

    int64_t
    time_realignment_ff_impl::get_user_delay_ns() const
    {
      return static_cast<int64_t>(d_user_delay * 1000000000.0);
    }

    bool
    time_realignment_ff_impl::add_timing_event(const std::string &event_id, int64_t wr_trigger_stamp, int64_t wr_trigger_stamp_utc)
    {
      d_wr_events_write_iter->event_id = event_id;
      d_wr_events_write_iter->wr_trigger_stamp = wr_trigger_stamp;
      d_wr_events_write_iter->wr_trigger_stamp_utc = wr_trigger_stamp_utc;
      d_wr_events_write_iter++;
      if(d_wr_events_write_iter == d_wr_events.end())
          d_wr_events_write_iter = d_wr_events.begin();
      if(d_wr_events_write_iter->wr_trigger_stamp == d_wr_events_read_iter->wr_trigger_stamp)
      {
          GR_LOG_ERROR(d_logger, name() + ": Write Iter reached read iter ...to few trigger tags");
          return false;
      }
      return true;

      //std::cout << "WR-Event Processing 5: " << get_timestamp_milli_utc() << std::endl;
    }



  } /* namespace digitizers */
} /* namespace gr */

