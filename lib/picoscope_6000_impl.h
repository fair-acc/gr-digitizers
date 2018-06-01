/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_PICOSCOPE_6000_IMPL_H
#define INCLUDED_DIGITIZERS_PICOSCOPE_6000_IMPL_H

#include <digitizers/picoscope_6000.h>
#include "picoscope_impl.h"
#include "utils.h"
#include <system_error>

#include <libps6000-1.4/ps6000Api.h>
#include <libps6000-1.4/PicoStatus.h>

namespace gr {
  namespace digitizers {

    class picoscope_6000_impl : public picoscope_impl, public picoscope_6000
    {
     private:
          int16_t d_handle;     // picoscope handle
          int16_t d_overflow;  // status returned from getValues

     public:
      picoscope_6000_impl(std::string serial_number, bool auto_arm);

      ~picoscope_6000_impl();

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

      void rapid_block_callback(int16_t handle, PICO_STATUS status);

     private:

      std::string get_unit_info_topic(PICO_INFO info);

      std::error_code set_buffers(size_t samples, uint32_t block_number);

      uint32_t convert_frequency_to_ps6000_timebase(double desired_freq, double &actual_freq);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_PICOSCOPE_6000_IMPL_H */

