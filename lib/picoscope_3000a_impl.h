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

#ifndef INCLUDED_DIGITIZERS_PICOSCOPE_3000A_IMPL_H
#define INCLUDED_DIGITIZERS_PICOSCOPE_3000A_IMPL_H

#include <digitizers/picoscope_3000a.h>
#include "digitizer_block_impl.h"
#include "digitizers/range.h"
#include "utils.h"
#include <system_error>
#include <array>
#include <boost/thread/mutex.hpp>

#include <libps3000a-1.1/ps3000aApi.h>
#include <libps3000a-1.1/PicoStatus.h>

namespace gr {
  namespace digitizers {

    //GNU Radio block implementation header
    class picoscope_3000a_impl : public digitizer_block_impl, public picoscope_3000a
    {
     private:

      int16_t d_handle;     // picoscope handle
      int16_t d_max_value;  // maximum ADC count

      std::string d_serial_number; // if empty firs tdevice that is found is used
      meta_range_t d_ranges;

      int16_t d_overflow;  // status returned from getValues

      int32_t d_stream_start_index;
      int32_t d_stream_no_samples;
      int64_t d_stream_timestamp;

      std::array<std::vector<int16_t>, PS3000A_MAX_CHANNELS> d_buffers;
      std::array<std::vector<int16_t>, PS3000A_MAX_CHANNELS> d_buffers_min;
      std::array<std::vector<int16_t>, PS3000A_MAX_DIGITAL_PORTS> d_port_buffers;

      boost::mutex d_mutex;

     public:

      picoscope_3000a_impl(std::string serial_number, std::vector<int> outSig, bool auto_arm);

      ~picoscope_3000a_impl();

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

      void rapid_block_callback(int16_t handle, PICO_STATUS status);

      void streaming_callback(int16_t handle, int32_t noOfSamples, uint32_t startIndex, int16_t overflow, uint32_t triggerAt, int16_t triggered, int16_t autoStop);

     private:

      std::error_code set_buffers(size_t samples, uint32_t block_number);

      uint32_t convert_frequency_to_ps3000a_timebase(double desired_freq, double &actual_freq);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_PICOSCOPE_3000A_IMPL_H */
