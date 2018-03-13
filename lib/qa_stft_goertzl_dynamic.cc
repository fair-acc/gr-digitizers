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
#include "qa_stft_goertzl_dynamic.h"
#include <digitizers/stft_goertzl_dynamic.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/blocks/vector_sink_f.h>
#include <digitizers/tags.h>

namespace gr {
  namespace digitizers {

  void
  qa_stft_goertzl_dynamic::basic_test()
  {
    auto top = gr::make_top_block("basic_goertzl_non-dynamic");
    int win_size = 1024;
    double freq = 500;
    double samp_rate = 10000;
    int nbins = 100;
    double apparent_delta = ((1.0 * win_size) / samp_rate);
    std::vector<float> data;
    for(int i = 0; i<win_size*64; i++) {
      data.push_back(sin(2.0 * M_PI * i * freq / samp_rate));
    }
    auto src = blocks::vector_source_f::make(data);
    auto snk0 = blocks::vector_sink_f::make(nbins);
    auto snk1 = blocks::vector_sink_f::make(nbins);
    auto snk2 = blocks::vector_sink_f::make(nbins);

    std::vector<double> center({800, 500});
    std::vector<double> width({200, 1000});
    auto stft = stft_goertzl_dynamic::make(samp_rate, apparent_delta, win_size, nbins, 0.1, center, width);
    top->connect(src, 0, stft, 0);
    top->connect(stft, 0, snk0, 0);
    top->connect(stft, 1, snk1, 0);
    top->connect(stft, 2, snk2, 0);

    top->run();

    auto mag_data = snk0->data();
    auto mag_tags = snk0->tags();
    auto fq_low_data = snk2->data();

    CPPUNIT_ASSERT_EQUAL(size_t(64 * nbins), mag_data.size());

    double expected =(freq - ( center.back() - width.back() / 2.0)) /
              (width.back());
    for(int i = 1; i < 64; i++) {
      int max_j = i * nbins;
      for(int j = 0; j < nbins; j++){
        if(mag_data[i * nbins + j] > mag_data[max_j]) {
          max_j = i * nbins + j;
        }
      }
      max_j -= i * nbins;

      CPPUNIT_ASSERT_DOUBLES_EQUAL(expected, ((max_j * 1.0) / nbins), 0.02);
    }
  }

  void
  qa_stft_goertzl_dynamic::tagged_test()
  {
    auto top = gr::make_top_block("tagged_goertzl_dynamic");
    int win_size = 1024;
    double freq = 500;
    double samp_rate = 10000;
    int nbins = 100;
    double apparent_delta = ((1.0 * win_size) / samp_rate);
    double effective_window = (0.95 * win_size) / (samp_rate * 1.0);

    //data create
    std::vector<float> data;
    for(int i = 0; i<win_size*64; i++) {
      data.push_back(sin(2.0 * M_PI * i * freq / samp_rate));
    }

    //tag create
    tag_t beam_in = make_trigger_tag(0, 0, 0, freq, 0);
    std::vector<tag_t> tags;
    tags.push_back(beam_in);

    auto src = blocks::vector_source_f::make(data, false, 1, tags);
    auto snk0 = blocks::vector_sink_f::make(nbins);
    auto snk1 = blocks::vector_sink_f::make(nbins);
    auto snk2 = blocks::vector_sink_f::make(nbins);

    std::vector<double> center({1000, 2500});
    std::vector<double> width({2000, 5000});
    auto stft = stft_goertzl_dynamic::make(samp_rate, apparent_delta, win_size, nbins, effective_window, center, width);
    top->connect(src, 0, stft, 0);
    top->connect(stft, 0, snk0, 0);
    top->connect(stft, 1, snk1, 0);
    top->connect(stft, 2, snk2, 0);

    top->run();

    auto mag_data = snk0->data();
    auto mag_tags = snk0->tags();

    CPPUNIT_ASSERT_EQUAL(size_t(64 * nbins), mag_data.size());

    //tag check
    CPPUNIT_ASSERT_EQUAL(size_t(1), mag_tags.size());
    CPPUNIT_ASSERT_EQUAL(uint64_t(0), mag_tags.front().offset);

    double expected0 =(freq - ( center.front() - width.front() / 2.0)) /
        width.front();
    double expected1 =(freq - ( center.back() - width.back() / 2.0)) /
        width.back();

    int max_j = 0;
    for(int j = 0; j < nbins; j++){
      if(mag_data[j] > mag_data[max_j]) {
        max_j = j;
      }
    }
    CPPUNIT_ASSERT_DOUBLES_EQUAL(expected0, ((max_j * 1.0) / nbins), 0.02);

    max_j = nbins;
    for(int j = nbins; j < 2 * nbins; j++){
      if(mag_data[j] > mag_data[max_j]) {
        max_j = j;
      }
    }
    max_j -= nbins;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(expected1, ((max_j * 1.0) / nbins), 0.02);

    max_j = 2 * nbins;
    for(int j = 2 * nbins; j < 3 * nbins; j++){
      if(mag_data[j] > mag_data[max_j]) {
        max_j = j;
      }
    }
    max_j -= 2 * nbins;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(expected1, ((max_j * 1.0) / nbins), 0.02);
  }

  } /* namespace digitizers */
} /* namespace gr */

