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


#include <gnuradio/attributes.h>
#include <cppunit/TestAssert.h>
#include "qa_extractor.h"
#include "utils.h"
#include "qa_common.h"
#include <digitizers/extractor.h>
#include <digitizers/status.h>
#include <digitizers/tags.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/blocks/vector_sink_f.h>
#include <gnuradio/top_block.h>
#include <boost/shared_ptr.hpp>
#include <algorithm>


namespace gr {
 namespace digitizers {

    struct extractor_test_flowgraph_t
    {
      gr::top_block_sptr top;
      gr::blocks::vector_source_f::sptr value_src;
      gr::blocks::vector_source_f::sptr error_src;
      gr::digitizers::extractor::sptr extractor;
      gr::blocks::vector_sink_f::sptr value_sink;
      gr::blocks::vector_sink_f::sptr error_sink;

      void
      run()
      {
        top->run();
      }

      void
      reset()
      {
        value_src->rewind();
        error_src->rewind();
        value_sink->reset();
        error_sink->reset();
      }

      std::vector<float>
      actual_values()
      {
        return value_sink->data();
      }

      std::vector<float>
      actual_errors()
      {
        return error_sink->data();
      }

      std::vector<gr::tag_t>
      tags()
      {
        return value_sink->tags();
      }
    };

    static boost::shared_ptr<extractor_test_flowgraph_t>
    make_test_flowgraph(const std::vector<float> &values, const std::vector<float> &errors,
            float pre_trigger_window, float post_trigger_window)
    {
      boost::shared_ptr<extractor_test_flowgraph_t> flowgraph{new extractor_test_flowgraph_t{}};

      flowgraph->top = gr::make_top_block("test");
      flowgraph->value_src = gr::blocks::vector_source_f::make(values);
      flowgraph->error_src = gr::blocks::vector_source_f::make(errors);
      flowgraph->extractor = gr::digitizers::extractor::make(post_trigger_window, pre_trigger_window);
      flowgraph->value_sink = gr::blocks::vector_sink_f::make();
      flowgraph->error_sink = gr::blocks::vector_sink_f::make();

      flowgraph->top->connect(flowgraph->value_src, 0, flowgraph->extractor, 0);
      flowgraph->top->connect(flowgraph->error_src, 0, flowgraph->extractor, 1);

      flowgraph->top->connect(flowgraph->extractor, 0, flowgraph->value_sink, 0);
      flowgraph->top->connect(flowgraph->extractor, 1, flowgraph->error_sink, 0);

      return flowgraph;
    }

    void
    qa_extractor::test_no_trigger()
    {
      size_t data_size = 5000000;
      auto values = make_test_data(data_size);
      auto errors = make_test_data(data_size, 0.1);
      auto flowgraph = make_test_flowgraph(values, errors, 0.5, 0.5);

      flowgraph->run();

      auto actual_values = flowgraph->actual_values();
      auto actual_errors = flowgraph->actual_errors();

      // no trigger is present therefore we expect no data on outputs
      CPPUNIT_ASSERT_EQUAL(0, (int)actual_values.size());
      CPPUNIT_ASSERT_EQUAL(0, (int)actual_errors.size());
    }

    void
    qa_extractor::test_single_trigger()
    {
      float timebase = 0.00001;                      // seconds
      float pre_trigger = 0.001, post_trigger = 0.1; // seconds

      uint32_t pre_trigger_samples = (uint32_t)(pre_trigger / timebase);
      uint32_t post_trigger_samples = (uint32_t)(post_trigger / timebase);

      size_t data_size = (pre_trigger_samples + post_trigger_samples) * 10;

      auto values = make_test_data(data_size);
      auto errors = make_test_data(data_size, 0.1);
      auto flowgraph = make_test_flowgraph(values, errors, pre_trigger, post_trigger);

      auto trigger_offset = 121;

      std::vector<gr::tag_t> tags = {
        make_trigger_tag(pre_trigger_samples, post_trigger_samples, 0, timebase)
      };
      tags[0].offset = trigger_offset;

      flowgraph->value_src->set_data(values, tags);
      flowgraph->run();

      auto actual_values = flowgraph->actual_values();
      auto actual_errors = flowgraph->actual_errors();

      auto collected_samples = pre_trigger_samples + post_trigger_samples;
      CPPUNIT_ASSERT_EQUAL(collected_samples, (uint32_t)actual_values.size());
      CPPUNIT_ASSERT_EQUAL(collected_samples, (uint32_t)actual_errors.size());

      assert_float_vector(values.begin() + trigger_offset, // see make_triggered_data_tag
              values.begin() + trigger_offset + collected_samples, actual_values.begin());
      assert_float_vector(errors.begin() + trigger_offset,
              errors.begin() + trigger_offset + collected_samples, actual_errors.begin());

      auto out_tags = flowgraph->tags();
      CPPUNIT_ASSERT_EQUAL(1, (int)out_tags.size());

      auto acq_info = decode_acq_info_tag(out_tags[0]);
      CPPUNIT_ASSERT_EQUAL(pre_trigger_samples, acq_info.pre_samples);
      CPPUNIT_ASSERT_EQUAL(post_trigger_samples, acq_info.samples);
      CPPUNIT_ASSERT_EQUAL(timebase, (float)acq_info.timebase);
    }

    void
    qa_extractor::test_single_trigger_no_pre()
    {
      float timebase = 0.00001;                      // seconds
      float pre_trigger = 0.0, post_trigger = 0.1;   // seconds

      uint32_t pre_trigger_samples = 5000;
      uint32_t post_trigger_samples = (uint32_t)(post_trigger / timebase);

      size_t data_size = pre_trigger_samples + post_trigger_samples;

      auto values = make_test_data(data_size);
      auto errors = make_test_data(data_size, 0.1);
      auto flowgraph = make_test_flowgraph(values, errors, pre_trigger, post_trigger);

      std::vector<gr::tag_t> tags = {
        make_trigger_tag(pre_trigger_samples, post_trigger_samples, 0, timebase)
      };
      tags[0].offset = 0;

      flowgraph->value_src->set_data(values, tags);
      flowgraph->run();

      auto actual_values = flowgraph->actual_values();
      auto actual_errors = flowgraph->actual_errors();

      CPPUNIT_ASSERT_EQUAL(post_trigger_samples, (uint32_t)actual_values.size());
      CPPUNIT_ASSERT_EQUAL(post_trigger_samples, (uint32_t)actual_errors.size());

      assert_float_vector(values.begin() + pre_trigger_samples, // see make_triggered_data_tag
              values.begin() + pre_trigger_samples + post_trigger_samples, actual_values.begin());
      assert_float_vector(errors.begin() + pre_trigger_samples,
              errors.begin() + pre_trigger_samples + post_trigger_samples, actual_errors.begin());

      auto out_tags = flowgraph->tags();
      CPPUNIT_ASSERT_EQUAL(1, (int)out_tags.size());

      auto trigger = decode_acq_info_tag(out_tags[0]);
      CPPUNIT_ASSERT_EQUAL(0, (int)trigger.pre_samples);
      CPPUNIT_ASSERT_EQUAL(post_trigger_samples, trigger.samples);
      CPPUNIT_ASSERT_EQUAL(timebase, (float)trigger.timebase);
    }

    void
    qa_extractor::test_user_delay()
    {
      float timebase = 0.000002;                     // seconds
      float pre_trigger = 0.001, post_trigger = 0.1; // seconds
      float user_delay = 0.0005;                     // seconds

      uint32_t pre_trigger_samples = (uint32_t)(pre_trigger / timebase) * 2;
      uint32_t post_trigger_samples = (uint32_t)(post_trigger / timebase) * 2;

      // make some more data in order to allow user delay to be applied
      size_t data_size = (pre_trigger_samples + post_trigger_samples) * 2;

      auto values = make_test_data(data_size);
      auto errors = make_test_data(data_size, 0.1);
      auto flowgraph = make_test_flowgraph(values, errors, pre_trigger, post_trigger);

      acq_info_t info {};
      info.pre_samples = pre_trigger_samples;
      info.samples = post_trigger_samples;
      info.timebase = timebase;
      info.trigger_timestamp = 123456;
      info.actual_delay = info.user_delay = user_delay;
      info.triggered_data = true;

      std::vector<gr::tag_t> tags = {
        make_acq_info_tag(info)
      };

      flowgraph->value_src->set_data(values, tags);
      flowgraph->run();

      auto actual_values = flowgraph->actual_values();
      auto actual_errors = flowgraph->actual_errors();

      uint32_t expected_pre_trigger_samples = (uint32_t)(pre_trigger / timebase);
      uint32_t expected_post_trigger_samples = (uint32_t)(post_trigger / timebase);

      auto expected_samples = expected_post_trigger_samples + expected_pre_trigger_samples;
      CPPUNIT_ASSERT(data_size > actual_values.size());
      CPPUNIT_ASSERT_EQUAL(expected_samples, (uint32_t)actual_values.size());
      CPPUNIT_ASSERT_EQUAL(expected_samples, (uint32_t)actual_errors.size());

      auto user_delay_samples = (int)(user_delay / timebase);

      auto begin_offset = pre_trigger_samples - expected_pre_trigger_samples + user_delay_samples;
      CPPUNIT_ASSERT(begin_offset < pre_trigger_samples); // detect overflow

      assert_float_vector(values.begin() + begin_offset, // see make_triggered_data_tag
              values.begin() + begin_offset + expected_samples, actual_values.begin());
      assert_float_vector(errors.begin() + begin_offset,
              errors.begin() + begin_offset + expected_samples, actual_errors.begin());

      auto out_tags = flowgraph->tags();
      CPPUNIT_ASSERT_EQUAL(1, (int)out_tags.size());

      auto trigger = decode_acq_info_tag(out_tags[0]);
      CPPUNIT_ASSERT_EQUAL(expected_pre_trigger_samples, trigger.pre_samples);
      CPPUNIT_ASSERT_EQUAL(expected_post_trigger_samples, trigger.samples);
      CPPUNIT_ASSERT_EQUAL(timebase, (float)trigger.timebase);
      CPPUNIT_ASSERT_EQUAL(user_delay, (float)trigger.user_delay);
      CPPUNIT_ASSERT_EQUAL(user_delay, (float)trigger.actual_delay);
    }

    void
    qa_extractor::test_realignment_delay()
    {
        // TODO: jgolob handled in the same way as user delay....

//      double timebase = 0.000002;                     // seconds
//      double pre_trigger = 0.001, post_trigger = 0.1; // seconds
//      double realignment_delay = 0.006;               // seconds
//
//      uint32_t pre_trigger_samples = (uint32_t)(pre_trigger / timebase);
//      uint32_t post_trigger_samples = (uint32_t)(post_trigger / timebase) * 2.0; // make space for realignment delay
//
//      // make some more data in order to allow user delay to be applied
//      size_t data_size = pre_trigger_samples + post_trigger_samples;
//
//      auto values = make_test_data(data_size);
//      auto errors = make_test_data(data_size, 0.1);
//      auto flowgraph = make_test_flowgraph(values, errors);
//
//      std::vector<gr::tag_t> tags = {
//        make_trigger_tag(pre_trigger_samples, post_trigger_samples, 0, timebase)
//      };
//      tags[0].offset = 0;
//
//      flowgraph->value_src->set_data(values, tags);
//      flowgraph->extractor->set_drop_non_tigger_data(true);
//      flowgraph->extractor->set_timebase(timebase);
//      flowgraph->extractor->set_observation_window(post_trigger, pre_trigger);
//
//      // Add made-up timing event
//      flowgraph->extractor->set_timing_available(true);
//      flowgraph->extractor->add_timing_event("TEST_EVENT", 987654321, true);
//      flowgraph->extractor->add_delay_event(987654321,
//              static_cast<uint32_t>(realignment_delay * 1000000000.0));
//
//      flowgraph->run();
//
//      uint32_t expected_post_trigger_samples, expected_pre_trigger_samples;
//      flowgraph->extractor->get_observation_window_in_samples(expected_post_trigger_samples,
//              expected_pre_trigger_samples);
//
//      auto actual_values = flowgraph->actual_values();
//      auto actual_errors = flowgraph->actual_errors();
//
//      auto expected_samples = expected_post_trigger_samples + expected_pre_trigger_samples;
//      CPPUNIT_ASSERT(data_size > actual_values.size());
//      CPPUNIT_ASSERT_EQUAL(expected_samples, (uint32_t)actual_values.size());
//      CPPUNIT_ASSERT_EQUAL(expected_samples, (uint32_t)actual_errors.size());
//
//      // Due to floating point nature we might be off by a sample
//      auto realignment_delay_samples = static_cast<uint32_t>(realignment_delay / timebase);
//
//      auto it = std::find(values.begin(), values.end(), actual_values[0]);
//      CPPUNIT_ASSERT(it != values.end());
//      auto distance = std::distance(values.begin(), it);
//
//      CPPUNIT_ASSERT_DOUBLES_EQUAL(realignment_delay_samples, distance, 1.0);
//
//      assert_float_vector(values.begin() + distance, // see make_triggered_data_tag
//              values.begin() + distance + expected_samples, actual_values.begin());
//      assert_float_vector(errors.begin() + distance,
//              errors.begin() + distance + expected_samples, actual_errors.begin());
//
//      auto out_tags = flowgraph->tags();
//      CPPUNIT_ASSERT_EQUAL(2, (int)out_tags.size());
//
//      auto trigger = decode_acq_info_tag(out_tags[0]);
//      CPPUNIT_ASSERT_EQUAL(expected_pre_trigger_samples, trigger.pre_samples);
//      CPPUNIT_ASSERT_EQUAL(expected_post_trigger_samples, trigger.samples);
//      CPPUNIT_ASSERT_EQUAL(timebase, trigger.timebase);
//      CPPUNIT_ASSERT_EQUAL(0.0, trigger.user_delay);
//      CPPUNIT_ASSERT_DOUBLES_EQUAL(realignment_delay, trigger.actual_delay, 0.0000001);
//      CPPUNIT_ASSERT_EQUAL(int64_t{987654321}, trigger.trigger_timestamp);
    }

    // If not enoguth pre- or post-trigger samples are provided no realignment
    // is performed
    void
    qa_extractor::test_not_enough_trigger_samples()
    {
//        double timebase = 0.000002;                     // seconds
//        double pre_trigger = 0.001, post_trigger = 0.1; // seconds
//
//        uint32_t pre_trigger_samples = (uint32_t)(pre_trigger / timebase) + 10;
//        // +10 not suffitient to perform realignment
//        uint32_t post_trigger_samples = (uint32_t)(post_trigger / timebase) + 10;
//
//        // make some more data in order to allow user delay to be applied
//        size_t data_size = pre_trigger_samples + post_trigger_samples;
//
//        auto values = make_test_data(data_size);
//        auto errors = make_test_data(data_size, 0.1);
//        auto flowgraph = make_test_flowgraph(values, errors);
//
//        std::vector<gr::tag_t> tags = {
//          make_trigger_tag(pre_trigger_samples, post_trigger_samples, 0, timebase)
//        };
//        tags[0].offset = 0;
//
//        flowgraph->value_src->set_data(values, tags);
//        flowgraph->extractor->set_drop_non_tigger_data(true);
//        flowgraph->extractor->set_timebase(timebase);
//        flowgraph->extractor->set_observation_window(post_trigger, pre_trigger);
//
//        // Add made-up timing event
//        flowgraph->extractor->set_timing_available(true);
//        flowgraph->extractor->add_timing_event("TEST_EVENT", 987654321, true);
//        flowgraph->extractor->add_delay_event(987654321,
//                static_cast<uint32_t>(0.2 * 1000000000.0));
//
//        flowgraph->run();
//
//        auto actual_values = flowgraph->actual_values();
//        auto actual_errors = flowgraph->actual_errors();
//
//        assert_float_vector(values.begin(), values.end(), actual_values.begin());
//        assert_float_vector(errors.begin(), errors.end(), actual_errors.begin());
//
//        auto out_tags = flowgraph->tags();
//        CPPUNIT_ASSERT_EQUAL(2, (int)out_tags.size());
//
//        auto trigger = decode_acq_info_tag(out_tags[0]);
//        CPPUNIT_ASSERT_EQUAL(pre_trigger_samples, trigger.pre_samples);
//        CPPUNIT_ASSERT_EQUAL(post_trigger_samples, trigger.samples);
//        CPPUNIT_ASSERT_EQUAL(timebase, trigger.timebase);
//        CPPUNIT_ASSERT_EQUAL(0.0, trigger.user_delay);
//        CPPUNIT_ASSERT_EQUAL(0.0, trigger.actual_delay);
//        CPPUNIT_ASSERT_EQUAL(int64_t{987654321}, trigger.trigger_timestamp);
//        CPPUNIT_ASSERT(CHANNEL_STATUS_REALIGNMENT_ERROR & trigger.status);
    }
  } /* namespace digitizers */
} /* namespace gr */

