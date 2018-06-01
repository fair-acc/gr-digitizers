/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#include <gnuradio/top_block.h>



#include <gnuradio/attributes.h>
#include <cppunit/TestAssert.h>
#include "qa_stft_goertzl_dynamic_decimated.h"
#include <digitizers/stft_goertzl_dynamic_decimated.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/blocks/vector_sink_f.h>

namespace gr {
  namespace digitizers {

    void
    qa_stft_goertzl_dynamic_decimated::t1()
    {
      auto top = gr::make_top_block("basic_goertzl_dynamic");

      int win_size = 1024;
      double freq = 512;
      double samp_rate = 10000;
      int nbins = 100;
      std::vector<float> min_v;
      std::vector<float> max_v;
      std::vector<float> sig_v;
      for(int i = 0; i<win_size*2; i++) {
        sig_v.push_back(sin(2.0 * M_PI * i * freq / samp_rate));
        if(i < win_size) {
          min_v.push_back(0);
          max_v.push_back(2 * freq);
        }
        else {
          min_v.push_back(freq);
          max_v.push_back(samp_rate/2);
        }
      }
      auto src = blocks::vector_source_f::make(sig_v);
      auto min = blocks::vector_source_f::make(min_v);
      auto max = blocks::vector_source_f::make(max_v);

      auto snk0 = blocks::vector_sink_f::make(nbins);
      auto snk1 = blocks::vector_sink_f::make(nbins);
      auto snk2 = blocks::vector_sink_f::make(nbins);
      auto stft = stft_goertzl_dynamic_decimated::make(samp_rate, (1.0 * win_size) / samp_rate, win_size, nbins);

      top->connect(src, 0, stft, 0);
      top->connect(min, 0, stft, 1);
      top->connect(max, 0, stft, 2);
      top->connect(stft, 0, snk0, 0);
      top->connect(stft, 1, snk1, 0);
      top->connect(stft, 2, snk2, 0);

      top->run();

      auto mag_data = snk0->data();

      CPPUNIT_ASSERT_EQUAL(size_t(2 * nbins), mag_data.size());


      size_t max_i = 0;

      for(int i = 0; i < nbins; i++){
        if(mag_data.at(i) > mag_data.at(max_i)) { max_i = i; }
      }
      CPPUNIT_ASSERT_EQUAL(size_t(49), max_i);
      //next window maximum
      max_i = nbins;
      for(int i = nbins; i < 2 * nbins; i++){
        if(mag_data.at(i) > mag_data.at(max_i)) { max_i = i; }
      }
      CPPUNIT_ASSERT_EQUAL(size_t(100), max_i);
    }

  } /* namespace digitizers */
} /* namespace gr */

