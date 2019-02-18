/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#include <gnuradio/top_block.h>

#include <digitizers/tags.h>
#include <gnuradio/attributes.h>
#include <cppunit/TestAssert.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/blocks/vector_sink_f.h>
#include "qa_decimate_and_adjust_timebase.h"
#include <digitizers/decimate_and_adjust_timebase.h>

#include <thread>
#include <chrono>

namespace gr {
  namespace digitizers {

    void
    qa_decimate_and_adjust_timebase::test_decimation()
    {
        int decim_factor = 5;
        int n_samples = decim_factor * 100;

        std::vector<float> signal;
        for(int i = 0; i <= n_samples; i++)
          signal.push_back(4711.);

        trigger_t tag1;
        tag1.pre_trigger_samples = 50;
        tag1.post_trigger_samples = 100;

        trigger_t tag2;
        tag2.pre_trigger_samples = 500;
        tag2.post_trigger_samples = 1000;

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
        auto decim = digitizers::decimate_and_adjust_timebase::make(decim_factor, 0.0);
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

        trigger_t trigger_tag_data0 = decode_trigger_tag(tags_out.at(0));
        trigger_t trigger_tag_data1 = decode_trigger_tag(tags_out.at(1));
        acq_info_t acq_info_tag = decode_acq_info_tag(tags_out.at(2));

        CPPUNIT_ASSERT_EQUAL(uint32_t(tag1.pre_trigger_samples/decim_factor),trigger_tag_data0.pre_trigger_samples);
        CPPUNIT_ASSERT_EQUAL(uint32_t(tag2.pre_trigger_samples/decim_factor), trigger_tag_data1.pre_trigger_samples);
        CPPUNIT_ASSERT_EQUAL(uint32_t(tag1.post_trigger_samples/decim_factor), trigger_tag_data0.post_trigger_samples);
        CPPUNIT_ASSERT_EQUAL(uint32_t(tag2.post_trigger_samples/decim_factor), trigger_tag_data1.post_trigger_samples);
        CPPUNIT_ASSERT_EQUAL(uint32_t(3), acq_info_tag.status); // logical OR of all stati
    }

  } /* namespace digitizers */
} /* namespace gr */

