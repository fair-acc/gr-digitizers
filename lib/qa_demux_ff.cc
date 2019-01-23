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
            const std::vector<gr::tag_t> &tags=std::vector<gr::tag_t>{})
    {
      extractor_test_flowgraph_t flowgraph;

      flowgraph.top = gr::make_top_block("test");
      flowgraph.value_src = gr::blocks::vector_source_f::make(values, false, 1, tags);
      flowgraph.error_src = gr::blocks::vector_source_f::make(errors);
      flowgraph.extractor = gr::digitizers::demux_ff::make(post_trigger_window, pre_trigger_window);
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
      unsigned pre_trigger_samples = 5000;
      unsigned post_trigger_samples = 20000;

      size_t data_size = 125000;

      auto values = make_test_data(data_size);
      auto errors = make_test_data(data_size, 0.1);

      auto trigger_offset = 10000;

      acq_info_t acq_info;

      std::vector<gr::tag_t> tags = {
        make_trigger_tag(trigger_offset),
        make_acq_info_tag(acq_info,trigger_offset + 2000)
      };

      auto flowgraph = make_test_flowgraph(values, errors, pre_trigger_samples, post_trigger_samples, tags);

      flowgraph.run();

      auto actual_values = flowgraph.actual_values();
      auto actual_errors = flowgraph.actual_errors();

      auto collected_samples = pre_trigger_samples + post_trigger_samples;
      CPPUNIT_ASSERT_EQUAL(collected_samples, (uint32_t)actual_values.size());
      CPPUNIT_ASSERT_EQUAL(collected_samples, (uint32_t)actual_errors.size());

//      int sample_of_interest = trigger_offset - pre_trigger_samples;
//      for (uint i= 0; i< collected_samples; i++)
//      {
//          std::cout << "values expected - actual : " << values[sample_of_interest] << " - " << actual_values[i] << std::endl;
//          sample_of_interest ++;
//      }

      ASSERT_VECTOR_EQUAL(values.begin() + trigger_offset - pre_trigger_samples,
                          values.begin() + trigger_offset + post_trigger_samples - 1,
                          actual_values.begin());
      ASSERT_VECTOR_EQUAL(errors.begin() + trigger_offset - pre_trigger_samples,
                          errors.begin() + trigger_offset + post_trigger_samples - 1,
                          actual_errors.begin());

      auto out_tags = flowgraph.tags();
      CPPUNIT_ASSERT_EQUAL(2, (int)out_tags.size());

      CPPUNIT_ASSERT_EQUAL(out_tags[0].key, pmt::string_to_symbol(trigger_tag_name));
      auto trigger_tag = decode_trigger_tag(out_tags[0]);
      CPPUNIT_ASSERT_EQUAL(uint64_t {pre_trigger_samples}, out_tags[0].offset);
      CPPUNIT_ASSERT_EQUAL(post_trigger_samples, trigger_tag.post_trigger_samples);
      CPPUNIT_ASSERT_EQUAL(pre_trigger_samples, trigger_tag.pre_trigger_samples);

      CPPUNIT_ASSERT_EQUAL(out_tags[1].key, pmt::string_to_symbol(acq_info_tag_name));
      CPPUNIT_ASSERT_EQUAL(uint64_t {pre_trigger_samples + 2000}, out_tags[1].offset);
    }

    void
    qa_demux_ff::test_multi_trigger()
    {
      unsigned pre_trigger_samples = 50;
      unsigned post_trigger_samples = 200;
      unsigned trigger_samples = pre_trigger_samples + post_trigger_samples;

      size_t data_size = 2000;

      auto values = make_test_data(data_size);
      auto errors = make_test_data(data_size, 0.1);

      auto trigger1_offset =  500;
      auto trigger2_offset = 1000;
      auto trigger3_offset = 1500;
      acq_info_t acq_info;

      std::vector<gr::tag_t> tags = {
        make_trigger_tag(trigger1_offset),
        make_trigger_tag(trigger2_offset),
        make_trigger_tag(trigger3_offset),
        make_acq_info_tag(acq_info,trigger1_offset + 10),
        make_acq_info_tag(acq_info,trigger2_offset - 50),
        make_acq_info_tag(acq_info,trigger3_offset + 199), // the trigger sample itself is the first sample
        make_acq_info_tag(acq_info,trigger2_offset - 51), // out of scope
        make_acq_info_tag(acq_info,trigger2_offset + 200) // out of scope
      };

      auto flowgraph = make_test_flowgraph(values, errors, pre_trigger_samples, post_trigger_samples, tags);

      flowgraph.run();

      auto actual_values = flowgraph.actual_values();
      auto actual_errors = flowgraph.actual_errors();

      auto collected_samples = trigger_samples * 3;
      CPPUNIT_ASSERT_EQUAL(collected_samples, (uint32_t)actual_values.size());
      CPPUNIT_ASSERT_EQUAL(collected_samples, (uint32_t)actual_errors.size());

//      int sample_of_interest = trigger1_offset - pre_trigger_samples;
//      for (uint i= 0; i< pre_trigger_samples + post_trigger_samples; i++)
//      {
//          std::cout << "errors expected - actual : " << errors[sample_of_interest] << " - " << actual_errors[i] << std::endl;
//          sample_of_interest ++;
//      }
      ASSERT_VECTOR_EQUAL(values.begin() + trigger1_offset - pre_trigger_samples,
                          values.begin() + trigger1_offset + post_trigger_samples - 1,
                          actual_values.begin());
      ASSERT_VECTOR_EQUAL(errors.begin() + trigger1_offset - pre_trigger_samples,
                          errors.begin() + trigger1_offset + post_trigger_samples - 1,
                          actual_errors.begin());
      ASSERT_VECTOR_EQUAL(values.begin() + trigger2_offset - pre_trigger_samples,
                          values.begin() + trigger2_offset + post_trigger_samples - 1,
                          actual_values.begin() + trigger_samples );
      ASSERT_VECTOR_EQUAL(errors.begin() + trigger2_offset - pre_trigger_samples,
                          errors.begin() + trigger2_offset + post_trigger_samples - 1,
                          actual_errors.begin() + trigger_samples);
      ASSERT_VECTOR_EQUAL(values.begin() + trigger3_offset - pre_trigger_samples,
                          values.begin() + trigger3_offset + post_trigger_samples - 1,
                          actual_values.begin() + 2 * trigger_samples);
      ASSERT_VECTOR_EQUAL(errors.begin() + trigger3_offset - pre_trigger_samples,
                          errors.begin() + trigger3_offset + post_trigger_samples - 1,
                          actual_errors.begin() + 2 * trigger_samples);

      auto out_tags = flowgraph.tags();
      CPPUNIT_ASSERT_EQUAL(6, (int)out_tags.size());
      CPPUNIT_ASSERT_EQUAL(out_tags[0].key, pmt::string_to_symbol(trigger_tag_name));

      auto trigger_tag1 = decode_trigger_tag(out_tags[0]);
      CPPUNIT_ASSERT_EQUAL(uint64_t {pre_trigger_samples}, out_tags[0].offset);
      CPPUNIT_ASSERT_EQUAL(post_trigger_samples, trigger_tag1.post_trigger_samples);
      CPPUNIT_ASSERT_EQUAL(pre_trigger_samples, trigger_tag1.pre_trigger_samples);

      CPPUNIT_ASSERT_EQUAL(out_tags[1].key, pmt::string_to_symbol(acq_info_tag_name));
      CPPUNIT_ASSERT_EQUAL(uint64_t {pre_trigger_samples + 10}, out_tags[1].offset);

      CPPUNIT_ASSERT_EQUAL(out_tags[2].key, pmt::string_to_symbol(acq_info_tag_name));
      CPPUNIT_ASSERT_EQUAL(uint64_t {trigger_samples + pre_trigger_samples - 50}, out_tags[2].offset);

      CPPUNIT_ASSERT_EQUAL(out_tags[3].key, pmt::string_to_symbol(trigger_tag_name));
      auto trigger_tag2 = decode_trigger_tag(out_tags[3]);
      CPPUNIT_ASSERT_EQUAL(uint64_t {trigger_samples + pre_trigger_samples}, out_tags[3].offset);
      CPPUNIT_ASSERT_EQUAL(post_trigger_samples, trigger_tag2.post_trigger_samples);
      CPPUNIT_ASSERT_EQUAL(pre_trigger_samples, trigger_tag2.pre_trigger_samples);

      CPPUNIT_ASSERT_EQUAL(out_tags[4].key, pmt::string_to_symbol(trigger_tag_name));
      auto trigger_tag3 = decode_trigger_tag(out_tags[4]);
      CPPUNIT_ASSERT_EQUAL(uint64_t {trigger_samples * 2 + pre_trigger_samples}, out_tags[4].offset);
      CPPUNIT_ASSERT_EQUAL(post_trigger_samples, trigger_tag3.post_trigger_samples);
      CPPUNIT_ASSERT_EQUAL(pre_trigger_samples, trigger_tag3.pre_trigger_samples);

      CPPUNIT_ASSERT_EQUAL(out_tags[5].key, pmt::string_to_symbol(acq_info_tag_name));
      CPPUNIT_ASSERT_EQUAL(uint64_t {trigger_samples * 2 + pre_trigger_samples + 199}, out_tags[5].offset);
    }

    void
    qa_demux_ff::test_to_few_post_trigger_samples()
    {
        unsigned pre_trigger_samples = 5;
        unsigned post_trigger_samples = 20;

        size_t data_size = 200;

        auto values = make_test_data(data_size);
        auto errors = make_test_data(data_size, 0.1);

        auto trigger_offset = 181;

        acq_info_t acq_info;

        std::vector<gr::tag_t> tags = {
          make_trigger_tag(trigger_offset),
          make_acq_info_tag(acq_info,trigger_offset + 2)
        };

        auto flowgraph = make_test_flowgraph(values, errors, pre_trigger_samples, post_trigger_samples, tags);

        flowgraph.run();

        auto actual_values = flowgraph.actual_values();
        auto actual_errors = flowgraph.actual_errors();

        CPPUNIT_ASSERT_EQUAL((uint32_t)0, (uint32_t)actual_values.size());
        CPPUNIT_ASSERT_EQUAL((uint32_t)0, (uint32_t)actual_errors.size());

        auto out_tags = flowgraph.tags();
        CPPUNIT_ASSERT_EQUAL(0, (int)out_tags.size());
    }

  } /* namespace digitizers */
} /* namespace gr */

