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
#include "qa_interlock_generation_ff.h"
#include <digitizers/interlock_generation_ff.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/blocks/vector_sink_f.h>
#include <digitizers/tags.h>

namespace gr {
  namespace digitizers {

    int count_interlock_calls = 0;
    void interlock(void *userdata)
    {
      count_interlock_calls++;
    }

    void
    qa_interlock_generation_ff::interlock_generation_test()
    {

// TODO: fix test


//      auto top = gr::make_top_block("interlock_generation_test");
//
//      int win_size = 16*8192;
//      double freq = 500;
//      double samp_rate = 10000;
//      double effective_window = 0.5;
//
//      //data create
//      std::vector<float> data;
//      for(int i = 0; i<win_size; i++) {
//        data.push_back(10.0 * cos(2.0 * M_PI * i * freq / samp_rate));
//      }
//
//      //tag create
//      tag_t beam_in = make_trigger_tag(0, 0, 0, freq, 0);
//      auto tag_info = decode_acq_info_tag(beam_in);
//      tag_info.timestamp = 1000000;
//      tag_info.last_beam_in_timestamp = 2000000;
//      beam_in = make_acq_info_tag(tag_info);
//      beam_in.offset = 1000;
//      std::vector<tag_t> tags;
//      tags.push_back(beam_in);
//
//      auto src = blocks::vector_source_f::make(data, false, 1, tags);
//      auto snk0 = blocks::vector_sink_f::make(1);
//      auto snk1 = blocks::vector_sink_f::make(1);
//      auto snk2 = blocks::vector_sink_f::make(1);
//
//      std::vector<double> center({1, 1});
//      std::vector<double> width({0.5, 4});
//      auto i_lk = interlock_generation_ff::make(-100, 100);
//      i_lk->set_interlock_callback(&interlock, nullptr);
//      top->connect(src, 0, i_lk, 0);
//      top->connect(i_lk, 0, snk0, 0);
//      top->connect(i_lk, 1, snk1, 0);
//      top->connect(i_lk, 2, snk2, 0);
//
//      top->run();
//
//      auto hi_data = snk2->data();
//      auto lo_data = snk1->data();
//
//      CPPUNIT_ASSERT_EQUAL(size_t(win_size), hi_data.size());
//      CPPUNIT_ASSERT_EQUAL(1, count_interlock_calls);
//      size_t start = beam_in.offset +
//          samp_rate * static_cast<double>(tag_info.last_beam_in_timestamp - tag_info.timestamp)/1000000000.0;
//      size_t end = start + samp_rate * effective_window;
//
//      double hi0 = center.front() + width.front()*0.5;
//      double hi1 = center.back() + width.back()*0.5;
//      double lo0 = center.front() - width.front()*0.5;
//      double lo1 = center.back() - width.back()*0.5;
//
//      for(size_t i = 0; i < size_t(win_size); i++) {
//        if(i >=start && i < end) {
//          double factor = static_cast<double>(i-start)/static_cast<double>(end-start);
//          CPPUNIT_ASSERT_DOUBLES_EQUAL(((1.0 - factor ) * hi0 + factor * hi1), hi_data.at(i), 0.05);
//          CPPUNIT_ASSERT_DOUBLES_EQUAL(((1.0 - factor ) * lo0 + factor * lo1), lo_data.at(i), 0.05);
//        }
//        else {
//          CPPUNIT_ASSERT_DOUBLES_EQUAL(hi1, hi_data.at(i), 0.05);
//          CPPUNIT_ASSERT_DOUBLES_EQUAL(lo1, lo_data.at(i), 0.05);
//
//        }
//      }

    }

  } /* namespace digitizers */
} /* namespace gr */

