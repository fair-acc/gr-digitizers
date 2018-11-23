/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "stream_to_vector_overlay_ff_impl.h"

namespace gr {
  namespace digitizers {

    stream_to_vector_overlay_ff::sptr
    stream_to_vector_overlay_ff::make(int vec_size, double samp_rate, double delta_t)
    {
      return gnuradio::get_initial_sptr
        (new stream_to_vector_overlay_ff_impl(vec_size, samp_rate, delta_t));
    }

    /*
     * The private constructor
     */
    stream_to_vector_overlay_ff_impl::stream_to_vector_overlay_ff_impl(int vec_size, double samp_rate, double delta_t)
      : gr::block("stream_to_vector_overlay_ff",
              gr::io_signature::make(1,1, sizeof(float)),
              gr::io_signature::make(1,1, sizeof(float) * vec_size)),
              d_samp_rate(samp_rate),
              d_delta_t(delta_t),
              d_vec_size(vec_size),
              d_offset(0),
              d_acq_info(),
              d_tag_offset(0)
    {
      set_tag_propagation_policy(TPP_DONT);

      d_acq_info.timestamp = -1;
    }

    bool
    stream_to_vector_overlay_ff_impl::start()
    {
      d_acq_info.timestamp = -1;
      return true;
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
      get_tags_in_range(this_tags, 0, nitems_read(0), nitems_read(0)+count, pmt::string_to_symbol(acq_info_tag_name));
      if(this_tags.size() != 0) {
        d_acq_info = decode_acq_info_tag(this_tags.back());
        d_tag_offset = this_tags.back().offset;
      }
    }

    void
    stream_to_vector_overlay_ff_impl::push_tags()
    {
      //fix offset.
      if (d_acq_info.timestamp != -1) {
        d_acq_info.timestamp += ((nitems_read(0) - d_tag_offset) * d_samp_rate);
      }
      tag_t tag = make_acq_info_tag(d_acq_info, nitems_written(0));
      add_item_tag(0, tag);
    }

    int
    stream_to_vector_overlay_ff_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const float *in = (const float *) input_items[0];
      float *out = (float *) output_items[0];

      if(d_offset >= 1.0) {
        //move along until new samples arrive
        int consumable_count = std::min(ninput_items[0], static_cast<int>(d_offset));
        save_tags(consumable_count);
        d_offset -= consumable_count;
        consume_each(consumable_count);
        return 0;
      }
      else {
        //enough samples on input!
        save_tags(d_vec_size);
        /*
        for(int i = 0; i < d_vec_size; i++){
          out[i] = in[i];            
        }
        */
        memcpy(out, in, d_vec_size*sizeof(float));
        push_tags();
        consume_each (0);
        d_offset += d_delta_t * d_samp_rate;

        return 1;
      }

    }

  } /* namespace digitizers */
} /* namespace gr */

