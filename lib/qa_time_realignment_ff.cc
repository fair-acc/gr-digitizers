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

      realignment_test_flowgraph_t (float user_delay, bool timing_available,
              const std::vector<tag_t> &tags, size_t data_size=33333)
      {
        values = make_test_data(data_size);
        errors = make_test_data(data_size, 0.2);

        top = gr::make_top_block("test");
        value_src = gr::blocks::vector_source_f::make(values, false, 1, tags);
        error_src = gr::blocks::vector_source_f::make(errors);
        realign = gr::digitizers::time_realignment_ff::make(user_delay, timing_available);
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

        assert_float_vector(values.begin(), values.end(), actual_values.begin());
        assert_float_vector(errors.begin(), errors.end(), actual_errors.begin());
      }

      std::vector<gr::tag_t>
      tags()
      {
        return value_sink->tags();
      }
    };

    // Compares acq_info fields that shouldn't be touched by realignment block
    static void
    compare_static_fields(const acq_info_t &l, const acq_info_t &r)
    {
        CPPUNIT_ASSERT_EQUAL(l.timebase, r.timebase);
        CPPUNIT_ASSERT_EQUAL(l.pre_samples, r.pre_samples);
        CPPUNIT_ASSERT_EQUAL(l.samples, r.samples);
        CPPUNIT_ASSERT_EQUAL(l.status, r.status);
        CPPUNIT_ASSERT_EQUAL(l.offset, r.offset);
        CPPUNIT_ASSERT_EQUAL(l.triggered_data, r.triggered_data);
    }

    // There should be no side effect if timing is not provided
    void
    qa_time_realignment_ff::no_timing()
    {
      float user_delay = .1234;

      acq_info_t tag;
      tag.timestamp = 654321;
      tag.trigger_timestamp = -1;
      tag.timebase = 0.000001;
      tag.actual_delay = 0.223;
      tag.user_delay = -0.2;
      tag.pre_samples = 2000;
      tag.samples = 300;
      tag.status = 0x123;
      tag.offset = 100;
      tag.triggered_data = true;
      tag.last_beam_in_timestamp = -1;

      std::vector<gr::tag_t> tags = {
        make_acq_info_tag(tag)
      };

      realignment_test_flowgraph_t flowgraph(user_delay, false, tags);
      flowgraph.run();
      flowgraph.verify_values();

      auto out_tags = flowgraph.tags();
      CPPUNIT_ASSERT_EQUAL(1, (int)out_tags.size());

      auto acq_info = decode_acq_info_tag(out_tags.at(0));
      compare_static_fields(tag, acq_info);

      CPPUNIT_ASSERT_DOUBLES_EQUAL(user_delay + tag.user_delay, acq_info.user_delay, 1E-9);
      CPPUNIT_ASSERT_DOUBLES_EQUAL(user_delay + tag.actual_delay, acq_info.actual_delay, 1E-9);
      CPPUNIT_ASSERT_EQUAL(tag.trigger_timestamp, acq_info.trigger_timestamp);
      CPPUNIT_ASSERT_DOUBLES_EQUAL(
              (double)tag.timestamp - (user_delay * 1000000000.0),
              (double)acq_info.timestamp, 1.0);
    }

    void
    qa_time_realignment_ff::realignment()
    {
      float user_delay = .1234;

      acq_info_t tag;
      tag.timestamp = 654321;
      tag.trigger_timestamp = -1;
      tag.timebase = 0.000001;
      tag.actual_delay = 0.223;
      tag.user_delay = -0.2;
      tag.pre_samples = 2000;
      tag.samples = 300;
      tag.status = 0x123;
      tag.offset = 100;
      tag.triggered_data = true;
      tag.last_beam_in_timestamp = -1;

      std::vector<gr::tag_t> tags = {
        make_acq_info_tag(tag)
      };

      realignment_test_flowgraph_t flowgraph(user_delay, true, tags);
      flowgraph.realign->add_timing_event("foo", 987654321, 123456789, false, false);
      flowgraph.run();
      flowgraph.verify_values();

      auto out_tags = flowgraph.tags();
      CPPUNIT_ASSERT_EQUAL(1, (int)out_tags.size());

      auto acq_info = decode_acq_info_tag(out_tags.at(0));
      compare_static_fields(tag, acq_info);

      CPPUNIT_ASSERT_DOUBLES_EQUAL(user_delay + tag.user_delay, acq_info.user_delay, 1E-9);
      CPPUNIT_ASSERT_DOUBLES_EQUAL(user_delay + tag.actual_delay, acq_info.actual_delay, 1E-9);
      CPPUNIT_ASSERT_EQUAL((int64_t)987654321, acq_info.trigger_timestamp);
      CPPUNIT_ASSERT_DOUBLES_EQUAL(
              987654321.0 - (user_delay * 1000000000.0),
              (double)acq_info.timestamp, 1.0);
      CPPUNIT_ASSERT_EQUAL((int64_t)123456789, acq_info.last_beam_in_timestamp);
    }

    void
    qa_time_realignment_ff::synchronization()
    {
      float user_delay = .1234;

      acq_info_t tag;
      tag.timestamp = -1;
      tag.trigger_timestamp = -1;
      tag.timebase = 0.000001;
      tag.actual_delay = tag.user_delay = 0.002;
      tag.pre_samples = 200;
      tag.samples = 300;
      tag.status = 0x123;
      tag.offset = 100;
      tag.triggered_data = true;
      tag.last_beam_in_timestamp = -1;

      acq_info_t tag2 = tag;
      tag2.offset = 1200;
      tag2.triggered_data = false;

      std::vector<gr::tag_t> tags = {
        make_acq_info_tag(tag),
        make_acq_info_tag(tag2)
      };

      realignment_test_flowgraph_t flowgraph(user_delay, true, tags);
      flowgraph.realign->add_timing_event("foo", 987654321, 123456789, false, false);
      flowgraph.run();
      flowgraph.verify_values();

      auto out_tags = flowgraph.tags();
      CPPUNIT_ASSERT_EQUAL(2, (int)out_tags.size());

      auto acq_info = decode_acq_info_tag(out_tags.at(0));
      compare_static_fields(tag, acq_info);
      CPPUNIT_ASSERT_EQUAL((int64_t)987654321, acq_info.trigger_timestamp);
      CPPUNIT_ASSERT_DOUBLES_EQUAL(
              987654321.0 - (user_delay * 1000000000.0),
              (double)acq_info.timestamp, 1.0);
      CPPUNIT_ASSERT_EQUAL((int64_t)123456789, acq_info.last_beam_in_timestamp);

      auto acq_info2 = decode_acq_info_tag(out_tags.at(1));
      compare_static_fields(tag2, acq_info2);
      CPPUNIT_ASSERT_EQUAL((int64_t)-1, acq_info2.trigger_timestamp);
      CPPUNIT_ASSERT_EQUAL((int64_t)123456789, acq_info2.last_beam_in_timestamp);

      auto timestamp = acq_info.timestamp
              + static_cast<int64_t>((double)(acq_info2.offset - acq_info.offset)
                      * acq_info2.timebase * 1000000000.0);

      CPPUNIT_ASSERT_DOUBLES_EQUAL((double)timestamp, (double)acq_info2.timestamp, 2.0);
    }

  } /* namespace digitizers */
} /* namespace gr */

