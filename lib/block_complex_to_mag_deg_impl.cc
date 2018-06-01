/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "block_complex_to_mag_deg_impl.h"
#include <volk/volk.h>
#include <gnuradio/math.h>

namespace gr {
  namespace digitizers {

    block_complex_to_mag_deg::sptr
    block_complex_to_mag_deg::make(int vec_len)
    {
      return gnuradio::get_initial_sptr
        (new block_complex_to_mag_deg_impl(vec_len));
    }

    /*
     * The private constructor
     */
    block_complex_to_mag_deg_impl::block_complex_to_mag_deg_impl(int vec_len)
      : gr::sync_block("block_complex_to_mag_deg",
              gr::io_signature::make(1, 1, sizeof(gr_complex) * vec_len),
              gr::io_signature::make(2, 2, sizeof(float) * vec_len)),
              d_vec_len(vec_len)
    {}

    /*
     * Our virtual destructor.
     */
    block_complex_to_mag_deg_impl::~block_complex_to_mag_deg_impl()
    {
    }

    int
    block_complex_to_mag_deg_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const gr_complex *in = (const gr_complex *) input_items[0];
      float *mags = (float *) output_items[0];
      float *degs = (float *) output_items[1];

      double rad2deg = 180.0 / M_PI;
      int noi = noutput_items * d_vec_len;

      volk_32fc_magnitude_32f_u(mags, in, noi);
      for(int i = 0; i < noi; i++) {
        degs[i] = rad2deg * gr::fast_atan2f(in[i]);
      }

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace digitizers */
} /* namespace gr */

