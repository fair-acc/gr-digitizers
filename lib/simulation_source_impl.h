/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
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

    class simulation_source_impl : public digitizer_block_impl, public simulation_source
    {
    private:
      meta_range_t d_ranges;
      std::vector<float> d_ch_a_data;
      std::vector<float> d_ch_b_data;
      std::vector<uint8_t> d_port_data;

    public:
      simulation_source_impl();
      ~simulation_source_impl();

      void set_data(const std::vector<float> &ch_a_vec, const std::vector<float> &ch_b_vec,
              const std::vector<uint8_t> &port_vec) override;

      std::vector<std::string> get_aichan_ids() override;

      meta_range_t get_aichan_ranges() override;

      std::string get_driver_version() override;

      std::string get_hardware_version() override;

      std::error_code driver_initialize() override;

      std::error_code driver_configure() override;

      std::error_code driver_arm() override;

      std::error_code driver_disarm() override;

      std::error_code driver_close() override;

      std::error_code driver_prefetch_block(size_t length, size_t block_number) override;

      std::error_code driver_get_rapid_block_data(size_t offset, size_t length, size_t waveform,
              gr_vector_void_star &arrays, std::vector<uint32_t> &status) override;

      std::error_code driver_poll() override;

    private:
      bool fill_in_data_chunk();
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_SIMULATION_SOURCE_IMPL_H */

