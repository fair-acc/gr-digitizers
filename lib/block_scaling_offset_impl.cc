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
#include "block_scaling_offset_impl.h"

namespace gr {
  namespace digitizers {

    block_scaling_offset::sptr
    block_scaling_offset::make(double scale, double offset)
    {
      return gnuradio::get_initial_sptr
        (new block_scaling_offset_impl(scale, offset));
    }

    /*
     * The private constructor
     */
    block_scaling_offset_impl::block_scaling_offset_impl(double scale, double offset)
      : gr::sync_block("block_scaling_offset",
              gr::io_signature::make(2, 2, sizeof(float)),
              gr::io_signature::make(2, 2, sizeof(float))),
              d_scale(scale),
              d_offset(offset)
    {}

    block_scaling_offset_impl::~block_scaling_offset_impl()
    {
    }

    int
    block_scaling_offset_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const float *in_sig = (const float *) input_items[0];
      const float *in_err = (const float *) input_items[1];
      float *out_sig = (float *) output_items[0];
      float *out_err = (float *) output_items[1];

      for(int i = 0; i < noutput_items; i++) {
        out_sig[i] = (in_sig[i] * d_scale) + d_offset;
        out_err[i] = in_err[i] * d_scale;
      }

      return noutput_items;
    }

    void
    block_scaling_offset_impl::update_design(double scale, double offset)
    {
      d_scale = scale;
      d_offset = offset;
    }

  } /* namespace digitizers */
} /* namespace gr */

