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
#include "qa_freq_estimator.h"
#include <digitizers/freq_estimator.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/blocks/vector_sink_f.h>
#include <thread>
#include <chrono>

namespace gr {
  namespace digitizers {

    void
    qa_freq_estimator::basic_frequency_estimation()
    {
      int sig_avg_window = 4;
      int freq_avg_window = 10;
      auto top = gr::make_top_block("basic_connection");
      // Put test here
      std::vector<float> cycle({0,1,2,3,4,5,6,7,8,9,8,7,6,5,4,3,2,1,0,-1,-2,-3,-4,-5,-6,-7,-8,-9,-8,-7,-6,-5,-4,-3,-2,-1});
      std::vector<float> sig;
      for(int i = 0; i < 100; i++) {
        sig.insert(sig.end(), cycle.begin(), cycle.end());
      }
      auto sine =gr::blocks::vector_source_f::make(sig);
      auto freq = digitizers::freq_estimator::make(36000, sig_avg_window, freq_avg_window, 1);
      auto sink = blocks::vector_sink_f::make(1);

      top->connect(sine,0,freq, 0);
      top->connect(freq,0, sink, 0);

      top->run();
      auto data = sink->data();
      CPPUNIT_ASSERT(data.size() > 0);
      for(size_t i = freq_avg_window * cycle.size(); i < data.size(); i++){
        CPPUNIT_ASSERT_DOUBLES_EQUAL(1000, data.at(i), 10.0);
      }
    }

  } /* namespace digitizers */
} /* namespace gr */

