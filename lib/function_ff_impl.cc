/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "function_ff_impl.h"

namespace gr {
  namespace digitizers {

    function_ff::sptr
    function_ff::make(int decimation)
    {
      return gnuradio::get_initial_sptr
        (new function_ff_impl(decimation));
    }

    /*
     * The private constructor
     */
    function_ff_impl::function_ff_impl(int decimation)
      : gr::sync_decimator("function_ff",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(3, 3, sizeof(float)), decimation),
        d_acq_info(),
        d_ref{0.0},
        d_min{0.0},
        d_max{0.0}
    {
     if(decimation <= 0) {
    	 GR_LOG_ALERT(logger, "function_ff_impl::function_ff_impl(int decimation) -decimation must not be <=0");
      }
      set_tag_propagation_policy(TPP_DONT);
    }

    bool
    function_ff_impl::start()
    {
      // reset state
      d_acq_info.last_beam_in_timestamp = -1;
      d_acq_info.timestamp = -1;
      d_acq_info.offset = 0;
      d_acq_info.trigger_timestamp = -1;

      return true;
    }

    void
    function_ff_impl::set_function(const std::vector<float> &timing,
            const std::vector<float> &ref,
            const std::vector<float> &min,
            const std::vector<float> &max)
    {
      if (min.empty() || min.size() != ref.size() || max.size() != ref.size()
                || (timing.size() < (ref.size() - 1))) {
        GR_LOG_ERROR(d_logger, "invalid function provided, length mismatch");
        return;
      }

      boost::mutex::scoped_lock lock(d_mutex);

      d_ref = ref;
      d_min = min;
      d_max = max;

      // convert seconds to nanoseconds
      d_timing.clear();
      for (const auto t : timing) {
        d_timing.push_back(static_cast<int64_t>(t * 1000000000.0));
      }
    }

    function_ff_impl::~function_ff_impl()
    {
    }

    int
    function_ff_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      float *out_ref = (float *) output_items[0];
      float *out_min = (float *) output_items[1];
      float *out_max = (float *) output_items[2];

      boost::mutex::scoped_lock lock(d_mutex);

      if (d_ref.size() == 1) {
        for (int i = 0; i < noutput_items; i++) {
          out_ref[i] = d_ref[0];
          out_min[i] = d_min[0];
          out_max[i] = d_max[0];
        }
      }
      else {

        const uint64_t samp0_count = nitems_read(0);

        for (int i = 0; i < noutput_items; i++) {

          auto input_idx = i * decimation();

          // Get acq_info tags in range
          std::vector<gr::tag_t> tags;
          get_tags_in_range(tags, 0, samp0_count + input_idx,
                  samp0_count + input_idx + decimation(),
                  pmt::string_to_symbol(acq_info_tag_name));

          auto acq_info = d_acq_info;

          if (!tags.empty()) {
            // use the first tag, save the last one
            if (tags.at(0).offset == (samp0_count + input_idx)) {
                acq_info = decode_acq_info_tag(tags.at(0));
            }

            d_acq_info = decode_acq_info_tag(tags.back());
          }

          // no timing
          if (acq_info.last_beam_in_timestamp == -1 || acq_info.timestamp == -1) {
            out_ref[i] = d_ref[0];
            out_min[i] = d_min[0];
            out_max[i] = d_max[0];
          }
          else {

            // timestamp of the first sample in working range
            auto timestamp = acq_info.timestamp + static_cast<int64_t>(
                    ((samp0_count + input_idx) - acq_info.offset) * acq_info.timebase * 1000000000.0);

            if (timestamp < acq_info.last_beam_in_timestamp) {
              out_ref[i] = d_ref[0];
              out_min[i] = d_min[0];
              out_max[i] = d_max[0];
            }
            else {
              auto distance_from_beam_in = timestamp - acq_info.last_beam_in_timestamp;

              // find left and right index
              auto it = std::find_if(d_timing.begin(), d_timing.end(),
                  [distance_from_beam_in] (const int64_t t)
              {
                  return t >= distance_from_beam_in;
              });

              auto idx_r = static_cast<int>(std::distance(d_timing.begin(), it));

              if (idx_r == static_cast<int>(d_timing.size())) {
                out_ref[i] = d_ref[idx_r - 1];
                out_min[i] = d_min[idx_r - 1];
                out_max[i] = d_max[idx_r - 1];
              }
              else if (idx_r == 0) {
                out_ref[i] = d_ref[0];
                out_min[i] = d_min[0];
                out_max[i] = d_max[0];
              }
              else {
                auto idx_l = idx_r - 1;

                // perform linear interpolation
                auto k = static_cast<float>(distance_from_beam_in - d_timing[idx_l])
                          / static_cast<float>(d_timing[idx_r] - d_timing[idx_l]);

                out_ref[i] = d_ref[idx_l] + (d_ref[idx_r] - d_ref[idx_l]) * k;
                out_min[i] = d_min[idx_l] + (d_min[idx_r] - d_min[idx_l]) * k;
                out_max[i] = d_max[idx_l] + (d_max[idx_r] - d_max[idx_l]) * k;
              }
            }
          }
        }
      }

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace digitizers */
} /* namespace gr */

