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
#include "qa_block_spectral_peaks.h"
#include <digitizers/block_spectral_peaks.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/blocks/vector_sink_f.h>
#include "utils.h"

namespace gr {
  namespace digitizers {

    void
    qa_block_spectral_peaks::test_spectral_peaks()
    {
      auto top = gr::make_top_block("basic_connection");
      int vec_len = 1024;
      int n_med = 5;
      int n_avg = 5;
      std::vector<float> cycle;
      //simple peak in the middle
      for(int i = 0; i<vec_len; i ++) {
        if(i < vec_len/2) { cycle.push_back(i); }
        else { cycle.push_back(vec_len - i); }
      }
      //spike middle
      int mult = 1;
      for(int i = vec_len/2 - (n_avg+n_med); i < vec_len/2 + (n_avg+n_med); i++) {
        cycle[i] *= mult;
        if(i<vec_len/2) { mult++; }
        else  { mult--; }
      }
      auto src = blocks::vector_source_f::make(cycle, false, vec_len);
      auto snk0 = blocks::vector_sink_f::make(vec_len);
      auto snk1 = blocks::vector_sink_f::make(1);
      auto snk2 = blocks::vector_sink_f::make(1);
      auto spec = digitizers::block_spectral_peaks::make(32000, vec_len, 0, 16000, 5, 15, 30);

      top->connect(src, 0, spec, 0);
      top->connect(spec, 0, snk0, 0);
      top->connect(spec, 1, snk1, 0);
      top->connect(spec, 2, snk2, 0);

      top->run();


      auto med = snk0->data();
      auto max = snk1->data();
      auto stdev = snk2->data();

      CPPUNIT_ASSERT_EQUAL(cycle.size(), med.size());
      CPPUNIT_ASSERT_EQUAL(size_t(1), max.size());
      CPPUNIT_ASSERT_DOUBLES_EQUAL(8000.0, max.at(0), 0.02);
      CPPUNIT_ASSERT_DOUBLES_EQUAL(150.0, stdev.at(0), 10);

    }

  } /* namespace digitizers */
} /* namespace gr */

