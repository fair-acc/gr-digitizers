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
      CPPUNIT_ASSERT(results.size() != 0);
      for(auto val : results) {
        CPPUNIT_ASSERT(val > 0);
      }
    }

  } /* namespace digitizers */
} /* namespace gr */

