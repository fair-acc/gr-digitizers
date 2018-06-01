/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "block_demux_impl.h"

namespace gr {
  namespace digitizers {

    block_demux::sptr
    block_demux::make(int bit_to_keep)
    {
      return gnuradio::get_initial_sptr
        (new block_demux_impl(bit_to_keep));
    }

    /*
     * The private constructor
     */
    block_demux_impl::block_demux_impl(int bit_to_keep)
      : gr::sync_block("block_demux",
              gr::io_signature::make(1, 1, sizeof(char)),
              gr::io_signature::make(1, 1, sizeof(float))),
              d_bit(bit_to_keep)
    {}

    /*
     * Our virtual destructor.
     */
    block_demux_impl::~block_demux_impl()
    {
    }

    int
    block_demux_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const char *in = (const char *) input_items[0];
      float *out = (float *) output_items[0];

      for(int i = 0; i < noutput_items; i++) {
        out[i] = ((in[i] >> d_bit) & 1);
      }

      return noutput_items;
    }

  } /* namespace digitizers */
} /* namespace gr */

