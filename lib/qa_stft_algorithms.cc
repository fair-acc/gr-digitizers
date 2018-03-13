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
#include "qa_stft_algorithms.h"
#include "digitizers/stft_algorithms.h"
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/blocks/vector_sink_f.h>


namespace gr {
  namespace digitizers {

  void
  qa_stft_algorithms::test_stft_fft()
  {
    auto top = gr::make_top_block("basic_connection");
    std::vector<float> data({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16});
    auto src = blocks::vector_source_f::make(data);
    auto snk0 = blocks::vector_sink_f::make(8);
    auto snk1 = blocks::vector_sink_f::make(8);
    auto fft = stft_algorithms::make(1000, 0.01, 8, 0, 0, 0, 500, 16);

    top->connect(src, 0, fft, 0);
    top->connect(fft, 0, snk0, 0);
    top->connect(fft, 1, snk1, 0);

    //run this circuit
    top->run();

    auto ampl = snk0->data();
    auto phas = snk1->data();
    CPPUNIT_ASSERT_EQUAL((int)phas.size(), 8);
  }

  } /* namespace digitizers */
} /* namespace gr */

