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
#include "qa_block_demux.h"
#include <digitizers/block_demux.h>
#include <gnuradio/blocks/vector_source_b.h>
#include <gnuradio/blocks/vector_sink_b.h>

namespace gr {
  namespace digitizers {

    void
    qa_block_demux::passes_only_desired()
    {
      std::vector<unsigned char> vals = {0,1,2,3,4,5,6,7};
      auto top = gr::make_top_block("basic_connection");
      auto demux = digitizers::block_demux::make(0);
      auto src =gr::blocks::vector_source_b::make(vals);
      auto snk = gr::blocks::vector_sink_b::make(1);
      top->connect(src, 0, demux, 0);
      top->connect(demux, 0, snk, 0);

      top->run();
      auto data = snk->data();

      CPPUNIT_ASSERT(data.size() != 0);
      for(int i = 0; i < static_cast<int>(data.size()); i++) {
        if(i % 2 == 0){
          CPPUNIT_ASSERT(data[i] == 0);
        }
        else {
          CPPUNIT_ASSERT(data[i] == 1);
        }
      }
    }

  } /* namespace digitizers */
} /* namespace gr */

