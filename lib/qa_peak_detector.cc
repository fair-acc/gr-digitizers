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
#include "qa_peak_detector.h"
#include <digitizers/peak_detector.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/blocks/vector_sink_f.h>
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
      auto max = blocks::vector_sink_f::make(1);
      auto stdev = blocks::vector_sink_f::make(1);
      auto detect = digitizers::peak_detector::make(100, vec_size, 5, 8, 4);
      auto flt = digitizers::median_and_average::make(vec_size, 3, 2);

      top->connect(src, 0, detect, 0);
      top->connect(src, 0, flt, 0);
      top->connect(flt, 0, detect, 1);
      top->connect(detect, 0, max, 0);
      top->connect(detect, 1, stdev, 0);

      top->run();

      auto maximum = max->data();
      auto stdevs = stdev->data();
      CPPUNIT_ASSERT(maximum.size() != 0);
      CPPUNIT_ASSERT(stdevs.size() != 0);

      CPPUNIT_ASSERT_DOUBLES_EQUAL(22.72, maximum.at(0), 0.02);
      CPPUNIT_ASSERT_DOUBLES_EQUAL(23.16, stdevs.at(0), 0.02);
    }

  } /* namespace digitizers */
} /* namespace gr */

