/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "interlock_generation_ff_impl.h"

namespace gr {
  namespace digitizers {

    interlock_generation_ff::sptr
    interlock_generation_ff::make(float max_min, float max_max)
    {
      return gnuradio::get_initial_sptr
        (new interlock_generation_ff_impl(max_min, max_max));
    }

    interlock_generation_ff_impl::interlock_generation_ff_impl(float max_min, float max_max)
      : gr::sync_block("interlock_generation_ff",
              gr::io_signature::make(3, 3, sizeof(float)),
              gr::io_signature::make(1, 1, sizeof(float))),
        d_max_max(max_max),
        d_max_min(max_min),
        d_interlock_issued(false),
        d_callback(nullptr),
        d_user_data(nullptr),
        d_acq_info()
    {
      d_acq_info.timestamp = -1;
    }

    interlock_generation_ff_impl::~interlock_generation_ff_impl()
    {
    }

    bool
    interlock_generation_ff_impl::start()
    {
      d_acq_info = acq_info_t {};
      d_acq_info.timestamp = -1;
      return true;
    }

    int
    interlock_generation_ff_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const float *in = (const float *) input_items[0];
      const float *min = (const float *) input_items[1];
      const float *max = (const float *) input_items[2];

      float *out = (float *) output_items[0];

      auto samp0_count = nitems_read(0);

      for (auto i = 0; i < noutput_items; i++) {

        // Get acq_info tags in range
        std::vector<gr::tag_t> tags;
        get_tags_in_window(tags, 0, i, i + 1, pmt::string_to_symbol(acq_info_tag_name));

        if (!tags.empty()) {
          d_acq_info = decode_acq_info_tag(tags.back());
        }

        bool interlock = false;

        if (max[i] < d_max_max) {
          if (in[i] >= max[i]) {
            interlock = true;
          }
        }

        if (min[i] > d_max_min) {
          if (in[i] <= min[i]) {
            interlock = true;
          }
        }

        out[i] = interlock;

        if (d_interlock_issued && !interlock) {
          d_interlock_issued = false;
        }
        else if (!d_interlock_issued && interlock) {
          // calculate timestamp
          int64_t timestamp = -1;

//          if (d_acq_info.timestamp != -1)
//          {
//            timestamp = d_acq_info.timestamp + static_cast<int64_t>
//            (
//                 ((samp0_count + static_cast<uint64_t>(i)) - d_acq_info.offset) * d_acq_info.timebase * 1000000000.0
//            );
//          }

          if (d_callback) {
            d_callback(timestamp, d_user_data);
          }

          d_interlock_issued = true;
        }
      }

      return noutput_items;
    }

    void
    interlock_generation_ff_impl::set_callback(interlock_cb_t callback, void *ptr)
    {
      d_callback = callback;
      d_user_data = ptr;
      d_interlock_issued = false;
    }

  } /* namespace digitizers */
} /* namespace gr */

