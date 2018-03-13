/* -*- c++ -*- */
/* 
 * Copyright 2018 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#include <gnuradio/top_block.h>



#include <gnuradio/attributes.h>
#include <cppunit/TestAssert.h>
#include "qa_stream_to_vector_overlay_ff.h"
#include <digitizers/stream_to_vector_overlay_ff.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/blocks/vector_sink_f.h>


namespace gr {
  namespace digitizers {
    void test_different_params(int size, int delta)
    {
      auto top = gr::make_top_block("single_input_test");

      std::vector<float> vec;
      for(unsigned i = 0; i < size * delta * 8; i++){
        vec.push_back(i);
      }

      auto src = blocks::vector_source_f::make(vec);
      auto blk = stream_to_vector_overlay_ff::make(size, delta);
      auto snk = blocks::vector_sink_f::make(size);

      top->connect(src, 0, blk, 0);
      top->connect(blk, 0, snk, 0);

      top->run();

      auto data = snk->data();

      for(int i = 0; i < data.size()/size; i ++) {
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

