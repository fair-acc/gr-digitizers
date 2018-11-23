/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "decimate_and_adjust_timebase_impl.h"
#include <digitizers/tags.h>
#include <boost/optional.hpp>

namespace gr {
  namespace digitizers {

    decimate_and_adjust_timebase::sptr
    decimate_and_adjust_timebase::make(int decimation, double delay)
    {
      return gnuradio::get_initial_sptr
        (new decimate_and_adjust_timebase_impl(decimation, delay));
    }

    /*
     * The private constructor
     */
    decimate_and_adjust_timebase_impl::decimate_and_adjust_timebase_impl(int decimation, double delay)
      : gr::sync_decimator("decimate_and_adjust_timebase",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(1, 1, sizeof(float)), decimation),
        d_delay(delay)
    {
      set_relative_rate(1./decimation);
      set_tag_propagation_policy(TPP_ONE_TO_ONE);
    }

    /*
     * Our virtual destructor.
     */
    decimate_and_adjust_timebase_impl::~decimate_and_adjust_timebase_impl()
    {
    }

    int
    decimate_and_adjust_timebase_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const auto decim = decimation();
      const auto count0 = nitems_read(0);

      float *out = (float *) output_items[0];
      const float *in = (const float *) input_items[0];

      for(int i = 0; i < noutput_items; i++)
      {
        // Keep one in N functionality
        out[i] = in[decim - 1];
        in += decim;
      }

      return noutput_items;
    }

  } /* namespace digitizers */
} /* namespace gr */

