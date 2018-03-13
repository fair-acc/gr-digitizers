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
#include "qa_block_amplitude_and_phase.h"
#include <digitizers/block_amplitude_and_phase.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/blocks/vector_sink_f.h>

#include <math.h>

namespace gr {
  namespace digitizers {

    void
    qa_block_amplitude_and_phase::find_ampl_phase()
    {
      int samp_rate = 200000;
      double desired_freq = 6000.0;
      auto top = gr::make_top_block("find_freq_ampl_phase");

      std::vector<float> sine;
      std::vector<float> cosine;
      for(int i = 0; i < 16*samp_rate; i++)
      {
        sine.push_back(sin(desired_freq * 2.0 * M_PI * static_cast<double>(i)/static_cast<double>(samp_rate)));
        cosine.push_back(cos(desired_freq * 2.0 * M_PI * static_cast<double>(i)/static_cast<double>(samp_rate)));
      }
      auto src = blocks::vector_source_f::make(sine);
      auto ref = blocks::vector_source_f::make(cosine);
      auto blk = block_amplitude_and_phase::make(samp_rate, 0, 5, 1.0, 1024, 50, 1024);
      auto snk0 = blocks::vector_sink_f::make(1);
      auto snk1 = blocks::vector_sink_f::make(1);
      auto snk2 = blocks::vector_sink_f::make(1);

      top->connect(src, 0, blk, 0);
      top->connect(ref, 0, blk, 1);

      top->connect(blk, 0, snk0, 0);
      top->connect(blk, 1, snk1, 0);

      top->run();

      auto phase = snk0->data();
      auto ampl = snk1->data();

      CPPUNIT_ASSERT(phase.size() != 0);
      for(int i = 2000; i < samp_rate; i++) {
        CPPUNIT_ASSERT_DOUBLES_EQUAL(0.5, phase.at(i), 0.05);
      }
    }

  } /* namespace digitizers */
} /* namespace gr */

