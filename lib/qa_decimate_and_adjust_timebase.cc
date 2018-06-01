/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#include <gnuradio/top_block.h>

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
    qa_decimate_and_adjust_timebase::test_single_decim_factor(int n, int d)
    {
      std::vector<float> cycle;
      for(int i = 1; i <= n; i++) {
        cycle.push_back(i);
      }
      std::vector<float> signal;
      for(int i = 0; i <= d * 5; i++) {
        signal.insert(signal.begin(), cycle.begin(), cycle.end());
      }
      auto top = gr::make_top_block("test_single_decim_factor");
      auto src = blocks::vector_source_f::make(signal);
      auto snk = blocks::vector_sink_f::make(1);
      auto decim = digitizers::decimate_and_adjust_timebase::make(d, 0.0);
      top->connect(src, 0, decim, 0);
      top->connect(decim, 0, snk, 0);

      top->run();

      auto data = snk->data();

      CPPUNIT_ASSERT(data.size() != 0);

      for(size_t i = 0; i < data.size(); i++) {
        CPPUNIT_ASSERT(data.at(i) == signal.at(((i+1)*d-1)%n));
      }
    }

    void
    qa_decimate_and_adjust_timebase::test_decimation()
    {
      for(int i = 1; i <= 30; i++){
        test_single_decim_factor(30, i);
      }
    }

  } /* namespace digitizers */
} /* namespace gr */

