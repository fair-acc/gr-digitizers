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
#include "qa_block_scaling_offset.h"
#include <digitizers/block_scaling_offset.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/blocks/vector_sink_f.h>

namespace gr {
  namespace digitizers {

    void
    qa_block_scaling_offset::scale_and_offset()
    {
      auto top = gr::make_top_block("scale_and_offset");

      double scale = 1.5;
      double offset = 2;
      int n = 30;
      std::vector<float> data;
      for(int i = 0; i < n; i++){  data.push_back(i); }

      auto src0 = blocks::vector_source_f::make(data);
      auto src1 = blocks::vector_source_f::make(data);
      auto snk0 = blocks::vector_sink_f::make(1);
      auto snk1 = blocks::vector_sink_f::make(1);
      auto bso = block_scaling_offset::make(scale, offset);

      top->connect(src0, 0, bso, 0);
      top->connect(bso, 0, snk0, 0);
      top->connect(src1, 0, bso, 1);
      top->connect(bso, 1, snk1, 0);

      top->run();
      auto result0 = snk0->data();
      auto result1 = snk1->data();
      CPPUNIT_ASSERT_EQUAL(data.size(), result0.size());
      CPPUNIT_ASSERT_EQUAL(data.size(), result1.size());

      for(int i = 0; i < n; i++) {
        float should_be0 = ((data.at(i) * scale) + offset);
        float should_be1 = (data.at(i) * scale);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(should_be0, result0.at(i), 0.0001);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(should_be1, result1.at(i), 0.0001);
      }
    }

  } /* namespace digitizers */
} /* namespace gr */

