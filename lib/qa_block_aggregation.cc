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
#include <gnuradio/blocks/vector_sink_f.h>
#include <digitizers/block_aggregation.h>
#include "digitizers/status.h"
#include "qa_common.h"
#include <utils.h>

namespace gr {
  namespace digitizers {

  struct aggregation_test_flowgraph_t
  {
    gr::top_block_sptr top;
    gr::blocks::vector_source_f::sptr value_src;
    gr::blocks::vector_source_f::sptr error_src;
    gr::digitizers::block_aggregation::sptr aggreagtion_block;
    gr::blocks::vector_sink_f::sptr value_sink;
    gr::blocks::vector_sink_f::sptr error_sink;
    std::vector<float> values;
    std::vector<float> errors;

    aggregation_test_flowgraph_t (int alg_id,
                                  int decim,
                                  int delay,
                                  const std::vector<float> &fir_taps,
                                  double low_freq,
                                  double up_freq,
                                  double tr_width,
                                  const std::vector<double> &fb_user_taps,
                                  const std::vector<double> &fw_user_taps,
                                  double samp_rate,
                                  const std::vector<tag_t> &tags,
                                  size_t data_size=33333)
    {
      values = make_test_data(data_size);
      errors = make_test_data(data_size, 0.2);

      top = gr::make_top_block("test");
      value_src = gr::blocks::vector_source_f::make(values, false, 1, tags);
      error_src = gr::blocks::vector_source_f::make(errors);
      aggreagtion_block = gr::digitizers::block_aggregation::make(alg_id, decim, delay, fir_taps, low_freq, up_freq, tr_width, fb_user_taps, fw_user_taps, samp_rate);
      value_sink = gr::blocks::vector_sink_f::make();
      error_sink = gr::blocks::vector_sink_f::make();

      top->connect(value_src, 0, aggreagtion_block, 0);
      top->connect(error_src, 0, aggreagtion_block, 1);

      top->connect(aggreagtion_block, 0, value_sink, 0);
      top->connect(aggreagtion_block, 1, error_sink, 0);
    }

    void
    run()
    {
      top->run();
    }

    std::vector<gr::tag_t>
    tags()
    {
      return value_sink->tags();
    }
  };

  void
  qa_block_aggregation::basic_connection()
  {
    std::vector<float> taps({1.0, -3.9692103976755453, 5.9090978836797685, -3.9105406695954064, 0.9706534726794167});
    std::vector<double> taps_d({1.0, -3.9692103976755453, 5.9090978836797685, -3.9105406695954064, 0.9706534726794167});

    std::vector<gr::tag_t> tags;
    aggregation_test_flowgraph_t flowgraph(0, 1, 1, taps, 1, 400, 4, taps_d, taps_d, 1000, tags);

    flowgraph.run();
  }

  void
  qa_block_aggregation::test_tags()
  {
    std::vector<float> taps({1.0, -3.9692103976755453, 5.9090978836797685, -3.9105406695954064, 0.9706534726794167});
    std::vector<double> taps_d({1.0, -3.9692103976755453, 5.9090978836797685, -3.9105406695954064, 0.9706534726794167});
    std::vector<tag_t> tags;
    tag_t tag0 = make_trigger_tag(10);
    tag_t tag1 = make_trigger_tag(100);
    tags.push_back(tag0);
    tags.push_back(tag1);

    aggregation_test_flowgraph_t flowgraph(0, 2, 1, taps, 1, 400, 4, taps_d, taps_d, 1000, tags);

    flowgraph.run();
    auto out_tags = flowgraph.tags();
    CPPUNIT_ASSERT_EQUAL(out_tags[0].key, pmt::string_to_symbol(trigger_tag_name));
    CPPUNIT_ASSERT_EQUAL(out_tags[1].key, pmt::string_to_symbol(trigger_tag_name));

    // gnuradio takes care on correct offset realligment
    CPPUNIT_ASSERT(out_tags[0].offset <= 6 && out_tags[0].offset >= 4 );
    CPPUNIT_ASSERT(out_tags[1].offset <= 51 && out_tags[1].offset >= 49 );
  }

  } /* namespace digitizers */
} /* namespace gr */

