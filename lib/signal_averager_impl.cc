/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "signal_averager_impl.h"
#include <digitizers/tags.h>
#include <boost/optional.hpp>

namespace gr {
  namespace digitizers {

    signal_averager::sptr
    signal_averager::make(int num_inputs, int window_size, float samp_rate)
    {
      return gnuradio::get_initial_sptr
        (new signal_averager_impl(num_inputs, window_size, samp_rate));
    }

    signal_averager_impl::signal_averager_impl(int num_inputs, int window_size, float samp_rate)
      : gr::sync_decimator("signal_averager",
              gr::io_signature::make(num_inputs, num_inputs, sizeof(float)),
              gr::io_signature::make(num_inputs, num_inputs, sizeof(float)),
              window_size),
        d_num_ports(num_inputs)
    {
      set_tag_propagation_policy(tag_propagation_policy_t::TPP_CUSTOM);

      assert(samp_rate != 0);
      d_sample_sample_distance_input_ns = 1000000000./samp_rate;
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
      for(int port = 0; port < d_num_ports; port++)
      {
        float *out = (float *) output_items[port];
        const float *in = (const float *) input_items[port];
        int i_in = 0, i_out = 0;
        for(i_out = 0; i_out < noutput_items; i_out++)
        {
          float sum = 0.0f;
          for(unsigned temp = 0; temp < decim; temp++)
            sum += in[i_in + temp];

          out[i_out] = sum / static_cast<float>(decim);

          std::vector<gr::tag_t> tags;
          get_tags_in_range(tags, port, nitems_read(port) + i_in, nitems_read(port) + i_in + decim );
          // required to merge acq_infotags due to this bug: https://github.com/gnuradio/gnuradio/issues/2364
          // otherwise we will eat to much memory
          // TODO: Fix bug in gnuradio (https://gitlab.com/al.schwinn/gr-digitizers/issues/33)
          acq_info_t merged_acq_info;
          merged_acq_info.status = 0;
          bool found_acq_info = false;
          for(auto tag : tags)
          {
              //std::cout << "tag found: " << tag.key << std::endl;
              if(tag.key == pmt::string_to_symbol(trigger_tag_name))
              {
                  trigger_t trigger_tag_data = decode_trigger_tag(tag);

//                  std::cout << "tag.offset: " << tag.offset << std::endl;
//                  std::cout << "nitems_read(port): " << nitems_read(port) << std::endl;
//                  std::cout << "i_in: " << i_in << std::endl;
//                  std::cout << "decim/2: " << decim/2 << std::endl;

                  add_item_tag(port, make_trigger_tag(trigger_tag_data, nitems_written(port) + i_out));
                  //std::cout << "trigger tag added" << std::endl;
              }
              else if(tag.key == pmt::string_to_symbol(acq_info_tag_name))
              {
                  found_acq_info = true;
                  merged_acq_info.status |= decode_acq_info_tag(tag).status;
              }
              else
              {
                  tag.offset = nitems_written(port) + i_out;
                  add_item_tag(port, tag);
              }
          }
          if(found_acq_info)
              add_item_tag(port, make_acq_info_tag(merged_acq_info, nitems_written(port) + i_out));

          i_in += decim;
        }
      }
      return noutput_items;
    }

  } /* namespace digitizers */
} /* namespace gr */

