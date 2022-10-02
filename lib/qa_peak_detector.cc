/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#include <gnuradio/top_block.h>


#include <gnuradio/attributes.h>
#include <cppunit/TestAssert.h>
#include "qa_peak_detector.h"
#include <digitizers/peak_detector.h>
#include <gnuradio/blocks/vector_source.h>
#include <gnuradio/blocks/vector_sink.h>
#include <digitizers/median_and_average.h>

#include <thread>
#include <chrono>

namespace gr {
  namespace digitizers {

    void
    qa_peak_detector::basic_peak_find()
    {
      auto top = gr::make_top_block("basic_peak_find");
      std::vector<float> data({1, 2, 3, 4, 5, 6, 5, 4, 3, 2, 1});
      int vec_size = data.size();
      auto src = blocks::vector_source_f::make(data, false, vec_size);
      auto flow = blocks::vector_source_f::make(std::vector<float>{25});
      auto fup = blocks::vector_source_f::make(std::vector<float>{40});
      auto max = blocks::vector_sink_f::make(1);
      auto stdev = blocks::vector_sink_f::make(1);
      auto detect = digitizers::peak_detector::make(100.0, vec_size, /* 5, 8,*/ 4);
      auto flt = digitizers::median_and_average::make(vec_size, 3, 2);

      top->connect(src, 0,  detect, 0);
      top->connect(src, 0, flt, 0);
      top->connect(flt, 0, detect, 1);
      top->connect(flow, 0, detect, 2);
      top->connect(fup, 0, detect, 3);
      top->connect(detect, 0, max, 0);
      top->connect(detect, 1, stdev, 0);

      top->run();

      auto maximum = max->data();
      auto stdevs = stdev->data();
      CPPUNIT_ASSERT(maximum.size() != 0);
      CPPUNIT_ASSERT(stdevs.size() != 0);

      CPPUNIT_ASSERT_DOUBLES_EQUAL(22.72, maximum.at(0), 0.02);
      CPPUNIT_ASSERT_DOUBLES_EQUAL(46.32, stdevs.at(0), 0.02);
    }

  } /* namespace digitizers */
} /* namespace gr */

