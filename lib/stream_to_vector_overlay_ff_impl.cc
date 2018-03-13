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
#include "stream_to_vector_overlay_ff_impl.h"

namespace gr {
  namespace digitizers {

    stream_to_vector_overlay_ff::sptr
    stream_to_vector_overlay_ff::make(int vec_size, int overlay_dist)
    {
      return gnuradio::get_initial_sptr
        (new stream_to_vector_overlay_ff_impl(vec_size, overlay_dist));
    }

    /*
     * The private constructor
     */
    stream_to_vector_overlay_ff_impl::stream_to_vector_overlay_ff_impl(int vec_size, int overlay_dist)
      : gr::block("stream_to_vector_overlay_ff",
              gr::io_signature::make(1,1, sizeof(float)),
              gr::io_signature::make(1,1, sizeof(float) * vec_size)),
              d_delta_samps(overlay_dist),
              d_vec_size(vec_size),
              d_offset(0),
              d_tags_between()
    {
      set_tag_propagation_policy(TPP_DONT);
    }

    /*
     * Our virtual destructor.
     */
    stream_to_vector_overlay_ff_impl::~stream_to_vector_overlay_ff_impl()
    {
    }

    void
    stream_to_vector_overlay_ff_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      ninput_items_required[0] = d_vec_size;
    }

    void
    stream_to_vector_overlay_ff_impl::save_tags(int count)
    {
      std::vector<tag_t> this_tags;
      get_tags_in_range(this_tags, 0, nitems_read(0), nitems_read(0)+count);
      d_tags_between.insert(d_tags_between.end(), this_tags.begin(), this_tags.end());
    }

    void
    stream_to_vector_overlay_ff_impl::push_tags()
    {
      //fix offsets.
      for(auto tag : d_tags_between) {
        tag.offset = nitems_written(0);
        add_item_tag(0, tag);
      }
      d_tags_between.clear();
    }

    int
    stream_to_vector_overlay_ff_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const float *in = (const float *) input_items[0];
      float *out = (float *) output_items[0];

      std::vector<tag_t> tags_on_input;

      if(d_offset != 0) {
        //move along until new samples arrive
        int consumable_count = std::min(ninput_items[0], d_offset);
        save_tags(consumable_count);
        d_offset -= consumable_count;
        consume_each(consumable_count);
        return 0;
      }
      else {
        //enough samples on input!
        save_tags(d_vec_size);
        for(int i = 0; i < d_vec_size; i++){
          out[i] = in[i+d_offset];
        }
        push_tags();
        consume_each (0);
        d_offset = d_delta_samps;

        return 1;
      }

    }

  } /* namespace digitizers */
} /* namespace gr */

