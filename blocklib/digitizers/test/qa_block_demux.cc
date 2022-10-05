/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#include <gnuradio/top_block.h>

#include <gnuradio/attributes.h>
#include <cppunit/TestAssert.h>
#include "qa_block_demux.h"
#include <digitizers/block_demux.h>
#include <gnuradio/blocks/vector_source.h>
#include <gnuradio/blocks/vector_sink.h>

namespace gr {
  namespace digitizers {

    void
    qa_block_demux::passes_only_desired()
    {
      std::vector<unsigned char> vals = {0, 1, 2, 3, 4, 5, 6, 7};
      auto top = gr::make_top_block("basic_connection");
      auto demux = digitizers::block_demux::make(0);
      auto src =gr::blocks::vector_source_b::make(vals);
      auto snk = gr::blocks::vector_sink_f::make(1);
      top->connect(src, 0, demux, 0);
      top->connect(demux, 0, snk, 0);

      top->run();
      auto data = snk->data();

      CPPUNIT_ASSERT(data.size() != 0);
      for(int i = 0; i < static_cast<int>(data.size()); i++) {
        if(i % 2 == 0){
          CPPUNIT_ASSERT(data[i] == 0.0);
        }
        else {
          CPPUNIT_ASSERT(data[i] == 1.0);
        }
      }
    }

  } /* namespace digitizers */
} /* namespace gr */

