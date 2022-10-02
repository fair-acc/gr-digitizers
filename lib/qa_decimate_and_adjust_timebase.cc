/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#include <gnuradio/top_block.h>

#include <digitizers/tags.h>
#include <gnuradio/attributes.h>
#include <cppunit/TestAssert.h>
#include <gnuradio/blocks/vector_source.h>
#include <gnuradio/blocks/vector_sink.h>
#include "qa_decimate_and_adjust_timebase.h"
#include <digitizers/decimate_and_adjust_timebase.h>

#include <thread>
#include <chrono>

namespace gr {
  namespace digitizers {

    void
    qa_decimate_and_adjust_timebase::test_decimation()
    {
        double samp_rate = 5000000;
        int decim_factor = 5;
        int n_samples = decim_factor * 100;

        std::vector<float> signal;
        for(int i = 0; i <= n_samples; i++)
          signal.push_back(4711.);

        trigger_t tag1;
        trigger_t tag2;

        acq_info_t acq_info_tag1;
        acq_info_tag1.status = 2;
        acq_info_t acq_info_tag2;
        acq_info_tag2.status = 1;


        std::vector<gr::tag_t> tags = {
          make_trigger_tag(tag1,40),
          make_trigger_tag(tag2,73),
          make_acq_info_tag(acq_info_tag1, 76), // two acq_info steps in the same decim window will be merged
          make_acq_info_tag(acq_info_tag2, 77)
        };

        auto top = gr::make_top_block("test_single_decim_factor");
        auto src = blocks::vector_source_f::make(signal,false, 1, tags);
        auto snk = blocks::vector_sink_f::make(1);
        auto decim = digitizers::decimate_and_adjust_timebase::make(decim_factor, 0.0, samp_rate);
        top->connect(src, 0, decim, 0);
        top->connect(decim, 0, snk, 0);

        top->run();

        auto data = snk->data();
        CPPUNIT_ASSERT_EQUAL(size_t(n_samples / decim_factor), data.size());

        auto tags_out = snk->tags();
        CPPUNIT_ASSERT_EQUAL(size_t(3), tags_out.size());
        CPPUNIT_ASSERT_EQUAL(uint64_t(40/decim_factor), tags_out[0].offset);
        CPPUNIT_ASSERT_EQUAL(uint64_t(73/decim_factor), tags_out[1].offset);
        CPPUNIT_ASSERT_EQUAL(uint64_t(76/decim_factor), tags_out[2].offset);

        acq_info_t acq_info_tag = decode_acq_info_tag(tags_out.at(2));

        CPPUNIT_ASSERT_EQUAL(uint32_t(3), acq_info_tag.status); // logical OR of all stati
    }

    void
    qa_decimate_and_adjust_timebase::offset_trigger_tag_test()
    {
      double samp_rate = 1000; // sample_to_sample distance = 1ms
      double decim = 10;
      size_t size = 1000;
      std::vector<float> samples;

      for(size_t i = 0; i < size; i++)
          samples.push_back(1.);

      trigger_t tag0, tag1,tag2,tag3;

      std::vector<gr::tag_t> tags = {
        make_trigger_tag(tag0,40), // samples 40 till 49 should be merged. Logic is "pick 1 of n". So no offset is expected.
        make_trigger_tag(tag1,41), // samples 40 till 49 should be merged. Logic is "pick 1 of n". So a positive offset of 1 samples (= 1ms) is expected.
        make_trigger_tag(tag2,75), // samples 70 till 79 should be merged. Logic is "pick 1 of n". So a positive offset of 5 samples (= 5ms) is expected.
        make_trigger_tag(tag3,79), // samples 70 till 79 should be merged. Logic is "pick 1 of n". So a positive offset of 9 samples (= 9ms) is expected.
      };

      auto top = gr::make_top_block("single_input_test");
      auto src = blocks::vector_source_f::make(samples, false, 1, tags);
      auto avg = decimate_and_adjust_timebase::make(decim, 0.0, samp_rate);
      auto snk = blocks::vector_sink_f::make(1);

      top->connect(src, 0, avg, 0);
      top->connect(avg, 0, snk, 0);

      top->run();
      auto data = snk->data();
      auto tags_out = snk->tags();
      CPPUNIT_ASSERT_EQUAL(size_t(4), tags_out.size());
      CPPUNIT_ASSERT_EQUAL(data.size(),size_t(size/decim));
    }

  } /* namespace digitizers */
} /* namespace gr */

