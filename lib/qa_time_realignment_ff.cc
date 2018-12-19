/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#include <gnuradio/attributes.h>
#include <cppunit/TestAssert.h>
#include "qa_common.h"
#include "qa_time_realignment_ff.h"
#include <digitizers/time_realignment_ff.h>
#include <digitizers/status.h>
#include "utils.h"
#include <digitizers/tags.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/blocks/vector_sink_f.h>
#include <gnuradio/top_block.h>
#include <algorithm>

namespace gr {
  namespace digitizers {

    struct sim_wr_event_t
    {
        std::string event_id;
        int64_t wr_trigger_stamp;
        int64_t wr_trigger_stamp_utc;
    };
    struct realignment_test_flowgraph_t
    {
      gr::top_block_sptr top;
      gr::blocks::vector_source_f::sptr value_src;
      gr::blocks::vector_source_f::sptr error_src;
      gr::digitizers::time_realignment_ff::sptr realign;
      gr::blocks::vector_sink_f::sptr value_sink;
      gr::blocks::vector_sink_f::sptr error_sink;
      std::vector<float> values;
      std::vector<float> errors;

      realignment_test_flowgraph_t (float user_delay, float triggerstamp_matching_tolerance, const std::vector<tag_t> &tags, const std::vector<sim_wr_event_t> &wr_events, size_t data_size=33333)
      {
        values = make_test_data(data_size);
        errors = make_test_data(data_size, 0.2);

        top = gr::make_top_block("test");
        value_src = gr::blocks::vector_source_f::make(values, false, 1, tags);
        error_src = gr::blocks::vector_source_f::make(errors);
        realign = gr::digitizers::time_realignment_ff::make(user_delay, triggerstamp_matching_tolerance);
        for(auto event : wr_events)
            realign->add_timing_event(event.event_id, event.wr_trigger_stamp, event.wr_trigger_stamp_utc);

        value_sink = gr::blocks::vector_sink_f::make();
        error_sink = gr::blocks::vector_sink_f::make();

        top->connect(value_src, 0, realign, 0);
        top->connect(error_src, 0, realign, 1);

        top->connect(realign, 0, value_sink, 0);
        top->connect(realign, 1, error_sink, 0);
      }

      void
      run()
      {
        top->run();
      }

      void
      verify_values()
      {
        auto actual_values = value_sink->data();
        auto actual_errors = error_sink->data();

        CPPUNIT_ASSERT_EQUAL(values.size(), actual_values.size());
        CPPUNIT_ASSERT_EQUAL(errors.size(), actual_errors.size());

        ASSERT_VECTOR_EQUAL(values.begin(), values.end(), actual_values.begin());
        ASSERT_VECTOR_EQUAL(errors.begin(), errors.end(), actual_errors.begin());
      }

      std::vector<gr::tag_t>
      tags()
      {
        return value_sink->tags();
      }
    };

    // Compares acq_info fields that shouldn't be touched by the realignment block
    static void
    compare_static_fields(const acq_info_t &l, const acq_info_t &r)
    {
        CPPUNIT_ASSERT_EQUAL(l.status, r.status);
    }

    void
    qa_time_realignment_ff::default_case()
    {
      float user_delay = .1234;
      float timeout = 0.01;

      trigger_t tag1;
      tag1.timestamp = 70;
      tag1.status = 0x00;

      trigger_t tag2;
      tag2.timestamp = 175;
      tag2.status = 0x00;

      std::vector<sim_wr_event_t> wr_events;
      sim_wr_event_t wr_event1;
      wr_event1.event_id = "first";
      wr_event1.wr_trigger_stamp = 100;
      wr_event1.wr_trigger_stamp_utc = 65;
      wr_events.push_back(wr_event1);

      sim_wr_event_t wr_event2;
      wr_event2.event_id = "second";
      wr_event2.wr_trigger_stamp = 200;
      wr_event2.wr_trigger_stamp_utc = 170;
      wr_events.push_back(wr_event2);

      std::vector<gr::tag_t> tags = {
        make_trigger_tag(tag1,6000),
        make_trigger_tag(tag2,8000),
      };

      realignment_test_flowgraph_t flowgraph(user_delay, timeout, tags, wr_events );

      flowgraph.run();
      flowgraph.verify_values();

      auto out_tags = flowgraph.tags();
      CPPUNIT_ASSERT_EQUAL(2, (int)out_tags.size()); // wr-tags are not forwarded

      CPPUNIT_ASSERT_EQUAL(out_tags[0].key, pmt::string_to_symbol(trigger_tag_name));
      CPPUNIT_ASSERT_EQUAL(out_tags[1].key, pmt::string_to_symbol(trigger_tag_name));

      trigger_t trigger_tag_data1 = decode_trigger_tag(out_tags.at(0));
      trigger_t trigger_tag_data2 = decode_trigger_tag(out_tags.at(1));

      CPPUNIT_ASSERT_EQUAL(wr_event1.wr_trigger_stamp, trigger_tag_data1.timestamp);
      CPPUNIT_ASSERT_EQUAL(wr_event2.wr_trigger_stamp, trigger_tag_data2.timestamp);

      CPPUNIT_ASSERT_EQUAL(trigger_tag_data1.status, tag1.status);
      CPPUNIT_ASSERT_EQUAL(trigger_tag_data2.status, tag2.status);
    }

    // There should be no side effect if timing is not provided
    void
    qa_time_realignment_ff::no_timing()
    {
      float user_delay = .1234;
      float timeout = 0.01;

      acq_info_t tag;
      tag.timestamp = 654321;
      tag.actual_delay = 0.223;
      tag.user_delay = -0.2;
      tag.status = 0x123;

      std::vector<sim_wr_event_t> wr_events;

      std::vector<gr::tag_t> tags = {
        make_acq_info_tag(tag,100)
      };

      realignment_test_flowgraph_t flowgraph(user_delay, timeout, tags, wr_events);
      flowgraph.run();
      flowgraph.verify_values();

      auto out_tags = flowgraph.tags();
      CPPUNIT_ASSERT_EQUAL(1, (int)out_tags.size());

      auto acq_info = decode_acq_info_tag(out_tags.at(0));
      compare_static_fields(tag, acq_info);

      // FIXME: Not clear yet what should be the purpose of user_delay
      //CPPUNIT_ASSERT_DOUBLES_EQUAL(user_delay + tag.user_delay, acq_info.user_delay, 1E-9);
      //CPPUNIT_ASSERT_DOUBLES_EQUAL(user_delay + tag.actual_delay, acq_info.actual_delay, 1E-9);
      //CPPUNIT_ASSERT_DOUBLES_EQUAL( (double)tag.timestamp + (user_delay * 1000000000.0), double)acq_info.timestamp, 1.0);
    }

    void
    qa_time_realignment_ff::no_wr_events()
    {
        float user_delay = .1234;
        float timeout = 0.01;

        trigger_t tag1;
        tag1.timestamp = 100;
        tag1.status = 0x00;

        std::vector<sim_wr_event_t> wr_events;

        std::vector<gr::tag_t> tags = {
          make_trigger_tag(tag1,10000)
        };

        realignment_test_flowgraph_t flowgraph(user_delay, timeout, tags, wr_events);

        flowgraph.run();
        flowgraph.verify_values();

        auto out_tags = flowgraph.tags();
        CPPUNIT_ASSERT_EQUAL(1, (int)out_tags.size());
        CPPUNIT_ASSERT_EQUAL(out_tags[0].key, pmt::string_to_symbol(trigger_tag_name));
        trigger_t trigger_tag_data1 = decode_trigger_tag(out_tags.at(0));
        CPPUNIT_ASSERT_EQUAL(trigger_tag_data1.status, uint32_t(channel_status_t::CHANNEL_STATUS_TIMEOUT_WAITING_WR_OR_REALIGNMENT_EVENT));
    }

    void
    qa_time_realignment_ff::out_of_tolerance_1()
    {
        float user_delay = .1234;
        float triggerstamp_matching_tolerance = 1. / 100000000; //10ns

        trigger_t tag1;
        tag1.timestamp = 90;
        tag1.status = 0x00;

        std::vector<sim_wr_event_t> wr_events;
        sim_wr_event_t wr_event1;
        wr_event1.event_id = "first";
        wr_event1.wr_trigger_stamp = 100;
        wr_event1.wr_trigger_stamp_utc = 70; // off by -20ns
        wr_events.push_back(wr_event1);

        std::vector<gr::tag_t> tags = {
          make_trigger_tag(tag1,10000),
        };

        realignment_test_flowgraph_t flowgraph(user_delay, triggerstamp_matching_tolerance, tags, wr_events);

        flowgraph.run();
        flowgraph.verify_values();

        auto out_tags = flowgraph.tags();
        CPPUNIT_ASSERT_EQUAL(1, (int)out_tags.size());
        CPPUNIT_ASSERT_EQUAL(out_tags[0].key, pmt::string_to_symbol(trigger_tag_name));
        trigger_t trigger_tag_data1 = decode_trigger_tag(out_tags.at(0));
        CPPUNIT_ASSERT_EQUAL(trigger_tag_data1.status, uint32_t(channel_status_t::CHANNEL_STATUS_TIMEOUT_WAITING_WR_OR_REALIGNMENT_EVENT));
    }

    void
    qa_time_realignment_ff::out_of_tolerance_2()
    {
        float user_delay = .1234;
        float triggerstamp_matching_tolerance = 1. / 100000000; //10ns

        trigger_t tag1;
        tag1.timestamp = 90;
        tag1.status = 0x00;

        std::vector<sim_wr_event_t> wr_events;
        sim_wr_event_t wr_event1;
        wr_event1.event_id = "first";
        wr_event1.wr_trigger_stamp = 100;
        wr_event1.wr_trigger_stamp_utc = 130; // off by +20ns
        wr_events.push_back(wr_event1);

        std::vector<gr::tag_t> tags = {
          make_trigger_tag(tag1,10000),
        };

        realignment_test_flowgraph_t flowgraph(user_delay, triggerstamp_matching_tolerance, tags,wr_events);

        flowgraph.run();
        flowgraph.verify_values();

        auto out_tags = flowgraph.tags();
        CPPUNIT_ASSERT_EQUAL(1, (int)out_tags.size());
        CPPUNIT_ASSERT_EQUAL(out_tags[0].key, pmt::string_to_symbol(trigger_tag_name));
        trigger_t trigger_tag_data1 = decode_trigger_tag(out_tags.at(0));
        CPPUNIT_ASSERT_EQUAL(trigger_tag_data1.status, uint32_t(channel_status_t::CHANNEL_STATUS_TIMEOUT_WAITING_WR_OR_REALIGNMENT_EVENT));
    }

  } /* namespace digitizers */
} /* namespace gr */

