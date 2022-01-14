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
    decimate_and_adjust_timebase::make(int decimation, double delay, float samp_rate)
    {
      return gnuradio::get_initial_sptr
        (new decimate_and_adjust_timebase_impl(decimation, delay, samp_rate));
    }

    decimate_and_adjust_timebase_impl::decimate_and_adjust_timebase_impl(int decimation, double delay, float samp_rate)
      : gr::sync_decimator("decimate_and_adjust_timebase",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(1, 1, sizeof(float)), decimation),
        d_delay(delay)

    {
      set_relative_rate(1./decimation);
      set_tag_propagation_policy(TPP_CUSTOM);

      assert(samp_rate != 0);
      d_sample_sample_distance_input_ns = 1000000000./samp_rate;
    }

    decimate_and_adjust_timebase_impl::~decimate_and_adjust_timebase_impl()
    {
    }

    int
    decimate_and_adjust_timebase_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const auto samp0_count = nitems_read(0);

      // add tags with corrected offset to the output stream
      std::vector<gr::tag_t> tags;
      get_tags_in_range(tags, 0, samp0_count, samp0_count + input_items.size());
      for (const auto &tag : tags){
          tag_t new_tag = tag;
          if(decimation() != 0)
              new_tag.offset = uint64_t(tag.offset / decimation());
          else
              new_tag.offset = uint64_t(tag.offset);
          add_item_tag(0, new_tag);
      }

      return noutput_items;
    }

  } /* namespace digitizers */
} /* namespace gr */

