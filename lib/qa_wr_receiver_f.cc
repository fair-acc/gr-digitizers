/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#include <gnuradio/attributes.h>
#include <cppunit/TestAssert.h>
#include "qa_wr_receiver_f.h"
#include <digitizers/wr_receiver_f.h>
#include <digitizers/tags.h>
#include <gnuradio/top_block.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/blocks/add_ff.h>
#include <gnuradio/blocks/vector_sink_f.h>
#include "qa_common.h"

namespace gr {
  namespace digitizers {

    void
    qa_wr_receiver_f::test()
    {
      size_t data_size = 16000;
      std::vector<float> expected_data(data_size);  // zeroes

      auto top = make_top_block("WR");

      // vector source is used to limit the data (flowgraph stops after data size samples is generated)
      auto vector_source = gr::blocks::vector_source_f::make(expected_data);
      auto wr_source = gr::digitizers::wr_receiver_f::make();
      auto adder = gr::blocks::add_ff::make();
      auto vector_sink = gr::blocks::vector_sink_f::make();

      top->connect(vector_source, 0, adder, 0);
      top->connect(wr_source,     0, adder, 1);
      top->connect(adder, 0, vector_sink, 0);

      // inject made up event
      wr_source->add_timing_event("made up event", 987654321, 123456789, true, false);

      top->run();

      auto data = vector_sink->data();
      CPPUNIT_ASSERT_EQUAL(data_size, data.size());
      ASSERT_VECTOR_EQUAL(expected_data.begin(), expected_data.end(), data.begin());

      auto tags = vector_sink->tags();
      CPPUNIT_ASSERT_EQUAL(size_t {1}, tags.size());

      auto event = decode_wr_event_tag(tags.at(0));
      CPPUNIT_ASSERT_EQUAL(std::string {"made up event"}, event.event_id);
      CPPUNIT_ASSERT_EQUAL(int64_t {987654321}, event.timestamp);
      CPPUNIT_ASSERT_EQUAL(int64_t {123456789}, event.last_beam_in_timestamp);
      CPPUNIT_ASSERT_EQUAL(true, event.time_sync_only);
      CPPUNIT_ASSERT_EQUAL(false, event.realignment_required);
      CPPUNIT_ASSERT_EQUAL(uint64_t {0}, event.offset);
    }

  } /* namespace digitizers */
} /* namespace gr */

