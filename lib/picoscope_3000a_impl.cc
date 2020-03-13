/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <digitizers/status.h>
#include "picoscope_3000a_impl.h"
#include "utils.h"
#include "ps_3000a_defs.h"
#include <cstring>


struct PicoStatus3000aErrc : std::error_category
{
  const char* name() const noexcept override;
  std::string message(int ev) const override;
};

const char* PicoStatus3000aErrc::name() const noexcept
{
  return "Ps3000a";
}

std::string PicoStatus3000aErrc::message(int ev) const
{
  PICO_STATUS status = static_cast<PICO_STATUS>(ev);
  return ps3000a_get_error_message(status);
}

const PicoStatus3000aErrc thePsErrCategory {};

/*!
 * This method is needed because PICO_STATUS is not a distinct type (e.g. an enum)
 * therfore we cannot really hook this into the std error code properly.
 */
std::error_code make_pico_3000a_error_code(PICO_STATUS e)
{
  return {static_cast<int>(e), thePsErrCategory};
}

namespace
{
    boost::mutex g_init_mutex;
}

namespace gr {
  namespace digitizers {


    /*!
     * a structure used for streaming setup
     */
    struct ps3000a_unit_interval_t
    {
      PS3000A_TIME_UNITS unit;
      uint32_t interval;
    };


   /**********************************************************************
    * Converters - helper functions
    *********************************************************************/

    static PS3000A_RANGE
    convert_to_ps3000a_range(float desired_range, float &actual_range)
    {
      if (desired_range <= 0.01) {
        actual_range = 0.01;
        return PS3000A_10MV;
      }
      else if (desired_range <= 0.02) {
        actual_range = 0.02;
        return PS3000A_20MV;
      }
      else if (desired_range <= 0.05){
        actual_range = 0.05;
        return PS3000A_50MV;
      }
      else if (desired_range <= 0.1) {
        actual_range = 0.1;
        return PS3000A_100MV;
      }
      else if (desired_range <= 0.2) {
        actual_range = 0.2;
        return PS3000A_200MV;
      }
      else if (desired_range <= 0.5) {
        actual_range = 0.5;
        return PS3000A_500MV;
      }
      else if (desired_range <= 1.0) {
        actual_range = 1.0;
        return PS3000A_1V;
      }
      else if (desired_range <= 2.0) {
        actual_range = 2.0;
        return PS3000A_2V;
      }
      else if (desired_range <= 5.0) {
        actual_range = 5.0;
        return PS3000A_5V;
      }
      else if (desired_range <= 10.0) {
        actual_range = 10.0;
        return PS3000A_10V;
      }
      else if (desired_range <= 20.0) {
        actual_range = 20.0;
        return PS3000A_20V;
      }
      else {
        actual_range = 50.0;
        return PS3000A_50V;
      }
    }

    /*!
     * Note this function has to be called after the call to the ps3000aSetChannel function, that is
     * just befor the arm!!!
     */
    uint32_t
    picoscope_3000a_impl::convert_frequency_to_ps3000a_timebase(double desired_freq, double &actual_freq)
    {
      // It is assumed that the timebase is calculated like this:
      // (timebase–2) / 125,000,000
      // e.g. timeebase == 3 --> 8ns sample interval
      //
      // Note, for some devices, the above formula might be wrong! To overcome this limitation
      // we use the ps3000aGetTimebase2 function to find the closest possible timebase. The below
      // timebase estimate is therefore used as a fallback only.
      auto time_interval_ns = 1000000000.0 / desired_freq;
      uint32_t timebase_estimate = (static_cast<uint32_t>(time_interval_ns) / 8) + 2;

      // In order to cover for all possible 30000 series devices, we use ps3000aGetTimebase2
      // function to get step size in ns between timebase 3 and 4. Based on that the actual timebase
      // is calculated.
      int32_t dummy;
      std::array<float, 2> time_interval_ns_34;

      for (auto i = 0; i<2; i++) {
        auto status = ps3000aGetTimebase2(d_handle, 3 + i, 1024, &time_interval_ns_34[i], 0, &dummy, 0);
        if(status != PICO_OK)
        {
          GR_LOG_NOTICE(d_logger, "timebase cannot be obtained: " + ps3000a_get_error_message(status));
          GR_LOG_NOTICE(d_logger, "    estimated timebase will be used...");

          float time_interval_ns;
          status = ps3000aGetTimebase2(d_handle, timebase_estimate, 1024, &time_interval_ns, 0, &dummy, 0);
          if(status != PICO_OK)
          {
              std::ostringstream message;
              message << "Exception in " << __FILE__ << ":" << __LINE__ << ": local time " << timebase_estimate << " Error: " << ps3000a_get_error_message(status);
              throw std::runtime_error(message.str());
          }

          actual_freq = 1000000000.0 / time_interval_ns;
          if (actual_freq != desired_freq)
          {
              std::cout  << "Critical Error in " << __FILE__ << ":" << __LINE__ << ": Desired and actual frequency do not match. desired: " << desired_freq << " actual: " << actual_freq <<  std::endl ;
              exit(1);
          }
          return timebase_estimate;
        }
      }

      // Calculate steps between timebase 3 and 4 and correct start_timebase estimate based on that
      auto step = time_interval_ns_34[1] - time_interval_ns_34[0];
      timebase_estimate = static_cast<uint32_t>((time_interval_ns - time_interval_ns_34[0]) / step) + 3;

      // The below code iterates trought the neighbouring timebases in order to find the best
      // match. In principle we could check only timebases on the left and right but since first
      // three timebases are in most cases special we make search space a bit bigger.
      const int search_space = 8;
      std::array<float, search_space> timebases;
      std::array<float, search_space> error_estimates;

      uint32_t start_timebase = timebase_estimate > (search_space / 2) ? timebase_estimate - (search_space / 2) : 0;

      for (auto i = 0; i < search_space; i++) {

        float obtained_time_interval_ns;
        auto status = ps3000aGetTimebase2(d_handle, start_timebase + i, 1024, &obtained_time_interval_ns, 0, &dummy, 0);
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

      if (actual_freq != desired_freq)
      {
          std::cout  << "Critical Error in " << __FILE__ << ":" << __LINE__ << ": Desired and actual frequency do not match. desired: " << desired_freq << " actual: " << actual_freq <<  std::endl ;
          exit(1);
      }
      return start_timebase + distance;
    }

    ps3000a_unit_interval_t
    convert_frequency_to_ps3000a_time_units_and_interval(double desired_freq, double &actual_freq)
    {
      ps3000a_unit_interval_t unint;
      auto interval = 1.0 / desired_freq;

      if(interval < 0.000001) {
        unint.unit = PS3000A_PS;
        unint.interval = static_cast<uint32_t>(1000000000000.0 / desired_freq);
        actual_freq = 1000000000000.0 / static_cast<double>(unint.interval);
      }
      else if(interval < 0.001) {
        unint.unit = PS3000A_NS;
        unint.interval = static_cast<uint32_t>(1000000000.0 / desired_freq);
        actual_freq = 1000000000.0 / static_cast<double>(unint.interval);
      }
      else if(interval < 0.1){
        unint.unit = PS3000A_US;
        unint.interval = static_cast<uint32_t>(1000000.0 / desired_freq );
        actual_freq = 1000000.0 / static_cast<double>(unint.interval);
      }
      else {
        unint.unit = PS3000A_MS;
        unint.interval = static_cast<uint32_t>(1000.0 / desired_freq);
        actual_freq = 1000.0 / static_cast<double>(unint.interval);
      }

      return unint;
    }

    static PS3000A_RATIO_MODE
    convert_to_ps3000a_ratio_mode(downsampling_mode_t mode)
    {
      switch(mode)
      {
      case downsampling_mode_t::DOWNSAMPLING_MODE_MIN_MAX_AGG:
        return PS3000A_RATIO_MODE_AGGREGATE;
      case downsampling_mode_t::DOWNSAMPLING_MODE_DECIMATE:
        return PS3000A_RATIO_MODE_DECIMATE;
      case downsampling_mode_t::DOWNSAMPLING_MODE_AVERAGE:
        return PS3000A_RATIO_MODE_AVERAGE;
      case downsampling_mode_t::DOWNSAMPLING_MODE_NONE:
      default:
        return PS3000A_RATIO_MODE_NONE;
      }
    }

    PS3000A_THRESHOLD_DIRECTION 
    convert_to_ps3000a_threshold_direction(trigger_direction_t direction)
    {
      switch(direction)
      {
      case trigger_direction_t::TRIGGER_DIRECTION_RISING:
        return PS3000A_RISING;
      case trigger_direction_t::TRIGGER_DIRECTION_FALLING:
        return PS3000A_FALLING;
      case trigger_direction_t::TRIGGER_DIRECTION_LOW:
        return PS3000A_BELOW;
      case trigger_direction_t::TRIGGER_DIRECTION_HIGH:
        return PS3000A_ABOVE;
      default:
        std::ostringstream message;
        message << "Exception in " << __FILE__ << ":" << __LINE__ << ": unsupported trigger direction:" << direction;
        throw std::runtime_error(message.str());
      }
    };

    PS3000A_DIGITAL_DIRECTION
    convert_to_ps3000a_digital_direction(trigger_direction_t direction)
    {
      switch(direction)
      {
      case trigger_direction_t::TRIGGER_DIRECTION_RISING:
        return PS3000A_DIGITAL_DIRECTION_RISING;
      case trigger_direction_t::TRIGGER_DIRECTION_FALLING:
        return PS3000A_DIGITAL_DIRECTION_FALLING;
      case trigger_direction_t::TRIGGER_DIRECTION_LOW:
        return PS3000A_DIGITAL_DIRECTION_LOW;
      case trigger_direction_t::TRIGGER_DIRECTION_HIGH:
        return PS3000A_DIGITAL_DIRECTION_HIGH;
      default:
          std::ostringstream message;
          message << "Exception in " << __FILE__ << ":" << __LINE__ << ": unsupported trigger direction:" << direction;
          throw std::runtime_error(message.str());
      }
    };

    int16_t
    convert_voltage_to_ps3000a_raw_logic_value(double value)
    {
      double max_logical_voltage = 5.0;

      if (value > max_logical_voltage)
      {
        std::ostringstream message;
        message << "Exception in " << __FILE__ << ":" << __LINE__ << ": max logical level is: " << max_logical_voltage;
        throw std::invalid_argument(message.str());
      }

      return (int16_t) ((value / max_logical_voltage) * (double)PS3000A_MAX_LOGIC_LEVEL);
    }

    PS3000A_CHANNEL
    convert_to_ps3000a_channel(const std::string &source)
    {
      if (source == "A") {
        return PS3000A_CHANNEL_A;
      }
      else if (source == "B") {
        return PS3000A_CHANNEL_B;
      }
      else if (source == "C") {
        return PS3000A_CHANNEL_C;
      }
      else if (source == "D") {
        return PS3000A_CHANNEL_D;
      }
      else if (source == "EXTERNAL") {
        return PS3000A_EXTERNAL;
      }
      else {
        // return invalid value
        return PS3000A_MAX_TRIGGER_SOURCES;
      }
    }

    /**********************************************************************
     * Structors
     *********************************************************************/

    picoscope_3000a::sptr
    picoscope_3000a::make(std::string serial_number, bool auto_arm)
    {
      std::vector<int> out_signature;

      for(int i = 0; i < PS3000A_MAX_CHANNELS; i++) {
        out_signature.push_back(sizeof(float));
        out_signature.push_back(sizeof(float));
      }

      for(int i = 0; i < 2; i++)
        out_signature.push_back(sizeof(char));

      return gnuradio::get_initial_sptr
        (new picoscope_3000a_impl(serial_number, out_signature, auto_arm));
    }

    picoscope_3000a_impl::picoscope_3000a_impl(std::string serial_number, std::vector<int> out_signature, bool auto_arm)
      : gr::sync_block("picoscope_3000a",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::makev(10, 10, out_signature)),
        picoscope_impl(serial_number,
              PS3000A_MAX_CHANNELS,
              2,     // it seems no 3000 series device supports 4 ports
              auto_arm,
              255,   // max raw value
              0.03), // vertical precision
        d_handle(-1),
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

    picoscope_3000a_impl::~picoscope_3000a_impl()
    {
      driver_close();
    }


    /**********************************************************************
     * Driver implementation
     *********************************************************************/

    std::string
    picoscope_3000a_impl::get_unit_info_topic(PICO_INFO info)
    {
      char line[40];
      int16_t required_size;

      auto status = ps3000aGetUnitInfo(d_handle, reinterpret_cast<int8_t *>(line), sizeof(line),
                &required_size, info);
      if (status == PICO_OK) {
        return std::string(line, required_size);
      }
      else {
        return "NA";
      }
    }

    std::string
    picoscope_3000a_impl::get_driver_version()
    {
      const std::string prefix = "PS3000A Linux Driver, ";
      auto version = get_unit_info_topic(PICO_DRIVER_VERSION);

      auto i = version.find(prefix);
      if (i != std::string::npos)
         version.erase(i, prefix.length());

      return version;
    }

    std::string
    picoscope_3000a_impl::get_hardware_version()
    {
      if (!d_initialized)
        return "NA";
      return get_unit_info_topic(PICO_HARDWARE_VERSION);
    }

    std::error_code
    picoscope_3000a_impl::driver_initialize()
    {
      PICO_STATUS status;

      // Required to force sequence execution of open unit calls...
      boost::mutex::scoped_lock init_guard(g_init_mutex);

      // take any if serial number is not provided (usefull for testing purposes)
      if (d_serial_number.empty()) {
        status = ps3000aOpenUnit(&(d_handle), NULL);
      }
      else {
        status = ps3000aOpenUnit(&(d_handle), (int8_t*)d_serial_number.c_str());
      }

      // ignore ext. power not connected error/warning
      if (status == PICO_POWER_SUPPLY_NOT_CONNECTED
              || status == PICO_USB3_0_DEVICE_NON_USB3_0_PORT) {
        status = ps3000aChangePowerSource(d_handle, status);
        if (status == PICO_POWER_SUPPLY_NOT_CONNECTED
                || status == PICO_USB3_0_DEVICE_NON_USB3_0_PORT) {
          status = ps3000aChangePowerSource(d_handle, status);
        }
      }

      if (status != PICO_OK) {
        GR_LOG_ERROR(d_logger, "open unit failed: " + ps3000a_get_error_message(status));
        return make_pico_3000a_error_code(status);
      }

      // maximum value is used for conversion to volts
      status = ps3000aMaximumValue(d_handle, &d_max_value);
      if (status != PICO_OK) {
        ps3000aCloseUnit(d_handle);
        GR_LOG_ERROR(d_logger, "ps3000aMaximumValue: " + ps3000a_get_error_message(status));
        return make_pico_3000a_error_code(status);
      }

      char line[40];
      int16_t required_size;

      // It would be nicer if the number of channels is communicated to the base driver in the form
      // of function call or something similar.
      status = ps3000aGetUnitInfo(d_handle, reinterpret_cast<int8_t *>(line), sizeof(line),
              &required_size, PICO_VARIANT_INFO);
      if (status != PICO_OK) {
        // this error is ignored
        GR_LOG_WARN(d_logger, "ps3000aGetUnitInfo failed: " + ps3000a_get_error_message(status));
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
    picoscope_3000a_impl::driver_configure()
    {
      assert(d_ai_channels <= PS3000A_MAX_CHANNELS);
      assert(d_ports <= PS3000A_MAX_DIGITAL_PORTS);

      PICO_STATUS status;

      if (d_acquisition_mode == acquisition_mode_t::RAPID_BLOCK) {

        int32_t max_samples;
        status = ps3000aMemorySegments(d_handle, d_nr_captures, &max_samples);
        if(status != PICO_OK) {
          GR_LOG_ERROR(d_logger, "ps3000aMemorySegments: " + ps3000a_get_error_message(status));
          return make_pico_3000a_error_code(status);
        }

        status = ps3000aSetNoOfCaptures(d_handle, d_nr_captures);
        if(status != PICO_OK) {
          GR_LOG_ERROR(d_logger, "ps3000aSetNoOfCaptures: " + ps3000a_get_error_message(status));
          return make_pico_3000a_error_code(status);
        }
      }

      // configure analog channels
      for (auto i = 0; i < d_ai_channels; i++) {
        auto enabled = d_channel_settings[i].enabled;
        auto dc_coupled = d_channel_settings[i].dc_coupled ? PS3000A_DC : PS3000A_AC;
        auto range = convert_to_ps3000a_range(
                d_channel_settings[i].range, d_channel_settings[i].actual_range);
        auto offset = d_channel_settings[i].offset;

        status = ps3000aSetChannel(d_handle,
                static_cast<PS3000A_CHANNEL>(i), enabled, dc_coupled, range, offset);
        if(status != PICO_OK) {
          GR_LOG_ERROR(d_logger, "ps3000aSetChannel (chan " + std::to_string(i)
              + "): " + ps3000a_get_error_message(status));
          return make_pico_3000a_error_code(status);
        }
      }

      // and digital ports
      for (auto port = 0; port < d_ports; port++) {
        status = ps3000aSetDigitalPort(
                  d_handle,
                  static_cast<PS3000A_DIGITAL_PORT>(PS3000A_DIGITAL_PORT0 + port),
                  d_port_settings[port].enabled,
                  convert_voltage_to_ps3000a_raw_logic_value(d_port_settings[port].logic_level));
        if(status != PICO_OK) {
            GR_LOG_ERROR(d_logger, "ps3000aSetDigitalPort (port " + std::to_string(port)
                + "): " + ps3000a_get_error_message(status));
            return make_pico_3000a_error_code(status);
        }
      }

      // apply trigger configuration
      if (d_trigger_settings.is_analog()
              && d_acquisition_mode == acquisition_mode_t::RAPID_BLOCK)
      {
        status = ps3000aSetSimpleTrigger(d_handle,
              true,  // enable
              convert_to_ps3000a_channel(d_trigger_settings.source),
              convert_voltage_to_ps3000a_raw_logic_value(d_trigger_settings.threshold),
              convert_to_ps3000a_threshold_direction(d_trigger_settings.direction),
              0,     // delay
             -1);    // auto trigger
        if(status != PICO_OK) {
          GR_LOG_ERROR(d_logger, "ps3000aSetSimpleTrigger: " + ps3000a_get_error_message(status));
          return make_pico_3000a_error_code(status);
        }
      }
      else if (d_trigger_settings.is_digital()
              && d_acquisition_mode == acquisition_mode_t::RAPID_BLOCK)
      {
        PS3000A_DIGITAL_CHANNEL_DIRECTIONS pinTrig;
        pinTrig.channel = static_cast<PS3000A_DIGITAL_CHANNEL>(d_trigger_settings.pin_number);
        pinTrig.direction = convert_to_ps3000a_digital_direction(d_trigger_settings.direction);

        status = ps3000aSetTriggerDigitalPortProperties(d_handle, &pinTrig, 1);
        if(status != PICO_OK) {
          GR_LOG_ERROR(d_logger, "ps3000aSetTriggerDigitalPortProperties: " + ps3000a_get_error_message(status));
          return make_pico_3000a_error_code(status);
        }

        PS3000A_TRIGGER_CONDITIONS_V2 conds = {
              PS3000A_CONDITION_DONT_CARE,
              PS3000A_CONDITION_DONT_CARE,
              PS3000A_CONDITION_DONT_CARE,
              PS3000A_CONDITION_DONT_CARE,
              PS3000A_CONDITION_DONT_CARE,
              PS3000A_CONDITION_DONT_CARE,
              PS3000A_CONDITION_DONT_CARE,
              PS3000A_CONDITION_TRUE
        };
        status = ps3000aSetTriggerChannelConditionsV2(d_handle, &conds, 1);
        if(status != PICO_OK) {
            GR_LOG_ERROR(d_logger, "ps3000aSetTriggerChannelConditionsV2: " + ps3000a_get_error_message(status));
            return make_pico_3000a_error_code(status);
        }
      }
      else {
        // disable triggers...
        PS3000A_TRIGGER_CONDITIONS_V2 conds = {
              PS3000A_CONDITION_DONT_CARE,
              PS3000A_CONDITION_DONT_CARE,
              PS3000A_CONDITION_DONT_CARE,
              PS3000A_CONDITION_DONT_CARE,
              PS3000A_CONDITION_DONT_CARE,
              PS3000A_CONDITION_DONT_CARE,
              PS3000A_CONDITION_DONT_CARE,
              PS3000A_CONDITION_DONT_CARE
        };
        status = ps3000aSetTriggerChannelConditionsV2(d_handle, &conds, 1);
        if(status != PICO_OK) {
          GR_LOG_ERROR(d_logger, "ps3000aSetTriggerChannelConditionsV2: " + ps3000a_get_error_message(status));
          return make_pico_3000a_error_code(status);
        }
      }

      return std::error_code{};
    }

    void
    rapid_block_callback_redirector_3000a(int16_t handle, PICO_STATUS status, void *vobj)
    {
      static_cast<picoscope_3000a_impl *>(vobj)->rapid_block_callback(handle, status);
    }

    void
    picoscope_3000a_impl::rapid_block_callback(int16_t handle, PICO_STATUS status)
    {
      auto errc = make_pico_3000a_error_code(status);
      notify_data_ready(errc);
    }

    std::error_code
    picoscope_3000a_impl::driver_arm()
    {
      if(d_acquisition_mode == acquisition_mode_t::RAPID_BLOCK) {
          uint32_t timebase =  convert_frequency_to_ps3000a_timebase(d_samp_rate, d_actual_samp_rate);

          auto status = ps3000aRunBlock(d_handle,
                  d_pre_samples,   // pre-triggersamples
                  d_post_samples,  // post-trigger samples
                  timebase,        // timebase
                  0,               // oversample
                  NULL,            // time indispossed
                  0,               // segment index
                  (ps3000aBlockReady)rapid_block_callback_redirector_3000a,
                  this);
          if(status != PICO_OK) {
            GR_LOG_ERROR(d_logger, "ps3000aRunBlock: " + ps3000a_get_error_message(status));
            return make_pico_3000a_error_code(status);
          }
      }
      else {
        set_buffers(d_driver_buffer_size, 0);

        ps3000a_unit_interval_t unit_int = convert_frequency_to_ps3000a_time_units_and_interval(
                d_samp_rate, d_actual_samp_rate);

        auto status = ps3000aRunStreaming(d_handle,
            &(unit_int.interval), // sample interval
            unit_int.unit,        // time unit of sample interval
            0,                    // pre-triggersamples (unused)
            d_driver_buffer_size,
            false,
            d_downsampling_factor,
            convert_to_ps3000a_ratio_mode(d_downsampling_mode),
            d_driver_buffer_size);

        if(status != PICO_OK) {
          GR_LOG_ERROR(d_logger, "ps3000aRunStreaming: " + ps3000a_get_error_message(status));
          return make_pico_3000a_error_code(status);
        }
      }

      return std::error_code{};
    }

    std::error_code
    picoscope_3000a_impl::driver_disarm()
    {
      auto status = ps3000aStop(d_handle);
      if(status != PICO_OK) {
        GR_LOG_ERROR(d_logger, "ps3000aStop: " + ps3000a_get_error_message(status));
      }

      return make_pico_3000a_error_code(status);
    }

    std::error_code
    picoscope_3000a_impl::driver_close()
    {
      if (d_handle == -1) {
        return std::error_code{};
      }

      auto status = ps3000aCloseUnit(d_handle);
      if(status != PICO_OK) {
        GR_LOG_ERROR(d_logger, "ps3000aCloseUnit: " + ps3000a_get_error_message(status));
      }

      d_handle = -1;
      return make_pico_3000a_error_code(status);
    }

    std::error_code
    picoscope_3000a_impl::set_buffers(size_t samples, uint32_t block_number)
    {
      PICO_STATUS status;

      for(auto aichan = 0; aichan < d_ai_channels; aichan++)
      {
        if(!d_channel_settings[aichan].enabled)
          continue;

        if(d_downsampling_mode == downsampling_mode_t::DOWNSAMPLING_MODE_MIN_MAX_AGG) {
          d_buffers[aichan].reserve(samples);
          d_buffers_min[aichan].reserve(samples);

          status = ps3000aSetDataBuffers(d_handle,
              static_cast<PS3000A_CHANNEL>(aichan),
              &d_buffers[aichan][0],
              &d_buffers_min[aichan][0],
              samples,
              block_number,
              convert_to_ps3000a_ratio_mode(d_downsampling_mode));
        }
        else {
          d_buffers[aichan].reserve(samples);

          status = ps3000aSetDataBuffer(d_handle,
              static_cast<PS3000A_CHANNEL>(aichan),
              &d_buffers[aichan][0],
              samples,
              block_number,
              convert_to_ps3000a_ratio_mode(d_downsampling_mode));
        }

        if(status != PICO_OK) {
          GR_LOG_ERROR(d_logger, "ps3000aSetDataBuffer (chan " + std::to_string(aichan)
                + "): " + ps3000a_get_error_message(status));
          return make_pico_3000a_error_code(status);
        }
      }

      for (auto port = 0; port < d_ports; port++) {
        if (!d_port_settings[port].enabled) {
          continue;
        }

        d_port_buffers[port].reserve(samples);

        status = ps3000aSetDataBuffer(d_handle,
              static_cast<PS3000A_CHANNEL>(PS3000A_DIGITAL_PORT0 + port),
              &d_port_buffers[port][0],
              samples,
              block_number,
              convert_to_ps3000a_ratio_mode(d_downsampling_mode));
        if(status != PICO_OK) {
          GR_LOG_ERROR(d_logger, "ps3000aSetDataBuffer (port " + std::to_string(port)
                + "): " + ps3000a_get_error_message(status));
          return make_pico_3000a_error_code(status);
        }
      }

      return std::error_code {};
    }

    std::error_code
    picoscope_3000a_impl::driver_prefetch_block(size_t samples, size_t block_number)
    {
      auto erc = set_buffers(samples, block_number);
      if(erc){
        return erc;
      }

      uint32_t nr_samples = samples;
      auto status = ps3000aGetValues(d_handle,
          0,    // offset
          &nr_samples,
          d_downsampling_factor,
          convert_to_ps3000a_ratio_mode(d_downsampling_mode),
          block_number,
          &d_overflow);
      if(status != PICO_OK) {
        GR_LOG_ERROR(d_logger, "ps3000aGetValues: " + ps3000a_get_error_message(status));
      }

      return make_pico_3000a_error_code(status);
    }

    std::error_code
    picoscope_3000a_impl::driver_get_rapid_block_data(size_t offset, size_t length,
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

        float *out = (float *) arrays.at(vec_index);
        float *err_out = (float *) arrays.at(vec_index + 1);
        int16_t *in = &d_buffers[chan_idx][0] + offset;

        if (d_downsampling_mode == downsampling_mode_t::DOWNSAMPLING_MODE_NONE
                || d_downsampling_mode == downsampling_mode_t::DOWNSAMPLING_MODE_DECIMATE)  {
          for (size_t i = 0; i < length; i++) {
            out[i] = (voltage_multiplier * (float)in[i]);
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
            auto max = (voltage_multiplier * (float)in[i]);
            auto min = (voltage_multiplier * (float)in_min[i]);

            out[i] = (max + min) / 2.0;
            err_out[i] = (max - min) / 4.0;
          }
        }
        else if (d_downsampling_mode == downsampling_mode_t::DOWNSAMPLING_MODE_AVERAGE) {
          for (size_t i = 0; i < length; i++) {
            out[i] = (voltage_multiplier * (float)in[i]);
          }
          // According to specs
          auto error_estimate_single = d_channel_settings[chan_idx].actual_range * 0.03;
          auto error_estimate = error_estimate_single / std::sqrt((float)d_downsampling_factor);
          for (size_t i = 0; i < length; i++) {
            err_out[i] = error_estimate;
          }
        }
        else {
          assert(false);
        }
      }

      for (auto port = 0; port < d_ports; port++, vec_index++) {

        if (!d_port_settings[port].enabled) {
          continue;
        }

        uint8_t *out = static_cast<uint8_t *>(arrays.at(vec_index));
        int16_t *in = &d_port_buffers[port][0] + offset;

        for (size_t i=0; i<length; i++) {
          out[i] = static_cast<uint8_t>(0x00ff & in[i]);
        }
      }

      return std::error_code {};
    }

    std::error_code
    picoscope_3000a_impl::driver_poll()
    {
      auto status = ps3000aGetStreamingLatestValues(d_handle,
              (ps3000aStreamingReady)invoke_streaming_callback, &d_streaming_callback);
      if (status == PICO_BUSY || status == PICO_DRIVER_FUNCTION) {
        return std::error_code {};
      }

      return make_pico_3000a_error_code(status);
    }

  } /* namespace digitizers */
} /* namespace gr */
