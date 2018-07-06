/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#include <gnuradio/attributes.h>
#include <cppunit/TestAssert.h>
#include "qa_block_aggregation.h"
#include <digitizers/block_aggregation.h>
#include <digitizers/tags.h>
#include <gnuradio/top_block.h>
#include <gnuradio/blocks/null_sink.h>
#include <gnuradio/blocks/null_source.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <digitizers/block_aggregation.h>
#include "digitizers/status.h"
#include <utils.h>

namespace gr {
  namespace digitizers {

  void
  qa_block_aggregation::basic_connection()
  {
    std::vector<float> taps({1.0, -3.9692103976755453, 5.9090978836797685, -3.9105406695954064, 0.9706534726794167});
    std::vector<double> taps_d({1.0, -3.9692103976755453, 5.9090978836797685, -3.9105406695954064, 0.9706534726794167});
    auto top = gr::make_top_block("basic_connection");
    auto sine = gr::blocks::vector_source_f::make(taps);
    auto noise = gr::blocks::null_source::make(4);
    auto agg = digitizers::block_aggregation::make(0, 1, 1, taps, 1, 400, 4, taps_d, taps_d, 1000);
    auto null1 = blocks::null_sink::make(4);
    auto null2 = blocks::null_sink::make(4);
    top->connect(sine, 0, agg, 0);
    top->connect(noise, 0, agg, 1);
    top->connect(agg, 0, null1, 0);
    top->connect(agg, 1, null2, 0);

    top->run();
  }

  void
  qa_block_aggregation::test_tags()
  {
    std::vector<float> taps({1.0, -3.9692103976755453, 5.9090978836797685, -3.9105406695954064, 0.9706534726794167});
    std::vector<double> taps_d({1.0, -3.9692103976755453, 5.9090978836797685, -3.9105406695954064, 0.9706534726794167});
    std::vector<tag_t> tags;
    tag_t tag0 = make_timebase_info_tag(100);
    tag_t tag1 = make_timebase_info_tag(100);
    tag0.offset = 0;
    tag1.offset = 10;
    tags.push_back(tag0);
    tags.push_back(tag1);
    auto top = gr::make_top_block("basic_connection");
    auto sine = gr::blocks::vector_source_f::make(taps, false, 1, tags);
    auto noise = gr::blocks::null_source::make(4);
    auto agg = digitizers::block_aggregation::make(0, 1, 1, taps, 1, 400, 4, taps_d, taps_d, 1000);
    auto null1 = blocks::null_sink::make(4);
    auto null2 = blocks::null_sink::make(4);
    top->connect(sine, 0, agg, 0);
    top->connect(noise, 0, agg, 1);
    top->connect(agg, 0, null1, 0);
    top->connect(agg, 1, null2, 0);

    top->run();
  }

  } /* namespace digitizers */
} /* namespace gr */
