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
#include "median_and_average_impl.h"

namespace gr {
  namespace digitizers {

    median_and_average::sptr
    median_and_average::make(int vec_len, int n_med, int n_lp)
    {
      return gnuradio::get_initial_sptr
        (new median_and_average_impl(vec_len, n_med, n_lp));
    }

    /*
     * The private constructor
     */
    median_and_average_impl::median_and_average_impl(int vec_len, int n_med, int n_lp)
      : gr::block("median_and_average",
        gr::io_signature::make(1, 1, sizeof(float) * vec_len),
        gr::io_signature::make(1, 1, sizeof(float) * vec_len)),
        d_median(n_med), d_average(n_lp), d_vec_len(vec_len),
        d_median_prewindow((n_med + n_lp)/2)
    {}

    /*
     * Our virtual destructor.
     */
    median_and_average_impl::~median_and_average_impl()
    {
    }

    void
    median_and_average_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      ninput_items_required[0] = noutput_items;
    }

    int
    median_and_average_impl::general_work (int noutput_items,
      gr_vector_int &ninput_items,
      gr_vector_const_void_star &input_items,
      gr_vector_void_star &output_items)
    {
      const float *in = (const float *) input_items[0];
      float *out = (float *) output_items[0];

      if(ninput_items[0] <= 0 || noutput_items <= 0){
        return 0;
      }

      //calculate median of samples and average it.
      int o =0;
      for(int i = 0; i < d_vec_len; i++) {
        float avg = d_average.add(d_median.add(in[i]));
        if(i<d_median_prewindow)
          continue;
        out[o++] = avg;
      }
      for(int i = 0; i < d_median_prewindow; i++) {
        out[o++] = d_average.add(d_median.add(in[i]));
      }
      consume_each (1);

      // Tell runtime system how many output items we produced.
      return 1;
    }

  } /* namespace digitizers */
} /* namespace gr */

