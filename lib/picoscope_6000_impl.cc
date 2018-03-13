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
#include <thread>
#include <chrono>
#include <mutex>
#include <cmath>
#include <algorithm>
#include <array>
#include <digitizers/digitizer_block.h>
#include <digitizers/status.h>
#include "picoscope_6000_impl.h"
#include "utils.h"
#include "ps_6000_defs.h"
#include <cstring>


struct PicoStatus6000Errc : std::error_category
{
  const char* name() const noexcept override;
  std::string message(int ev) const override;
};

const char* PicoStatus6000Errc::name() const noexcept
{
  return "Ps6000";
}

std::string PicoStatus6000Errc::message(int ev) const
{
  PICO_STATUS status = static_cast<PICO_STATUS>(ev);
  return ps6000_get_error_message(status);
}

const PicoStatus6000Errc thePsErrCategory {};

/*!
 * This method is needed because PICO_STATUS is not a distinct type (e.g. an enum)
 * therfore we cannot really hook this into the std error code properly.
 */
std::error_code make_pico_6000_error_code(PICO_STATUS e)
{
  return {static_cast<int>(e), thePsErrCategory};
}

namespace gr {
  namespace digitizers {


    /*!
     * a structure used for streaming setup
     */
    struct ps6000_unit_interval_t
    {
      PS6000_TIME_UNITS unit;
      uint32_t interval;
    };


   /**********************************************************************
    * Converters - helper functions
    *********************************************************************/

    static PS6000_RANGE
    convert_to_ps6000_range(float desired_range, float &actual_range)
    {
      if (desired_range < 0.015){
        actual_range = 0.05;
        return PS6000_10MV;
      }
      else if (desired_range < 0.035){
        actual_range = 0.05;
        return PS6000_20MV;
      }
      else if (desired_range < 0.075){
        actual_range = 0.05;
        return PS6000_50MV;
      }
      else if (desired_range < 0.15) {
        actual_range = 0.1;
        return PS6000_100MV;
      }
      else if (desired_range < 0.35) {
        actual_range = 0.2;
        return PS6000_200MV;
      }
      else if (desired_range < 0.75) {
        actual_range = 0.5;
        return PS6000_500MV;
      }
      else if (desired_range < 1.5) {
        actual_range = 1;
        return PS6000_1V;
      }
      else if (desired_range < 3.5) {
        actual_range = 2;
        return PS6000_2V;
      }
      else if (desired_range < 7.5) {
        actual_range = 5;
        return PS6000_5V;
      }
      else if (desired_range < 15) {
        actual_range = 10;
        return PS6000_10V;
      }
      else if (desired_range < 35) {
        actual_range = 20;
        return PS6000_20V;
      }
      else {
        actual_range = 50;
        return PS6000_50V;
      }
    }

    /*!
     * Note this function has to be called after the call to the ps6000SetChannel function, that is
     * just befor the arm!!!
     */
    uint32_t
    picoscope_6000_impl::convert_frequency_to_ps6000_timebase(double desired_freq, double &actual_freq)
    {
      // It is assumed that the timebase is calculated like this:
      // (timebase–2) / 125,000,000
      // e.g. timeebase == 3 --> 8ns sample interval
      //
      // Note, for some devices, the above formula might be wrong! To overcome this limitation
      // we use the ps6000GetTimebase2 function to find the closest possible timebase. The below
      // timebase estimate is therefore used as a fallback only.
      auto time_interval_ns = 1000000000.0 / desired_freq;
      uint32_t timebase_estimate = (static_cast<uint32_t>(time_interval_ns / 6.4)) + 4;

      // In order to cover for all possible 30000 series devices, we use ps6000GetTimebase2
      // function to get step size in ns between timebase 3 and 4. Based on that the actual timebase
      // is calculated.
      uint32_t dummy;
      std::array<float, 2> time_interval_ns_56;

      for (auto i = 0; i<2; i++) {
        auto status = ps6000GetTimebase2(d_handle, 5 + i, 1024, &time_interval_ns_56[i], 0, &dummy, 0);
        if(status != PICO_OK)
        {
          GR_LOG_NOTICE(d_logger, "timebase cannot be obtained: " + ps6000_get_error_message(status));
          GR_LOG_NOTICE(d_logger, "    estimated timebase will be used...");

          float time_interval_ns;
          status = ps6000GetTimebase2(d_handle, timebase_estimate, 1024, &time_interval_ns, 0, &dummy, 0);
          if(status != PICO_OK) {
            throw std::runtime_error("ps6000GetTimebase2 (timebase "
                   + std::to_string(timebase_estimate) + "): "
                   + ps6000_get_error_message(status));
          }

          actual_freq = 1000000000.0 / time_interval_ns;
          return timebase_estimate;
        }
      }

      // Calculate steps between timebase 3 and 4 and correct start_timebase estimate based on that
      auto step = time_interval_ns_56[1] - time_interval_ns_56[0];
      timebase_estimate = static_cast<uint32_t>((time_interval_ns - time_interval_ns_56[0]) / step) + 5;

      // The below code iterates trought the neighbouring timebases in order to find the best
      // match. In principle we could check only timebases on the left and right but since first
      // three timebases are in most cases special we make search space a bit bigger.
      const int search_space = 8;
      std::array<float, search_space> timebases;
      std::array<float, search_space> error_estimates;

      uint32_t start_timebase = timebase_estimate > (search_space / 2) ? timebase_estimate - (search_space / 2) : 0;

      for (auto i = 0; i < search_space; i++) {

        float obtained_time_interval_ns;
        auto status = ps6000GetTimebase2(d_handle, start_timebase + i, 1024, &obtained_time_interval_ns, 0, &dummy, 0);
        if(status != PICO_OK)
        {
          // this timebase can't be used, lets set error estimate to something big
          timebases[i] = -1;
          error_estimates[i] = 10000000000.0;
        }
        else {
          timebases[i] = obtained_time_interval_ns;
          error_estimates[i] = fabs(time_interval_ns - obtained_time_interval_ns);
        }
      }

      auto it = std::min_element(&error_estimates[0], &error_estimates[0] + error_estimates.size());
      auto distance = std::distance(&error_estimates[0], it);

      assert (distance < search_space);

      // update actual update rate and return timebase number
      actual_freq = 1000000000.0 / timebases[distance];
      return start_timebase + distance;
    }

    ps6000_unit_interval_t
    convert_frequency_to_ps6000_time_units_and_interval(double desired_freq, double &actual_freq)
    {
      ps6000_unit_interval_t unint;
      auto interval = 1.0 / desired_freq;

      if(interval < 0.000001) {
        unint.unit = PS6000_PS;
        unint.interval = static_cast<uint32_t>(1000000000000.0 / desired_freq);
        actual_freq = 1000000000000.0 / static_cast<double>(unint.interval);
      }
      else if(interval < 0.001) {
        unint.unit = PS6000_NS;
        unint.interval = static_cast<uint32_t>(1000000000.0 / desired_freq);
        actual_freq = 1000000000.0 / static_cast<double>(unint.interval);
      }
      else if(interval < 0.1){
        unint.unit = PS6000_US;
        unint.interval = static_cast<uint32_t>(1000000.0 / desired_freq );
        actual_freq = 1000000.0 / static_cast<double>(unint.interval);
      }
      else {
        unint.unit = PS6000_MS;
        unint.interval = static_cast<uint32_t>(1000.0 / desired_freq);
        actual_freq = 1000.0 / static_cast<double>(unint.interval);
      }

      return unint;
    }

    static PS6000_RATIO_MODE
    convert_to_ps6000_ratio_mode(downsampling_mode_t mode)
    {
      switch(mode)
      {
      case downsampling_mode_t::DOWNSAMPLING_MODE_MIN_MAX_AGG:
        return PS6000_RATIO_MODE_AGGREGATE;
      case downsampling_mode_t::DOWNSAMPLING_MODE_DECIMATE:
        return PS6000_RATIO_MODE_DECIMATE;
      case downsampling_mode_t::DOWNSAMPLING_MODE_AVERAGE:
        return PS6000_RATIO_MODE_AVERAGE;
      case downsampling_mode_t::DOWNSAMPLING_MODE_NONE:
      default:
        return PS6000_RATIO_MODE_NONE;
      }
    }

    PS6000_THRESHOLD_DIRECTION
    convert_to_ps6000_threshold_direction(trigger_direction_t direction)
    {
      switch(direction)
      {
      case trigger_direction_t::TRIGGER_DIRECTION_RISING:
        return PS6000_RISING;
      case trigger_direction_t::TRIGGER_DIRECTION_FALLING:
        return PS6000_FALLING;
      case trigger_direction_t::TRIGGER_DIRECTION_LOW:
        return PS6000_BELOW;
      case trigger_direction_t::TRIGGER_DIRECTION_HIGH:
        return PS6000_ABOVE;
      default:
        throw std::runtime_error("unsupported trigger direction: "
              + std::to_string((int)direction));
      }
    };

    int16_t
    convert_voltage_to_ps6000_raw_logic_value(double value)
    {
      double max_logical_voltage = 5.0;

      if (value > max_logical_voltage) {
        throw std::invalid_argument("max logical level is: " + std::to_string(max_logical_voltage));
      }

      return (int16_t) ((value / max_logical_voltage) * (double)PS6000_MAX_VALUE);
    }

    PS6000_CHANNEL
    convert_to_ps6000_channel(const std::string &source)
    {
      if (source == "A") {
        return PS6000_CHANNEL_A;
      }
      else if (source == "B") {
        return PS6000_CHANNEL_B;
      }
      else if (source == "C") {
        return PS6000_CHANNEL_C;
      }
      else if (source == "D") {
        return PS6000_CHANNEL_D;
      }
      else if (source == "EXTERNAL") {
        return PS6000_EXTERNAL;
      }
      else {
        // return invalid value
        return PS6000_MAX_TRIGGER_SOURCES;
      }
    }

    /**********************************************************************
     * Structors
     *********************************************************************/

    picoscope_6000::sptr
    picoscope_6000::make(std::string serial_number, bool auto_arm)
    {
      std::vector<int> out_signature;

      for(int i = 0; i < PS6000_MAX_CHANNELS; i++) {
        out_signature.push_back(sizeof(float));
        out_signature.push_back(sizeof(float));
      }

      for(int i =0; i<2; i++)
        out_signature.push_back(sizeof(char));

      return gnuradio::get_initial_sptr
        (new picoscope_6000_impl(serial_number,auto_arm));
    }

    picoscope_6000_impl::picoscope_6000_impl(std::string serial_number, bool auto_arm)
      : gr::sync_block("picoscope_6000",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(1, 8, sizeof(float))),
        digitizer_block_impl(PS6000_MAX_CHANNELS, 0, auto_arm), // it seems that no digitizer supports 4 ports
        d_handle(-1),
        d_max_value(255),
        d_serial_number(serial_number),
        d_ranges(),
        d_overflow(0),
        d_stream_start_index(0),
        d_stream_no_samples(0),
        d_stream_timestamp(-1)
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
      d_ranges.push_back(range_t(50));
    }

    picoscope_6000_impl::~picoscope_6000_impl()
    {
    }


    /**********************************************************************
     * Driver implementation
     *********************************************************************/

    std::vector<std::string>
    picoscope_6000_impl::get_aichan_ids()
    {
      return std::vector<std::string> {"A", "B", "C", "D"};
    }

    meta_range_t
    picoscope_6000_impl::get_aichan_ranges()
    {
      return d_ranges;
    }

    std::error_code
    picoscope_6000_impl::driver_initialize()
    {
      PICO_STATUS status;

      // take any if serial number is not provided (usefull for testing purposes)
      if (d_serial_number.empty()) {
        status = ps6000OpenUnit(&(d_handle), NULL);
      }
      else {
        status = ps6000OpenUnit(&(d_handle), (int8_t*)d_serial_number.c_str());
      }

      if (status != PICO_OK) {
        GR_LOG_ERROR(d_logger, "open unit failed: " + ps6000_get_error_message(status));
        return make_pico_6000_error_code(status);
      }

      // maximum value is used for conversion to volts
      d_max_value = PS6000_MAX_VALUE;
      if (status != PICO_OK) {
        ps6000CloseUnit(d_handle);
        GR_LOG_ERROR(d_logger, "ps6000MaximumValue: " + ps6000_get_error_message(status));
        return make_pico_6000_error_code(status);
      }

      char line[40];
      int16_t required_size;

      // It would be nicer if the number of channels is communicated to the base driver in the form
      // of function call or something similar.
      status = ps6000GetUnitInfo(d_handle, reinterpret_cast<int8_t *>(line), sizeof(line),
              &required_size, PICO_VARIANT_INFO);
      if (status != PICO_OK) {
        // this error is ignored
        GR_LOG_WARN(d_logger, "ps6000GetUnitInfo failed: " + ps6000_get_error_message(status));
        GR_LOG_WARN(d_logger, "   assuming device with 4 analog channels, and 2 digital ports");
      }
      else {
        if (line[1] == '4') {
          d_ai_channels = 4;
        }
        else {
          d_ai_channels = 2;
        }

        // Check if MSO device
        if (strnlen(line, sizeof(line)) >= 7) {
          if(strncmp(line + 4, "MSO", 3) == 0 || strncmp(line + 5, "MSO", 3) == 0 ) {
           d_ports = 2;
          }
          else {
            d_ports = 0;
          }
        }
      }

      return std::error_code{};
    }

    std::error_code
    picoscope_6000_impl::driver_configure()
    {
      assert(d_ai_channels <= PS6000_MAX_CHANNELS);

      uint32_t max_samples;
      PICO_STATUS status = ps6000MemorySegments(d_handle, d_nr_captures, &max_samples);
      if(status != PICO_OK) {
        GR_LOG_ERROR(d_logger, "ps6000MemorySegments: " + ps6000_get_error_message(status));
        return make_pico_6000_error_code(status);
      }

      status = ps6000SetNoOfCaptures(d_handle, d_nr_captures);
      if(status != PICO_OK) {
        GR_LOG_ERROR(d_logger, "ps6000SetNoOfCaptures: " + ps6000_get_error_message(status));
        return make_pico_6000_error_code(status);
      }

      auto buffer_size = (d_pre_samples + d_samples);

      // configure analog channels
      for (auto i = 0; i < d_ai_channels; i++) {
        auto enabled = d_channel_settings[i].enabled;
        auto dc_coupled = d_channel_settings[i].dc_coupled ? PS6000_DC_1M : PS6000_AC;
        auto range = convert_to_ps6000_range(
                d_channel_settings[i].range, d_channel_settings[i].actual_range);
        auto offset = d_channel_settings[i].offset;

        status = ps6000SetChannel(d_handle,
                static_cast<PS6000_CHANNEL>(i), enabled, dc_coupled, range, offset, PS6000_BW_FULL);
        if(status != PICO_OK) {
          GR_LOG_ERROR(d_logger, "ps6000SetChannel (chan " + std::to_string(i)
              + "): " + ps6000_get_error_message(status));
          return make_pico_6000_error_code(status);
        }

        if (enabled) {
          d_buffers[i].reserve(buffer_size);
        }

        if (enabled && d_downsampling_mode == DOWNSAMPLING_MODE_MIN_MAX_AGG) {
          d_buffers_min[i].reserve(buffer_size);
        }
      }

      // apply trigger configuration
      if (d_trigger_settings.is_analog())
      {
        status = ps6000SetSimpleTrigger(d_handle,
              true,  // enable
              convert_to_ps6000_channel(d_trigger_settings.source),
              convert_voltage_to_ps6000_raw_logic_value(d_trigger_settings.threshold),
              convert_to_ps6000_threshold_direction(d_trigger_settings.direction),
              0,     // delay
             -1);    // auto trigger
        if(status != PICO_OK) {
          GR_LOG_ERROR(d_logger, "ps6000SetSimpleTrigger: " + ps6000_get_error_message(status));
          return make_pico_6000_error_code(status);
        }
      }
      else {
        // disable triggers...
        PS6000_TRIGGER_CONDITIONS conds = {
              PS6000_CONDITION_DONT_CARE,
              PS6000_CONDITION_DONT_CARE,
              PS6000_CONDITION_DONT_CARE,
              PS6000_CONDITION_DONT_CARE,
              PS6000_CONDITION_DONT_CARE,
              PS6000_CONDITION_DONT_CARE,
              PS6000_CONDITION_DONT_CARE
        };
        status = ps6000SetTriggerChannelConditions(d_handle, &conds, 1);
        if(status != PICO_OK) {
          GR_LOG_ERROR(d_logger, "ps6000SetTriggerChannelConditionsV2: " + ps6000_get_error_message(status));
          return make_pico_6000_error_code(status);
        }
      }

      return std::error_code{};
    }

    void
    rapid_block_callback_redirector_6000(int16_t handle, PICO_STATUS status, void *vobj)
    {
      static_cast<picoscope_6000_impl *>(vobj)->rapid_block_callback(handle, status);
    }

    void
    picoscope_6000_impl::rapid_block_callback(int16_t handle, PICO_STATUS status)
    {
      auto errc = make_pico_6000_error_code(status);
      notify_data_ready(errc);
    }

    void
    streaming_callback_redicator_6000(int16_t handle,
        int32_t     noOfSamples,
        uint32_t    startIndex,
        int16_t     overflow,
        uint32_t    triggerAt,
        int16_t     triggered,
        int16_t     autoStop,
        void *      vobj)
    {
      static_cast<picoscope_6000_impl *>(vobj)->streaming_callback(handle,
              noOfSamples, startIndex, overflow, triggerAt, triggered, autoStop);
    }

    void
    picoscope_6000_impl::streaming_callback(int16_t handle,
        int32_t     noOfSamples,
        uint32_t    startIndex,
        int16_t     overflow,
        uint32_t    triggerAt,
        int16_t     triggered,
        int16_t     autoStop)
    {
      boost::mutex::scoped_lock lock(d_mutex);

      d_overflow = overflow;
      d_stream_start_index = startIndex;
      d_stream_no_samples = noOfSamples;
    }

    std::error_code
    picoscope_6000_impl::driver_arm()
    {
      if(d_acquisition_mode == acquisition_mode_t::RAPID_BLOCK) {
          uint32_t timebase =  convert_frequency_to_ps6000_timebase(d_samp_rate, d_actual_samp_rate);

          auto status = ps6000RunBlock(d_handle,
                  d_pre_samples,   // pre-triggersamples
                  d_samples,       // post-trigger samples
                  timebase,        // timebase
                  0,               // oversample
                  NULL,            // time indispossed
                  0,               // segment index
                  (ps6000BlockReady)rapid_block_callback_redirector_6000,
                  this);
          if(status != PICO_OK) {
            GR_LOG_ERROR(d_logger, "ps6000RunBlock: " + ps6000_get_error_message(status));
            return make_pico_6000_error_code(status);
          }
      }
      else {
        set_buffers(d_buffer_size, 0);
        d_stream_no_samples = 0;

        ps6000_unit_interval_t unit_int = convert_frequency_to_ps6000_time_units_and_interval(
                d_samp_rate, d_actual_samp_rate);

        auto status = ps6000RunStreaming(d_handle,
            &(unit_int.interval), // sample interval
            unit_int.unit,        // time unit of sample interval
            0,                    // pre-triggersamples (unused)
            d_samples + d_pre_samples, // post-trigger samples
            false,
            d_downsampling_factor,
            convert_to_ps6000_ratio_mode(d_downsampling_mode),
            d_buffer_size);

        if(status != PICO_OK) {
          GR_LOG_ERROR(d_logger, "ps6000RunStreaming: " + ps6000_get_error_message(status));
          return make_pico_6000_error_code(status);
        }
      }

      return std::error_code{};
    }

    std::error_code
    picoscope_6000_impl::driver_disarm()
    {
      auto status = ps6000Stop(d_handle);
      if(status != PICO_OK) {
        GR_LOG_ERROR(d_logger, "ps6000Stop: " + ps6000_get_error_message(status));
      }

      return make_pico_6000_error_code(status);
    }

    std::error_code
    picoscope_6000_impl::driver_close()
    {
      auto status = ps6000CloseUnit(d_handle);
      if(status != PICO_OK) {
        GR_LOG_ERROR(d_logger, "ps6000CloseUnit: " + ps6000_get_error_message(status));
      }

      d_handle = -1;
      return make_pico_6000_error_code(status);
    }

    std::error_code
    picoscope_6000_impl::set_buffers(size_t samples, uint32_t block_number)
    {
      PICO_STATUS status;

      for(auto aichan = 0; aichan < d_ai_channels; aichan++)
      {
        if(!d_channel_settings[aichan].enabled)
          continue;

        if(d_downsampling_mode == downsampling_mode_t::DOWNSAMPLING_MODE_MIN_MAX_AGG) {
          d_buffers[aichan].reserve(samples);
          d_buffers_min[aichan].reserve(samples);

          status = ps6000SetDataBuffers(d_handle,
              static_cast<PS6000_CHANNEL>(aichan),
              &d_buffers[aichan][0],
              &d_buffers_min[aichan][0],
              samples,
              convert_to_ps6000_ratio_mode(d_downsampling_mode));
        }
        else {
          d_buffers[aichan].reserve(samples);

          status = ps6000SetDataBuffer(d_handle,
              static_cast<PS6000_CHANNEL>(aichan),
              &d_buffers[aichan][0],
              samples,
              convert_to_ps6000_ratio_mode(d_downsampling_mode));
        }

        if(status != PICO_OK) {
          GR_LOG_ERROR(d_logger, "ps6000SetDataBuffer (chan " + std::to_string(aichan)
                + "): " + ps6000_get_error_message(status));
          return make_pico_6000_error_code(status);
        }
      }
      return std::error_code {};
    }

    std::error_code
    picoscope_6000_impl::driver_prefetch_block(size_t samples, size_t block_number)
    {
      auto erc = set_buffers(samples, block_number);
      if(erc){
        return erc;
      }

      uint32_t nr_samples = samples;
      auto status = ps6000GetValues(d_handle,
          0,    // offset
          &nr_samples,
          d_downsampling_factor,
          convert_to_ps6000_ratio_mode(d_downsampling_mode),
          block_number,
          &d_overflow);
      if(status != PICO_OK) {
        GR_LOG_ERROR(d_logger, "ps6000GetValues: " + ps6000_get_error_message(status));
      }

      return make_pico_6000_error_code(status);
    }

    std::error_code
    picoscope_6000_impl::driver_get_rapid_block_data(size_t offset, size_t length,
            size_t waveform, gr_vector_void_star &arrays, std::vector<uint32_t> &status)
    {
      int vec_index = 0;

      for(auto chan_idx = 0; chan_idx < d_ai_channels; chan_idx++, vec_index +=2) {
        if(!d_channel_settings[chan_idx].enabled) {
          continue;
        }

        if (d_overflow & (1 << chan_idx)) {
          status[chan_idx] = channel_status_t::CHANNEL_STATUS_OVERFLOW;
        }
        else {
          status[chan_idx] = 0;
        }

        float voltage_multiplier = d_channel_settings[chan_idx].actual_range / (float)d_max_value;
        float range_offset = d_channel_settings[chan_idx].offset;

        float *out = (float *) arrays.at(vec_index);
        float *err_out = (float *) arrays.at(vec_index + 1);
        int16_t *in = &d_buffers[chan_idx][0] + offset;

        if (d_downsampling_mode == downsampling_mode_t::DOWNSAMPLING_MODE_NONE
                || d_downsampling_mode == downsampling_mode_t::DOWNSAMPLING_MODE_DECIMATE)  {
          for (size_t i = 0; i < length; i++) {
            out[i] = (voltage_multiplier * (float)in[i]) + range_offset;
          }
          // According to specs
          auto error_estimate = d_channel_settings[chan_idx].actual_range * 0.03;
          for (size_t i=0; i<length; i++) {
            err_out[i] = error_estimate;
          }
        }
        else if (d_downsampling_mode == downsampling_mode_t::DOWNSAMPLING_MODE_MIN_MAX_AGG) {
          // this mode is different because samples are in two distinct buffers
          int16_t *in_min = &d_buffers_min[chan_idx][0] + offset;

          for (size_t i = 0; i < length; i++) {
            auto max = (voltage_multiplier * (float)in[i])     + range_offset;
            auto min = (voltage_multiplier * (float)in_min[i]) + range_offset;

            out[i] = (max + min) / 2.0;
            err_out[i] = (max - min) / 4.0;
          }
        }
        else if (d_downsampling_mode == downsampling_mode_t::DOWNSAMPLING_MODE_AVERAGE) {
          for (size_t i = 0; i < length; i++) {
            out[i] = (voltage_multiplier * (float)in[i]) + range_offset;
          }
          // According to specs
          auto error_estimate_single = d_channel_settings[chan_idx].actual_range * 0.03;
          auto error_estimate = std::sqrt(error_estimate_single * error_estimate_single * (float)d_downsampling_factor)
                / std::sqrt((float)d_downsampling_factor);
          for (size_t i = 0; i < length; i++) {
            err_out[i] = error_estimate;
          }
        }
        else {
          assert(false);
        }
      }

      return std::error_code {};
    }

    std::error_code
    picoscope_6000_impl::driver_get_streaming_data(
      gr_vector_void_star &ai_buffers,
      gr_vector_void_star &ai_error_buffers,
      gr_vector_void_star &port_buffers, std::vector<uint32_t> &status, size_t length, size_t &actual)
    {
      length = (size_t)d_stream_no_samples > length ? length : (size_t)d_stream_no_samples;
      actual = length;

      if (length == 0) {
        return std::error_code {};
      }

      int vec_index = 0;

      for(auto chan_idx = 0; chan_idx < d_ai_channels; chan_idx++) {
        if(!d_channel_settings[chan_idx].enabled) {
          vec_index +=2;
          continue;
        }

        double voltage_multiplier = (double)d_channel_settings[chan_idx].actual_range/ (double)d_max_value;
        double range_offset = (double)d_channel_settings[chan_idx].offset;

        float *ai_buffer = (float *)ai_buffers.at(chan_idx);
        float *ai_error_buffer = (float *)ai_error_buffers.at(chan_idx);

        if (d_downsampling_mode == downsampling_mode_t::DOWNSAMPLING_MODE_NONE
                || d_downsampling_mode == downsampling_mode_t::DOWNSAMPLING_MODE_DECIMATE)  {
          for (size_t i=0; i<length; i++) {
            int buff_idx = (i + d_stream_start_index) % d_buffer_size;
            ai_buffer[i] = voltage_multiplier * (float)d_buffers[chan_idx][buff_idx] + range_offset;
          }
          // According to specs
          auto error_estimate = d_channel_settings[chan_idx].actual_range * 0.03;
          for (size_t i=0; i<length; i++) {
            ai_error_buffer[i] = error_estimate;
          }
        }
        else if (d_downsampling_mode == downsampling_mode_t::DOWNSAMPLING_MODE_MIN_MAX_AGG) {
          for (size_t i=0; i<length; i++) {
            int buff_idx = (i + d_stream_start_index) % d_buffer_size;
            auto max = (voltage_multiplier * (float)d_buffers[chan_idx][buff_idx]) + range_offset;
            auto min = (voltage_multiplier * (float)d_buffers_min[chan_idx][buff_idx]) + range_offset;

            ai_buffer[i] = (max + min) / 2.0;
            ai_error_buffer[i] = (max - min) / 4.0;
          }
        }
        else if (d_downsampling_mode == downsampling_mode_t::DOWNSAMPLING_MODE_AVERAGE) {
          for (size_t i=0; i<length; i++) {
            int buff_idx = (i + d_stream_start_index) % d_buffer_size;
            ai_buffer[i] = voltage_multiplier * (float)d_buffers[chan_idx][buff_idx] + range_offset;
          }
          // According to specs
          auto error_estimate_single = d_channel_settings[chan_idx].actual_range * 0.03;
          auto error_estimate = std::sqrt(error_estimate_single * error_estimate_single * (double)d_downsampling_factor)
                / std::sqrt((double)d_downsampling_factor);
          for (size_t i=0; i<length; i++) {
            ai_error_buffer[i] = error_estimate;
          }
        }
        else {
          assert(false);
        }

        vec_index += 2;
      }

      d_stream_no_samples -= length;
      d_stream_start_index = (d_stream_start_index + length) % d_buffer_size;

      return std::error_code {};
    }

    size_t
    picoscope_6000_impl::driver_get_streaming_samples()
    {
      boost::mutex::scoped_lock lock(d_mutex);
      return static_cast<size_t>(d_stream_no_samples);
    }

    std::error_code
    picoscope_6000_impl::driver_poll()
    {
      auto status = PICO_OK;
      do{
        status = ps6000GetStreamingLatestValues(d_handle,
            (ps6000StreamingReady)streaming_callback_redicator_6000, this);
      }while(status == PICO_BUSY);
      return make_pico_6000_error_code(status);
    }

  } /* namespace digitizers */
} /* namespace gr */
