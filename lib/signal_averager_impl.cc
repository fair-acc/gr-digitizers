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
#include "signal_averager_impl.h"
#include <digitizers/tags.h>

namespace gr {
  namespace digitizers {

    signal_averager::sptr
    signal_averager::make(int num_inputs, int window_size)
    {
      return gnuradio::get_initial_sptr
        (new signal_averager_impl(num_inputs, window_size));
    }

    signal_averager_impl::signal_averager_impl(int num_inputs, int window_size)
      : gr::sync_decimator("signal_averager",
              gr::io_signature::make(num_inputs,num_inputs, sizeof(float)),
              gr::io_signature::make(num_inputs,num_inputs, sizeof(float)), window_size),
              d_num_ports(num_inputs)
    {
      set_tag_propagation_policy(tag_propagation_policy_t::TPP_DONT);
    }

    signal_averager_impl::~signal_averager_impl()
    {
    }

    int
    signal_averager_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      unsigned decim = decimation();

      for(int port = 0; port <d_num_ports; port++) {
        //average signal
        float *out = (float *) output_items[port];
        const float *in = (const float *) input_items[port];
        for(int i = 0; i < noutput_items; i++){
          double sum = 0.0;
          for(size_t j = 0; j < decim; j++) { sum += in[j]; }
          out[i] = sum / decim;
          in += decim;
        }

        //fix tags
        std::vector<gr::tag_t> tags;
        get_tags_in_range(tags, port, nitems_read(port), nitems_read(port)+noutput_items/decim);
        for(auto tag : tags) {
          tag.offset *= (1.0/decim);
          if(pmt::symbol_to_string(tag.key) == "timebase_info") {
            auto timebase = decode_timebase_info_tag(tag);
            tag = make_timebase_info_tag(timebase/decim);
          }
          add_item_tag(port, tag);
        }
      }


      return noutput_items;
    }

  } /* namespace digitizers */
} /* namespace gr */

