/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#include <gnuradio/top_block.h>


#include <gnuradio/attributes.h>
#include <cppunit/TestAssert.h>
#include "qa_interlock_generation_ff.h"
#include <digitizers/interlock_generation_ff.h>
#include <gnuradio/blocks/vector_source.h>
#include <gnuradio/blocks/vector_sink.h>
#include <digitizers/tags.h>

namespace gr {
  namespace digitizers {

    int count_interlock_calls = 0;
    void interlock(long timebase, void *userdata)
    {
      count_interlock_calls++;
    }

    void
    qa_interlock_generation_ff::interlock_generation_test()
    {
      auto top = gr::make_top_block("interlock_generation_test");

      std::vector<float> sig_v;
      std::vector<float> min_v;
      std::vector<float> max_v;

      // signal gets smaller than minimum
      for(int i = 0; i<10; i++) {
        sig_v.push_back(0);
        min_v.push_back(i-5);
        max_v.push_back(i+5);
      }
      // signal gets bigger than maximum
      for(int i = 0; i<10; i++) {
        sig_v.push_back(0);
        min_v.push_back(i-15);
        max_v.push_back(i-5);
      }
      auto sig = blocks::vector_source_f::make(sig_v);
      auto min = blocks::vector_source_f::make(min_v);
      auto max = blocks::vector_source_f::make(max_v);

      auto i_lk = interlock_generation_ff::make(-100, 100);
      i_lk->set_callback(&interlock, nullptr);

      auto snk = blocks::vector_sink_f::make(1);

      top->connect(sig, 0, i_lk, 0);
      top->connect(min, 0, i_lk, 1);
      top->connect(max, 0, i_lk, 2);

      top->connect(i_lk, 0, snk, 0);

      top->run();

      auto interlocks = snk->data();

      CPPUNIT_ASSERT_EQUAL(sig_v.size(), interlocks.size());
      CPPUNIT_ASSERT_EQUAL(1, count_interlock_calls);
      for( size_t i = 0; i < sig_v.size(); i++) {
        bool exp = (sig_v.at(i) < max_v.at(i) && sig_v.at(i) > min_v.at(i));
        bool act = (interlocks.at(i) == 0);
        CPPUNIT_ASSERT_EQUAL(exp, act);
      }

    }

  } /* namespace digitizers */
} /* namespace gr */

