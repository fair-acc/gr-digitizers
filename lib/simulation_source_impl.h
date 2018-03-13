/* -*- c++ -*- */
/* 
 * Copyright 2017 Cosylab d.o.o.
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

#ifndef INCLUDED_DIGITIZERS_SIMULATION_SOURCE_IMPL_H
#define INCLUDED_DIGITIZERS_SIMULATION_SOURCE_IMPL_H

#include <digitizers/simulation_source.h>
#include "digitizer_block_impl.h"
#include <system_error>
#include <string>
#include "digitizers/range.h"

namespace gr {
  namespace digitizers {

    class simulation_source_impl : public digitizer_block_impl, public simulation_source {

      meta_range_t d_ranges;
      int d_overflow;
      std::vector<std::vector<float>> ch_data;
      std::vector<uint8_t> port_data;
    public:
      simulation_source_impl();
      ~simulation_source_impl();

      void set_data(std::vector<float> ch_a_vec,
        std::vector<float> ch_b_vec,
        std::vector<uint8_t> port_vec,
        int overflow_pattern) override;

      std::vector<std::string> get_aichan_ids() override;

      meta_range_t get_aichan_ranges() override;

      std::error_code driver_initialize() override;

      std::error_code driver_configure() override;

      std::error_code driver_arm() override;

      std::error_code driver_disarm() override;

      std::error_code driver_close() override;

      std::error_code driver_prefetch_block(size_t length, size_t block_number) override;

      std::error_code driver_get_rapid_block_data(size_t offset, size_t length, size_t waveform,
              gr_vector_void_star &arrays, std::vector<uint32_t> &status) override;

      std::error_code driver_get_streaming_data(
              gr_vector_void_star &ai_buffers,
              gr_vector_void_star &ai_error_buffers,
              gr_vector_void_star &port_buffers, std::vector<uint32_t> &status, size_t length, size_t &actual) override;

      size_t driver_get_streaming_samples() override;

      std::error_code driver_poll() override;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_SIMULATION_SOURCE_IMPL_H */

