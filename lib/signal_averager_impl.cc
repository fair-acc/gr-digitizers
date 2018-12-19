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
    signal_averager::make(int num_inputs, int window_size)
    {
      return gnuradio::get_initial_sptr
        (new signal_averager_impl(num_inputs, window_size));
    }

    signal_averager_impl::signal_averager_impl(int num_inputs, int window_size)
      : gr::sync_decimator("signal_averager",
              gr::io_signature::make(num_inputs, num_inputs, sizeof(float)),
              gr::io_signature::make(num_inputs, num_inputs, sizeof(float)), window_size),
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

      for(int port = 0; port < d_num_ports; port++)
      {

//        auto count0 = nitems_read(port);

        //average signal
        float *out = (float *) output_items[port];
        const float *in = (const float *) input_items[port];

        for(int i = 0; i < noutput_items; i++)
        {

          // Signal averaging part
          float sum = 0.0f;

          for(unsigned j = 0; j < decim; j++)
          {
            sum += in[j];
          }

          out[i] = sum / static_cast<float>(decim);
          in += decim;

//          // Tag correction/propagation part
//          std::vector<gr::tag_t> tags;
//          auto local_count0 = count0 + static_cast<uint64_t>(i * decim);
//          get_tags_in_range(tags, port, local_count0, local_count0 + decim);
//
//          // Merge acq info tags if more than one falls into the same bucket
//          boost::optional<acq_info_t> merged_acq_info_tag = boost::none;
//
//          for(auto tag : tags) {
//            tag.offset *= (1.0 / decim);
//
//            auto tag_key = pmt::symbol_to_string(tag.key);
//
//            if(tag_key == timebase_info_tag_name) {
//              auto timebase = decode_timebase_info_tag(tag);
//
//              // Fix timebase
//              auto tmp =  make_timebase_info_tag(timebase * static_cast<double>(decim));
//              tmp.offset = tag.offset;
//              add_item_tag(port, tmp);
//            }
//            else if (tag_key == acq_info_tag_name) {
//              auto acq_info = decode_acq_info_tag(tag);
//
//              if (merged_acq_info_tag) {
//                merged_acq_info_tag->status |= acq_info.status;
//              }
//              else {
//                acq_info.pre_samples /= decim;
//                acq_info.samples /= decim;
//                acq_info.timebase *= static_cast<double>(decim);
//
//                merged_acq_info_tag = acq_info;
//              }
//            }
//            else {
//              add_item_tag(port, tag);
//            }
//          }
//
//          if (merged_acq_info_tag) {
//            auto tag = make_acq_info_tag(*merged_acq_info_tag);
//            add_item_tag(port, tag);
//          }
//
          }
      }

      return noutput_items;
    }

  } /* namespace digitizers */
} /* namespace gr */

