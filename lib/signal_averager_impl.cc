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
              gr::io_signature::make(num_inputs, num_inputs, sizeof(float)),
              window_size),
        d_num_ports(num_inputs)
    {

      // The gr scheduler uses the block's gr::block::relative_rate concept to perform the update on the tag's offset value
      // The relative rate of a block determines the relationship between the input rate and output rate.
      // Decimators that decimate by a factor of D have a relative rate of 1/D.
      set_tag_propagation_policy(tag_propagation_policy_t::TPP_ONE_TO_ONE ); //
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
        for(int i = 0; i < noutput_items; i++)
        {
          float sum = 0.0f;
          for(unsigned j = 0; j < decim; j++)
            sum += in[j];

          out[i] = sum / static_cast<float>(decim);
          in += decim;
        }
      }
      return noutput_items;
    }

  } /* namespace digitizers */
} /* namespace gr */

