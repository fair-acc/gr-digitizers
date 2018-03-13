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

      for (auto i = 0; i < noutput_items; i++) {

        // Get acq_info tags in range
        std::vector<gr::tag_t> tags;
        get_tags_in_window(tags, 0, i, i + 1, pmt::string_to_symbol("acq_info"));

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
          auto samp0_count = nitems_read(0);
          // timestamp of the first sample in working range
          auto timestamp = -1;

          if (d_acq_info.timestamp != -1) {
            timestamp = d_acq_info.timestamp + static_cast<int64_t>(
                 ((samp0_count + i) - d_acq_info.offset) * d_acq_info.timebase * 1000000000.0);
          }

          if (d_callback) {
            d_callback(timestamp, d_user_data);
          }

          d_interlock_issued = true;
        }
      }

      return noutput_items;
    }

    void
    interlock_generation_ff_impl::set_interlock_callback(interlock_cb_t callback, void *ptr)
    {
      d_callback = callback;
      d_user_data = ptr;
      d_interlock_issued = false;
    }

  } /* namespace digitizers */
} /* namespace gr */

