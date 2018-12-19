/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#include <gnuradio/attributes.h>
#include <cppunit/TestAssert.h>
#include "qa_demux_ff.h"
#include "utils.h"
#include "qa_common.h"
#include <digitizers/demux_ff.h>
#include <digitizers/status.h>
#include <digitizers/tags.h>
#include <digitizers/edge_trigger_utils.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/blocks/vector_sink_f.h>
#include <gnuradio/blocks/tag_debug.h>
#include <gnuradio/top_block.h>


namespace gr {
 namespace digitizers {

    struct extractor_test_flowgraph_t
    {
      gr::top_block_sptr top;
      gr::blocks::vector_source_f::sptr value_src;
      gr::blocks::vector_source_f::sptr error_src;
      gr::digitizers::demux_ff::sptr extractor;
      gr::blocks::vector_sink_f::sptr value_sink;
      gr::blocks::vector_sink_f::sptr error_sink;
      gr::blocks::tag_debug::sptr tag_debug;

      void
      run()
      {
        top->run();
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

    inline extractor_test_flowgraph_t
    make_test_flowgraph(const std::vector<float> &values,
            const std::vector<float> &errors,
            unsigned pre_trigger_window,
            unsigned post_trigger_window,
            float samp_rate=100000,
            unsigned history=10000,
            const std::vector<gr::tag_t> &tags=std::vector<gr::tag_t>{})
    {
      extractor_test_flowgraph_t flowgraph;

      flowgraph.top = gr::make_top_block("test");
      flowgraph.value_src = gr::blocks::vector_source_f::make(values, false, 1, tags);
      flowgraph.error_src = gr::blocks::vector_source_f::make(errors);
      flowgraph.extractor = gr::digitizers::demux_ff::make(samp_rate, history, post_trigger_window, pre_trigger_window);
      flowgraph.value_sink = gr::blocks::vector_sink_f::make();
      flowgraph.error_sink = gr::blocks::vector_sink_f::make();
      flowgraph.tag_debug = gr::blocks::tag_debug::make(sizeof(float), "neki");
      flowgraph.tag_debug->set_display(false);

      flowgraph.top->connect(flowgraph.value_src, 0, flowgraph.tag_debug, 0);
      flowgraph.top->connect(flowgraph.value_src, 0, flowgraph.extractor, 0);
      flowgraph.top->connect(flowgraph.error_src, 0, flowgraph.extractor, 1);

      flowgraph.top->connect(flowgraph.extractor, 0, flowgraph.value_sink, 0);
      flowgraph.top->connect(flowgraph.extractor, 1, flowgraph.error_sink, 0);

      return flowgraph;
    }

    void
    qa_demux_ff::test_no_trigger()
    {
      size_t data_size = 5000000;
      auto values = make_test_data(data_size);
      auto errors = make_test_data(data_size, 0.1);
      auto flowgraph = make_test_flowgraph(values, errors, 100, 200);

      flowgraph.run();

      auto actual_values = flowgraph.actual_values();
      auto actual_errors = flowgraph.actual_errors();

      // no trigger is present therefore we expect no data on outputs
      CPPUNIT_ASSERT_EQUAL(0, (int)actual_values.size());
      CPPUNIT_ASSERT_EQUAL(0, (int)actual_errors.size());
    }

    void
    qa_demux_ff::test_single_trigger()
    {
      float samp_rate = 200000.0;

      unsigned pre_trigger_samples = 500;
      unsigned post_trigger_samples = 2000;

      size_t data_size = (pre_trigger_samples + post_trigger_samples) * 10;

      auto values = make_test_data(data_size);
      auto errors = make_test_data(data_size, 0.1);

      auto trigger_offset = 1000;

      wr_event_t wr_event;
      wr_event.wr_trigger_stamp = 87654321;

      std::vector<gr::tag_t> tags = {
        make_trigger_tag(trigger_offset),
        make_wr_event_tag(wr_event,trigger_offset + 100)
      };

      auto flowgraph = make_test_flowgraph(values, errors, pre_trigger_samples,
              post_trigger_samples, samp_rate, 10000 /*history*/, tags);

      flowgraph.run();

      auto actual_values = flowgraph.actual_values();
      auto actual_errors = flowgraph.actual_errors();

      auto collected_samples = pre_trigger_samples + post_trigger_samples;
      CPPUNIT_ASSERT_EQUAL(collected_samples, (uint32_t)actual_values.size());
      CPPUNIT_ASSERT_EQUAL(collected_samples, (uint32_t)actual_errors.size());

      ASSERT_VECTOR_EQUAL(values.begin() + trigger_offset - pre_trigger_samples,
                          values.begin() + trigger_offset - pre_trigger_samples + collected_samples,
                          actual_values.begin());
      ASSERT_VECTOR_EQUAL(errors.begin() + trigger_offset - pre_trigger_samples,
                          errors.begin() + trigger_offset - pre_trigger_samples + collected_samples,
                          actual_errors.begin());

      auto out_tags = flowgraph.tags();
      CPPUNIT_ASSERT_EQUAL(2, (int)out_tags.size());
      CPPUNIT_ASSERT_EQUAL(out_tags[0].key, pmt::string_to_symbol(trigger_tag_name));
      auto trigger_tag = decode_trigger_tag(out_tags[0]);

      CPPUNIT_ASSERT_EQUAL(uint64_t {0}, out_tags[0].offset);
      // no realignment & no user delay in this case
     // int64_t expected_timestamp = wr_event.wr_trigger_stamp - (pre_trigger_samples / samp_rate * 1000000000.0);
      CPPUNIT_ASSERT_EQUAL(wr_event.wr_trigger_stamp, trigger_tag.timestamp);
      CPPUNIT_ASSERT_EQUAL(post_trigger_samples, trigger_tag.post_trigger_samples);
      CPPUNIT_ASSERT_EQUAL(pre_trigger_samples, trigger_tag.pre_trigger_samples);
    }

    void
    qa_demux_ff::test_timeout()
    {
      float samp_rate = 200000.0;

      unsigned pre_trigger_samples = 500;
      unsigned post_trigger_samples = 2000;

      size_t data_size = (pre_trigger_samples + post_trigger_samples) * 10;

      auto values = make_test_data(data_size);
      auto errors = make_test_data(data_size, 0.1);

      auto trigger_offset = 1000;

      wr_event_t wr_event;
      wr_event.wr_trigger_stamp = 654321;

      std::vector<gr::tag_t> tags = {
        make_trigger_tag(trigger_offset),
        make_wr_event_tag(wr_event,trigger_offset + 100)
      };

      auto flowgraph = make_test_flowgraph(values, errors, pre_trigger_samples,
              post_trigger_samples, samp_rate, 10000 /*history*/, tags);

      flowgraph.run();

      auto actual_values = flowgraph.actual_values();
      auto actual_errors = flowgraph.actual_errors();

      auto collected_samples = pre_trigger_samples + post_trigger_samples;
      CPPUNIT_ASSERT_EQUAL(collected_samples, (uint32_t)actual_values.size());
      CPPUNIT_ASSERT_EQUAL(collected_samples, (uint32_t)actual_errors.size());

      ASSERT_VECTOR_EQUAL(values.begin() + trigger_offset - pre_trigger_samples,
                          values.begin() + trigger_offset - pre_trigger_samples + collected_samples,
                          actual_values.begin());
      ASSERT_VECTOR_EQUAL(errors.begin() + trigger_offset - pre_trigger_samples,
                          errors.begin() + trigger_offset - pre_trigger_samples + collected_samples,
                          actual_errors.begin());

      auto out_tags = flowgraph.tags();
      CPPUNIT_ASSERT_EQUAL(2, (int)out_tags.size());
      CPPUNIT_ASSERT_EQUAL(out_tags[0].key, pmt::string_to_symbol(trigger_tag_name));
      auto trigger_tag = decode_trigger_tag(out_tags[0]);

      CPPUNIT_ASSERT_EQUAL(uint64_t {0}, out_tags[0].offset);
      // no realignment & no user delay in this case
      //int64_t expected_timestamp = wr_event.wr_trigger_stamp - (pre_trigger_samples / samp_rate * 1000000000.0);
      CPPUNIT_ASSERT_EQUAL(wr_event.wr_trigger_stamp, trigger_tag.timestamp);
      CPPUNIT_ASSERT_EQUAL(uint32_t {channel_status_t::CHANNEL_STATUS_TIMEOUT_WAITING_WR_OR_REALIGNMENT_EVENT}, trigger_tag.status);
      CPPUNIT_ASSERT_EQUAL(post_trigger_samples, trigger_tag.post_trigger_samples);
      CPPUNIT_ASSERT_EQUAL(pre_trigger_samples, trigger_tag.pre_trigger_samples);
    }

    void
    qa_demux_ff::test_user_delay()
    {
      float samp_rate = 200000.0;

      unsigned pre_trigger_samples = 500;
      unsigned post_trigger_samples = 2000;

      size_t data_size = (pre_trigger_samples + post_trigger_samples) * 10;

      auto values = make_test_data(data_size);
      auto errors = make_test_data(data_size, 0.1);

      auto trigger_offset = 1000;

      acq_info_t acq_tag;
      acq_tag.status = 0x3;
      acq_tag.user_delay = 0.0007;
      acq_tag.actual_delay = 0.0007;

      wr_event_t wr_event;
      wr_event.wr_trigger_stamp = 87654321;

      edge_detect_t edge;
      edge.retrigger_event_timestamp = wr_event.wr_trigger_stamp + 333;
      edge.offset = trigger_offset + 99;

      std::vector<gr::tag_t> tags = {
        make_acq_info_tag(acq_tag,0),
        make_trigger_tag(trigger_offset),
        make_edge_detect_tag(edge),
        make_wr_event_tag(wr_event,trigger_offset + 100)
      };

      auto flowgraph = make_test_flowgraph(values, errors, pre_trigger_samples,
              post_trigger_samples, samp_rate, 10000 /*history*/, tags);

      flowgraph.run();

      auto actual_values = flowgraph.actual_values();
      auto actual_errors = flowgraph.actual_errors();

      auto collected_samples = pre_trigger_samples + post_trigger_samples;
      CPPUNIT_ASSERT_EQUAL(collected_samples, (uint32_t)actual_values.size());
      CPPUNIT_ASSERT_EQUAL(collected_samples, (uint32_t)actual_errors.size());

      auto delay_samples =  (acq_tag.user_delay * samp_rate)
              + 333 / 1000000000.0 * samp_rate;

      ASSERT_VECTOR_EQUAL(values.begin() + trigger_offset - pre_trigger_samples + delay_samples,
                          values.begin() + trigger_offset - pre_trigger_samples + delay_samples + collected_samples,
                          actual_values.begin());
      ASSERT_VECTOR_EQUAL(errors.begin() + trigger_offset - pre_trigger_samples + delay_samples,
                          errors.begin() + trigger_offset - pre_trigger_samples + delay_samples + collected_samples,
                          actual_errors.begin());

      auto out_tags = flowgraph.tags();
      CPPUNIT_ASSERT_EQUAL(2, (int)out_tags.size());

      CPPUNIT_ASSERT_EQUAL(out_tags[0].key, pmt::string_to_symbol(trigger_tag_name));
      auto trigger_tag = decode_trigger_tag(out_tags[0]);

      CPPUNIT_ASSERT_EQUAL(uint64_t {0}, out_tags[0].offset);
      // no realignment & no user delay in this case
      int64_t expected_timestamp = wr_event.wr_trigger_stamp
              - ((pre_trigger_samples / samp_rate) * 1000000000.0);

      // Note in this case timestamp is not adjusted because a different part of the signal is taken
      // thereby user delay and realignment compensation is achieved
      CPPUNIT_ASSERT_DOUBLES_EQUAL(expected_timestamp, trigger_tag.timestamp, 1);
      CPPUNIT_ASSERT_EQUAL(acq_tag.status, trigger_tag.status);
      CPPUNIT_ASSERT_EQUAL(post_trigger_samples, trigger_tag.post_trigger_samples);
      CPPUNIT_ASSERT_EQUAL(pre_trigger_samples, trigger_tag.pre_trigger_samples);
    }
  } /* namespace digitizers */
} /* namespace gr */

