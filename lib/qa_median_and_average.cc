/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#include <gnuradio/top_block.h>

#include <gnuradio/attributes.h>
#include <cppunit/TestAssert.h>
#include "qa_median_and_average.h"
#include <digitizers/median_and_average.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/blocks/vector_sink_f.h>

#include <thread>
#include <chrono>

namespace gr {
  namespace digitizers {

    void
    qa_median_and_average::basic_median_and_average()
    {
      // Put test here
      auto top = gr::make_top_block("basic_median_and_average");
      std::vector<float> data({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
      int vec_size = data.size();
      auto src = blocks::vector_source_f::make(data, false, vec_size);
      auto snk = blocks::vector_sink_f::make(vec_size);
      auto flt = digitizers::median_and_average::make(vec_size, 3, 2);

      top->connect(src, 0, flt, 0);
      top->connect(flt, 0, snk, 0);

      top->run();

      auto results = snk->data();
      CPPUNIT_ASSERT_EQUAL(data.size(), results.size());
      for(auto val : results) {
        CPPUNIT_ASSERT(val > 0.0);
        CPPUNIT_ASSERT(val < 10.0);
      }
    }

  } /* namespace digitizers */
} /* namespace gr */

