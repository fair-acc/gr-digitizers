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
              gr::io_signature::make(1, 1, sizeof(char))),
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
      char *out = (char *) output_items[0];

      for(int i = 0; i < noutput_items; i++) {
        out[i] = ((in[i] >> d_bit) & 1);
      }

      return noutput_items;
    }

  } /* namespace digitizers */
} /* namespace gr */

