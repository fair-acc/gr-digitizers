/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#include <gnuradio/top_block.h>



#include <gnuradio/attributes.h>
#include <cppunit/TestAssert.h>
#include "qa_signal_averager.h"
#include <digitizers/signal_averager.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/blocks/vector_sink_f.h>

namespace gr {
  namespace digitizers {

  void
  qa_signal_averager::single_input_test()
  {
    unsigned num_of_repeats = 8;
    auto top = gr::make_top_block("single_input_test");

    std::vector<float> simple({1, 2, 3, 4, 5, 6, 5, 4, 3, 2});
    std::vector<float> vec;
    for(unsigned i = 0; i < num_of_repeats; i++){
      vec.insert(vec.end(), simple.begin(), simple.end());
      for(auto it = simple.begin(); it != simple.end(); ++it) { *it *= 2; }
    }

    auto src = blocks::vector_source_f::make(vec);
    auto avg = signal_averager::make(1, simple.size());
    auto snk = blocks::vector_sink_f::make(1);

    top->connect(src, 0, avg, 0);
    top->connect(avg, 0, snk, 0);

    top->run();
    auto data = snk->data();
    CPPUNIT_ASSERT(data.size() == num_of_repeats);
    float desired_avg = data.at(0);
    for(auto it = data.begin(); it != data.end(); ++it) {
      CPPUNIT_ASSERT_DOUBLES_EQUAL(desired_avg, *it, 0.0001);
      desired_avg *= 2;
    }

  }

  void
  qa_signal_averager::multiple_input_test()
  {
    unsigned number_of = 8;
    auto top = gr::make_top_block("single_input_test");

    std::vector<float> simple({1, 2, 3, 4, 5, 6, 5, 4, 3, 2});
    std::vector<float> vec0, vec1;
    for(unsigned i = 0; i < number_of; i++){
      vec0.insert(vec0.end(), simple.begin(), simple.end());
      for(auto it = simple.begin(); it != simple.end(); ++it) { *it *= 2.0; }
      vec1.insert(vec1.end(), simple.begin(), simple.end());
    }

    auto src0 = blocks::vector_source_f::make(vec0);
    auto src1 = blocks::vector_source_f::make(vec1);
    auto avg = signal_averager::make(2, simple.size());
    auto snk0 = blocks::vector_sink_f::make(1);
    auto snk1 = blocks::vector_sink_f::make(1);

    top->connect(src0, 0, avg, 0);
    top->connect(src1, 0, avg, 1);
    top->connect(avg, 0, snk0, 0);
    top->connect(avg, 1, snk1, 0);

    top->run();
    auto data0 = snk0->data();
    auto data1 = snk1->data();
    CPPUNIT_ASSERT(data0.size() == number_of);
    CPPUNIT_ASSERT(data1.size() == number_of);
    float desired_avg = data0.at(0);
    for(size_t i = 0; i < number_of; i++) {
      CPPUNIT_ASSERT_DOUBLES_EQUAL(desired_avg, data0.at(i), 0.0001);
      desired_avg *= 2;
      CPPUNIT_ASSERT_DOUBLES_EQUAL(desired_avg, data1.at(i), 0.0001);
    }

  }

  } /* namespace digitizers */
} /* namespace gr */

