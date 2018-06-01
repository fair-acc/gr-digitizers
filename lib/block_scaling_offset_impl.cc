/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
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
        out_sig[i] = (in_sig[i] * d_scale) - d_offset;
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

