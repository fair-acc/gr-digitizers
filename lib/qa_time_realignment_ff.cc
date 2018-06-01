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
#include "utils.h"
#include <digitizers/tags.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/blocks/vector_sink_f.h>
#include <gnuradio/top_block.h>
#include <algorithm>

namespace gr {
  namespace digitizers {

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

      realignment_test_flowgraph_t (float samp_rate, float user_delay,
              const std::vector<tag_t> &tags, size_t data_size=33333)
      {
        values = make_test_data(data_size);
        errors = make_test_data(data_size, 0.2);

        top = gr::make_top_block("test");
        value_src = gr::blocks::vector_source_f::make(values, false, 1, tags);
        error_src = gr::blocks::vector_source_f::make(errors);
        realign = gr::digitizers::time_realignment_ff::make(samp_rate, user_delay);
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
        CPPUNIT_ASSERT_EQUAL(l.offset, r.offset);
    }

    // There should be no side effect if timing is not provided
    void
    qa_time_realignment_ff::no_timing()
    {
      float user_delay = .1234;

      acq_info_t tag;
      tag.timestamp = 654321;
      tag.actual_delay = 0.223;
      tag.user_delay = -0.2;
      tag.status = 0x123;
      tag.offset = 100;
      tag.last_beam_in_timestamp = -1;

      std::vector<gr::tag_t> tags = {
        make_acq_info_tag(tag)
      };

      realignment_test_flowgraph_t flowgraph(10000/*samp_rate*/, user_delay, tags);
      flowgraph.run();
      flowgraph.verify_values();

      auto out_tags = flowgraph.tags();
      CPPUNIT_ASSERT_EQUAL(1, (int)out_tags.size());

      auto acq_info = decode_acq_info_tag(out_tags.at(0));
      compare_static_fields(tag, acq_info);

      CPPUNIT_ASSERT_DOUBLES_EQUAL(user_delay + tag.user_delay, acq_info.user_delay, 1E-9);
      CPPUNIT_ASSERT_DOUBLES_EQUAL(user_delay + tag.actual_delay, acq_info.actual_delay, 1E-9);
      CPPUNIT_ASSERT_DOUBLES_EQUAL(
              (double)tag.timestamp + (user_delay * 1000000000.0),
              (double)acq_info.timestamp, 1.0);
    }

    void
    qa_time_realignment_ff::synchronization()
    {
      float user_delay = .1234;
      float samp_rate = 20000000.0;
      int acq_info_offset = 6000;

      acq_info_t tag;
      tag.timestamp = 654321;
      tag.actual_delay = 0.223;
      tag.user_delay = -0.2;
      tag.status = 0x123;
      tag.offset = acq_info_offset;
      tag.last_beam_in_timestamp = -1;

      wr_event_t event;
      event.event_id = "made_up";
      event.timestamp = 987654321;
      event.last_beam_in_timestamp = 123456789;
      event.offset = 0;
      event.time_sync_only = true;
      event.realignment_required = false;

      std::vector<gr::tag_t> tags = {
        make_trigger_tag(0),
        make_wr_event_tag(event),
        make_acq_info_tag(tag)
      };

      realignment_test_flowgraph_t flowgraph(samp_rate, user_delay, tags);

      flowgraph.run();
      flowgraph.verify_values();

      auto out_tags = flowgraph.tags();
      CPPUNIT_ASSERT_EQUAL(3, (int)out_tags.size());

      auto received_event = decode_wr_event_tag(out_tags.at(1));
      CPPUNIT_ASSERT_EQUAL(event.event_id, received_event.event_id);
      CPPUNIT_ASSERT_EQUAL(event.timestamp, received_event.timestamp);
      CPPUNIT_ASSERT_EQUAL(event.last_beam_in_timestamp, received_event.last_beam_in_timestamp);
      CPPUNIT_ASSERT_EQUAL(event.offset, received_event.offset);
      CPPUNIT_ASSERT_EQUAL(event.time_sync_only, received_event.time_sync_only);
      CPPUNIT_ASSERT_EQUAL(event.realignment_required, received_event.realignment_required);

      auto acq_info = decode_acq_info_tag(out_tags.at(2));
      compare_static_fields(tag, acq_info);

      CPPUNIT_ASSERT_DOUBLES_EQUAL(user_delay + tag.user_delay, acq_info.user_delay, 1E-9);
      CPPUNIT_ASSERT_DOUBLES_EQUAL(user_delay + tag.actual_delay, acq_info.actual_delay, 1E-9);
      CPPUNIT_ASSERT_DOUBLES_EQUAL(
              987654321.0 + (user_delay * 1000000000.0) + ((acq_info_offset / samp_rate) * 1000000000.0),
              (double)acq_info.timestamp, 1.0);
      CPPUNIT_ASSERT_EQUAL((int64_t)123456789, acq_info.last_beam_in_timestamp);
    }
  } /* namespace digitizers */
} /* namespace gr */

