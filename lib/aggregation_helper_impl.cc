/* -*- c++ -*- */
/* 
 * Copyright 2017 <+YOU OR YOUR COMPANY+>.
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

#include <cmath>
#include <gnuradio/io_signature.h>
#include "aggregation_helper_impl.h"

namespace gr {
  namespace digitizers {

    aggregation_helper::sptr
    aggregation_helper::make(int decim, float sigmaMult)
    {
      return gnuradio::get_initial_sptr
        (new aggregation_helper_impl(decim, sigmaMult));
    }

    /*
     * The private constructor
     */
    aggregation_helper_impl::aggregation_helper_impl(int decim, float sigma_mult)
      : gr::block("aggregation_helper",
              gr::io_signature::make(3, 3, sizeof(float)),
              gr::io_signature::make(1, 1, sizeof(float))),
        d_sigma_mult_fact(sigma_mult),
        d_out_idx(decim),
        d_jump_size(decim)
    {
      set_tag_propagation_policy(tag_propagation_policy_t::TPP_DONT);
    }

    /*
     * Our virtual destructor.
     */
    aggregation_helper_impl::~aggregation_helper_impl()
    {
    }

    int
    aggregation_helper_impl::general_work(int noutput_items,
        gr_vector_int &ninput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const float *a = (const float *) input_items[0];
      const float *b = (const float *) input_items[1];
      const float *c = (const float *) input_items[2];
      float *out = (float *) output_items[0];

      // Do <+signal processing+>
      int nr_input_items = *std::min_element(ninput_items.begin(), ninput_items.end());
      int i, j;
      for(i = 0, j = 0; i < nr_input_items && j < noutput_items; i ++) {
        d_out_idx--;
        if(d_out_idx <= 0) {
          out[j] = std::sqrt(std::fabs(a[i] - b[i]*b[i]) + (d_sigma_mult_fact * c[i]*c[i]));
          d_out_idx = d_jump_size;
          j++;
        }
      }
      //Sigma is not supposed to include tags so they are ignored.
      consume_each(nr_input_items);
      return j;
    }

    void
    aggregation_helper_impl::update_design(float sigma_mult)
    {
      d_sigma_mult_fact = sigma_mult;
    }
  } /* namespace digitizers */
} /* namespace gr */

