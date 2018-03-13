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
#include "decimate_and_adjust_timebase_impl.h"
#include <digitizers/tags.h>
#include "utils.h"

namespace gr {
  namespace digitizers {

    decimate_and_adjust_timebase::sptr
    decimate_and_adjust_timebase::make(int decimation)
    {
      return gnuradio::get_initial_sptr
        (new decimate_and_adjust_timebase_impl(decimation));
    }

    /*
     * The private constructor
     */
    decimate_and_adjust_timebase_impl::decimate_and_adjust_timebase_impl(int decimation)
      : gr::block("decimate_and_adjust_timebase",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(1, 1, sizeof(float))),
              d_decim(decimation), d_count(decimation)
    {}

    /*
     * Our virtual destructor.
     */
    decimate_and_adjust_timebase_impl::~decimate_and_adjust_timebase_impl()
    {
    }

    int
    decimate_and_adjust_timebase_impl::general_work(int noutput_items,
        gr_vector_int &ninput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const float *in = (const float *) input_items[0];
      float *out = (float *) output_items[0];
      int n_in = ninput_items[0];
      int in_idx = 0;
      int out_idx = 0;
      
      while (in_idx < ninput_items[0] && out_idx < noutput_items){
        d_count--;
        if (d_count <= 0){
          out[out_idx] = in[in_idx];
          out_idx++;
          d_count = d_decim;
        }
        in_idx++;
      }
      std::vector<gr::tag_t> tags;
      get_tags_in_range(tags, 0, nitems_read(0), nitems_read(0)+n_in);
      for(auto tag : tags) {
        tag.offset *= (1.0/d_decim);
        if(pmt::symbol_to_string(tag.key) == "timebase_info") {
          auto timebase = decode_timebase_info_tag(tag);
          tag = make_timebase_info_tag(timebase/d_decim);
        }
        add_item_tag(0, tag);
      }
      consume_each (in_idx);
      return out_idx;
    }

  } /* namespace digitizers */
} /* namespace gr */

