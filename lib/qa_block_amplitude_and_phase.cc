/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */
#include <gnuradio/top_block.h>


#include <gnuradio/attributes.h>
#include <cppunit/TestAssert.h>
#include "qa_block_amplitude_and_phase.h"
#include <digitizers/block_amplitude_and_phase.h>
#include <digitizers/block_complex_to_mag_deg.h>
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
      for(int i = 0; i < 16*samp_rate; i++) {
        sine.push_back(sin(desired_freq * 2.0 * M_PI * static_cast<double>(i)/static_cast<double>(samp_rate)));
        cosine.push_back(cos(desired_freq * 2.0 * M_PI * static_cast<double>(i)/static_cast<double>(samp_rate)));
      }
      auto src = blocks::vector_source_f::make(sine);
      auto ref = blocks::vector_source_f::make(cosine);
      auto blk = block_amplitude_and_phase::make(samp_rate, 0, 5, 1.0, 1024, 50, 1024);
      auto c2md = block_complex_to_mag_deg::make(1);
      auto snk0 = blocks::vector_sink_f::make(1);
      auto snk1 = blocks::vector_sink_f::make(1);
      auto snk2 = blocks::vector_sink_f::make(1);

      top->connect(src, 0, blk, 0);
      top->connect(ref, 0, blk, 1);

      top->connect(blk, 0, c2md, 0);
      top->connect(c2md, 0, snk0, 0);
      top->connect(c2md, 1, snk1, 0);

      top->run();

      auto phase = snk1->data();
      auto ampl = snk0->data();

      CPPUNIT_ASSERT(phase.size() != 0);
      //start at an offset, because the block needs to stabilize the output
      for(int i = 2000; i < (int)phase.size(); i++) {
        CPPUNIT_ASSERT_DOUBLES_EQUAL(-90.0, phase.at(i), 0.05);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(0.5, ampl.at(i), 0.05);
      }
    }

  } /* namespace digitizers */
} /* namespace gr */

