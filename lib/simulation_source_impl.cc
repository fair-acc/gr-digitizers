/* -*- c++ -*- */
/* 
 * Copyright 2017 <+YOU OR YOUR COMPANY+>.
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
#include "simulation_source_impl.h"
#include <thread>
#include <chrono>
#include <digitizers/status.h>

namespace gr {
  namespace digitizers {

    simulation_source::sptr
    simulation_source::make()
    {
      return gnuradio::get_initial_sptr
        (new simulation_source_impl());
    }

    /*
     * The private constructor
     */
    simulation_source_impl::simulation_source_impl()
      : gr::sync_block("simulation_source",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::makev(1, 5, std::vector<int>({4,4,4,4,1}))),
      digitizer_block_impl(2, 1, false),
      d_overflow(0)
    {
      d_ranges.push_back(range_t(0.01));
      d_ranges.push_back(range_t(0.02));
      d_ranges.push_back(range_t(0.05));
      d_ranges.push_back(range_t(0.1));
      d_ranges.push_back(range_t(0.2));
      d_ranges.push_back(range_t(0.5));
      d_ranges.push_back(range_t(1));
      d_ranges.push_back(range_t(2));
      d_ranges.push_back(range_t(5));
      d_ranges.push_back(range_t(10));
      d_ranges.push_back(range_t(20));
    }

    /*
     * Our virtual destructor.
     */
    simulation_source_impl::~simulation_source_impl()
    {
    }

    void
    simulation_source_impl::set_data(std::vector<float> ch_a_vec,
      std::vector<float> ch_b_vec,
      std::vector<uint8_t> port_vec,
      int overflow_pattern)
    {
      if(d_samples+d_pre_samples != ch_a_vec.size() ||
          ch_a_vec.size() != ch_b_vec.size() ||
          ch_a_vec.size() != port_vec.size()) {
        return;
      }
      ch_data.push_back(ch_a_vec);
      ch_data.push_back(ch_b_vec);
      port_data = port_vec;
      d_overflow = overflow_pattern;
      return;
    }

    std::vector<std::string>
    simulation_source_impl::get_aichan_ids()
    {
      return std::vector<std::string> {"A", "B"};
    }

    meta_range_t
    simulation_source_impl::get_aichan_ranges()
    {
      return d_ranges;
    }

    std::error_code
    simulation_source_impl::driver_initialize()
    {
      return std::error_code{};
    }

    std::error_code
    simulation_source_impl:: driver_configure()
    {
      return std::error_code{};
    }

    std::error_code
    simulation_source_impl::driver_arm()
    {
      notify_data_ready(std::error_code{});
      return std::error_code{};
    }

    std::error_code
    simulation_source_impl::driver_disarm()
    {
      return std::error_code{};
    }

    std::error_code
    simulation_source_impl::driver_close()
    {
      return std::error_code{};
    }

    std::error_code
    simulation_source_impl::driver_prefetch_block(size_t length, size_t block_number)
    {
      return std::error_code{};
    }

    std::error_code
    simulation_source_impl::driver_get_rapid_block_data(size_t offset, size_t length, size_t waveform,
                  gr_vector_void_star &arrays, std::vector<uint32_t> &status)
    {
      //analog channels
      unsigned vec_index = 0;
      for(auto chan_idx = 0; chan_idx < 2; chan_idx++) {
        if(!d_channel_settings[chan_idx].enabled) {
          vec_index +=2;
          continue;
        }

        if (d_overflow & (1 << chan_idx)) {
          status[chan_idx] = channel_status_t::CHANNEL_STATUS_OVERFLOW;
        }
        else {
          status[chan_idx] = 0;
        }

        float *out = (float *) arrays.at(vec_index);
        float *err_out = (float *) arrays.at(vec_index + 1);
        std::vector<float> in = ch_data.at(chan_idx);

        for (size_t i = 0; i < length; i++) {
          out[i] = in[i+offset];
        }

        // 8bit resolution (minus sign bit)
        auto error_estimate = d_channel_settings[chan_idx].actual_range / 128.0;
        for (size_t i=0; i<length; i++) {
          err_out[i] = error_estimate;
        }

        vec_index += 2;
      }

      //digital port
      if (d_port_settings[0].enabled) {
        uint8_t *out = static_cast<uint8_t *>(arrays.at(0));
        uint8_t *in = &port_data[0] + offset;

        for (size_t i=0; i<length; i++) {
          out[i] = in[i];
        }
      }

      return std::error_code {};
    }

    std::error_code
    simulation_source_impl::driver_get_streaming_data(
            gr_vector_void_star &ai_buffers,
            gr_vector_void_star &ai_error_buffers,
            gr_vector_void_star &port_buffers, std::vector<uint32_t> &status, size_t length, size_t &actual)
    {
      actual = length;

      for(auto chan_idx = 0; chan_idx < 2; chan_idx++) {
        if(!d_channel_settings[chan_idx].enabled) {
          continue;
        }

        if (d_overflow & (1 << chan_idx)) {
          status[chan_idx] = channel_status_t::CHANNEL_STATUS_OVERFLOW;
        }
        else {
          status[chan_idx] = 0;
        }

        float *out = static_cast<float *>(ai_buffers.at(chan_idx));
        float *err_out = static_cast<float *>(ai_error_buffers.at(chan_idx));
        std::vector<float>in = ch_data.at(chan_idx);

        for (size_t i = 0; i < length; i++) {
          out[i] = in[i];
        }
        // 8bit resolution (minus sign bit)
        auto error_estimate = d_channel_settings[chan_idx].actual_range / 128.0;
        for (size_t i=0; i<length; i++) {
          err_out[i] = error_estimate;
        }
      }

      //digital port
      if (d_port_settings[0].enabled) {
        uint8_t *out = static_cast<uint8_t *>(port_buffers.at(0));

        for (size_t i=0; i<length; i++) {
          out[i] = port_data[i];
        }
      }

      return std::error_code {};
    }

    size_t
    simulation_source_impl::driver_get_streaming_samples()
    {
      return ch_data.at(0).size();
    }

    std::error_code
    simulation_source_impl::driver_poll()
    {
      return std::error_code {};
    }
  } /* namespace digitizers */
} /* namespace gr */

