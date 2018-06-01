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
      set_tag_propagation_policy(TPP_DONT);
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

      for(int i = 0; i < noutput_items; i++) {
        // Keep one in N functionality
        out[i] = in[decim - 1];
        in += decim;

        // Tag correction/propagation part
        std::vector<gr::tag_t> tags;
        auto local_count0 = count0 + static_cast<uint64_t>(i * decim);
        get_tags_in_range(tags, 0, local_count0, local_count0 + decim);

        // Merge acq info tags if more than one falls into the same bucket
        boost::optional<acq_info_t> merged_acq_info_tag = boost::none;

        for(auto tag : tags) {
          tag.offset *= (1.0 / decim);

          auto tag_key = pmt::symbol_to_string(tag.key);

          if(tag_key == timebase_info_tag_name) {
            auto timebase = decode_timebase_info_tag(tag);

            // Fix timebase
            auto tmp =  make_timebase_info_tag(timebase * static_cast<double>(decim));
            tmp.offset = tag.offset;
            add_item_tag(0, tmp);
          }
          else if (tag_key == acq_info_tag_name) {
            auto acq_info = decode_acq_info_tag(tag);

            if (merged_acq_info_tag) {
              merged_acq_info_tag->status |= acq_info.status;
            }
            else {
              acq_info.pre_samples /= decim;
              acq_info.samples /= decim;
              acq_info.timebase *= static_cast<double>(decim);
              acq_info.user_delay += d_delay;
              acq_info.actual_delay += d_delay;

              merged_acq_info_tag = acq_info;
            }
          }
          else {
            add_item_tag(0, tag);
          }
        }

        if (merged_acq_info_tag) {
          auto tag = make_acq_info_tag(*merged_acq_info_tag);
          add_item_tag(0, tag);
        }

      }

      return noutput_items;
    }

  } /* namespace digitizers */
} /* namespace gr */

