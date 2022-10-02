/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */
#include <gnuradio/top_block.h>



#include <gnuradio/attributes.h>
#include <cppunit/TestAssert.h>
#include "qa_stream_to_vector_overlay_ff.h"
#include <digitizers/stream_to_vector_overlay_ff.h>
#include <gnuradio/blocks/vector_source.h>
#include <gnuradio/blocks/vector_sink.h>


namespace gr {
  namespace digitizers {
    void test_different_params(int size, int delta)
    {
      auto top = gr::make_top_block("single_input_test");

      std::vector<float> vec;
      for(int i = 0; i < (size * delta * 8); i++){
        vec.push_back(i);
      }

      auto src = blocks::vector_source_f::make(vec);
      auto blk = stream_to_vector_overlay_ff::make(size, 1, delta);
      auto snk = blocks::vector_sink_f::make(size);

      top->connect(src, 0, blk, 0);
      top->connect(blk, 0, snk, 0);

      top->run();

      auto data = snk->data();

      for(int i = 0; i < (int)data.size()/size; i ++) {
        for(int j = 0; j < size; j++){
          CPPUNIT_ASSERT_DOUBLES_EQUAL((i * delta + j)*1.0, data.at(i*size + j), 0.02);
        }
      }
    }
    void
    qa_stream_to_vector_overlay_ff::t1()
    {
      for(int i = 1; i < 10; i++) {
        for(int j = 1; j < 10; j++) {
          test_different_params(i, j);
        }
      }
    }

  } /* namespace digitizers */
} /* namespace gr */

