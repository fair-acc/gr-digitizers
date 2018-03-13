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

#ifndef INCLUDED_DIGITIZERS_DIGITIZER_BLOCK_IMPL_H
#define INCLUDED_DIGITIZERS_DIGITIZER_BLOCK_IMPL_H

#include <digitizers/digitizer_block.h>
#include "utils.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/chrono.hpp>
#include <system_error>



namespace gr {
  namespace digitizers {

    enum class digitizer_block_errc
    {
      Stopped = 1,
      Interrupted = 10,   // did not respond in time
    };

    std::error_code make_error_code(digitizer_block_errc e);

  }
}

// Hook in our error code visible to the std system error
namespace std
{
  template <>
  struct is_error_code_enum<gr::digitizers::digitizer_block_errc> : true_type {};
}


namespace gr {
  namespace digitizers {


  /**********************************************************************
   * Helpers and struct definitions
   **********************************************************************/

   /*!
    * \brief Returns nanoseconds since UNIX epoch.
    */
    inline int64_t
    get_timestamp_utc_ns() {

      auto timepoint = boost::chrono::high_resolution_clock::now();
      auto nanosecods_since_epoch= boost::chrono::duration_cast<boost::chrono::nanoseconds>(
              timepoint.time_since_epoch());
      return nanosecods_since_epoch.count();
    }

    /*!
     * A helper struct for keeping track which samples have been already processes
     * in rapid block mode.
     */
    struct rapid_block_state_t
    {
      enum State {WAITING, READING_PART1, READING_THE_REST};

      rapid_block_state_t() :
          state(WAITING),
          waveform_count(0),
          waveform_idx(0),
          offset(0),
          samples_left(0)
      {}

      State state;

      int waveform_count;
      int waveform_idx;    // index of the waveform we are currently reading
      int offset;          // reading offset
      int samples_left;

      void to_wait()
      {
        state = rapid_block_state_t::WAITING;
      }

      void initialize(int nr_waveforms)
      {
        state = rapid_block_state_t::READING_PART1;
        waveform_idx = 0;
        waveform_count = nr_waveforms;
      }

      void set_waveform_params(uint32_t offset_samps, uint32_t samples_to_read)
      {
        offset = offset_samps;
        samples_left = samples_to_read;
      }

      // update state
      void update_state(uint32_t nsamples)
      {
        offset += nsamples;
        samples_left -= nsamples;

        if (samples_left > 0) {
          state = rapid_block_state_t::READING_THE_REST;
        }
        else {
          waveform_idx++;
          if (waveform_idx >= waveform_count) {
            state = rapid_block_state_t::WAITING;
          }
          else {
            state = rapid_block_state_t::READING_PART1;
          }
        }
      }
    };

    struct streaming_state_t
    {
      void
      initialize(int aichan_count, int port_count, size_t buffer_size)
      {
          last_state = 0;
          trigger_samples = 0;
      }

      int last_state = 0;
      int trigger_samples;  // samples until the end-of-trigger
    };


    /*!
     * A struct holding AI channel settings.
     */
    struct channel_setting_t
    {
      channel_setting_t()
        : range(2.0),
          actual_range(2.0),
          offset(0.0),
          enabled(false),
          dc_coupled(false)
      {}

      float range;
      float actual_range;
      float offset;
      bool enabled;
      bool dc_coupled;
    };

    struct port_setting_t
    {
      port_setting_t()
        : logic_level(1.5),
          enabled(false)
      {}

      float logic_level;
      bool enabled;
    };

    static const std::string TRIGGER_NONE_SOURCE    = "NONE";
    static const std::string TRIGGER_DIGITAL_SOURCE = "DI";

    struct trigger_setting_t
    {

      trigger_setting_t()
        : source(TRIGGER_NONE_SOURCE),
          threshold(0),
          direction(TRIGGER_DIRECTION_RISING),
          pin_number(0)
      {}

      bool
      is_enabled() const
      {
        return source != TRIGGER_NONE_SOURCE;
      }

      bool
      is_digital() const
      {
        return is_enabled() && source == TRIGGER_DIGITAL_SOURCE;
      }

      bool
      is_analog() const
      {
        return is_enabled() && source != TRIGGER_DIGITAL_SOURCE;
      }

      std::string source;
      float threshold;   // AI only
      trigger_direction_t direction;
      int pin_number;    // DI only
    };


    // GNU Radio block implementation header
    class digitizer_block_impl : virtual public digitizer_block
    {

    /**********************************************************************
     * Public API calls (see digitizer_block.h for docs)
     **********************************************************************/

     public:

      static const int MAX_SUPPORTED_AI_CHANNELS = 16;
      static const int MAX_SUPPORTED_PORTS = 8;

      acquisition_mode_t get_acquisition_mode() override;

      void set_samples(int samples, int pre_samples = 0) override;

      void set_samp_rate(double rate) override;

      double get_samp_rate() override;

      void set_buffer_size(int buffer_size) override;

      void set_auto_arm(bool auto_arm) override;

      void set_trigger_once(bool auto_arm) override;

      void set_rapid_block(int nr_captures) override;

      void set_streaming(double poll_rate=0.001) override;

      void set_downsampling(downsampling_mode_t mode, int downsample_factor) override;

      void set_aichan(const std::string &id, bool enabled, double range, bool dc_coupling, double range_offset = 0) override;

      void set_aichan_range(const std::string &id, double range, double range_offset = 0) override;

      void set_aichan_trigger(const std::string &id, trigger_direction_t direction, double threshold) override;

      void set_diport(const std::string &id, bool enabled, double thresh_voltage) override;

      void set_di_trigger(uint32_t pin, trigger_direction_t direction) override;

      void disable_triggers() override;

      void initialize() override;

      void configure() override;

      void arm() override;

      bool is_armed() override;

      void disarm() override;

      void close() override;

      std::vector<err::error_info_t> get_errors();

      bool start() override;

      bool stop() override;

      // Where all the action really happens
      int work(int noutput_items, gr_vector_const_void_star &input_items, gr_vector_void_star &output_items) override;

     /**********************************************************************
      * Structors
      **********************************************************************/

      virtual ~digitizer_block_impl();

     protected:

      digitizer_block_impl(int ai_channels, int di_ports=0, bool auto_arm=true);

     /**********************************************************************
      * Driver interface and handlers
      **********************************************************************/

      virtual std::error_code driver_initialize() = 0;

      virtual std::error_code driver_configure() = 0;

      virtual std::error_code driver_arm() = 0;

      virtual std::error_code driver_disarm() = 0;

      virtual std::error_code driver_close() = 0;

      virtual std::error_code driver_poll() = 0;

      /*!
       * This function should be called when data is ready (rapid block only?)
       */
      void notify_data_ready(std::error_code errc);

      std::error_code wait_data_ready();

      void clear_data_ready();

      /*!
       * Note offset and length is in non-decimated samples
       */
      virtual std::error_code driver_prefetch_block(size_t length, size_t block_number) = 0;

      /*!
       * By offset and length we mean decimated samples, and offset is offset within the subparts of data.
       */
      virtual std::error_code driver_get_rapid_block_data(size_t offset, size_t length, size_t waveform,
              gr_vector_void_star &arrays, std::vector<uint32_t> &status) = 0;

      virtual std::error_code driver_get_streaming_data(
              gr_vector_void_star &ai_buffers,
              gr_vector_void_star &ai_error_buffers,
              gr_vector_void_star &port_buffers, std::vector<uint32_t> &status, size_t length, size_t &actual) = 0;

      /*!
       * \brief Number of streaming samples available in the driver buffer or device.
       */
      virtual size_t driver_get_streaming_samples() = 0;

      int work_rapid_block(int noutput_items, gr_vector_void_star &output_items);

      int work_stream(int noutput_items, gr_vector_void_star &output_items);

     /**********************************************************************
      * Helpers
      **********************************************************************/

      uint32_t get_pre_trigger_samples_with_downsampling() const;

      uint32_t get_post_trigger_samples_with_downsampling() const;

      /*!
       * Returns number of pre-triggers samples plus number of post-trigger samples.
       */
      uint32_t get_block_size() const;

      uint32_t get_block_size_with_downsampling() const;

      int convert_to_aichan_idx(const std::string &id) const;

      int convert_to_port_idx(const std::string &id) const;

      /*!
       * \brief Distance between output items in seconds.
       *
       * This function takes into account downsampling factor (if set).
       */
      double get_timebase_with_downsampling() const;

      /*!
       * \brief This function will search for an edge (trigger) in the streaming buffer.
       *
       * Only next n samples are taken into account. If triggers are not enabled -1 is returned.
       * It is also important to note that trigger is searched for only in the following range:
       *   [d_pre_samples + read_idx, d_pre_samples + read_idx + n)
       *
       * Since this function returns relative offset of the detected edge, the pre-trigger samples
       * needs to be accoutned for:
       *   offset = retval - d_pre_samples
       */
      int find_stream_trigger(int n);

      void reset_stream_trigger();

      void initialize_stream_buffers();

      void shift_stream_data(uint32_t samples_to_keep);


    /**********************************************************************
     * Members
     *********************************************************************/

      // Sample rate in Hz
      double d_samp_rate;
      double d_actual_samp_rate;

      // Number of pre- and post-trigger samples the user wants to see on the outputs.
      // Note when calculating actual number of pre- and post-trigger samples one should
      // take into account the user delay and realignment delay.

      uint32_t d_samples;
      uint32_t d_pre_samples;

      // Number of captures in rapid block mode
      uint32_t d_nr_captures;

      // Buffer size in streaming mode
      uint32_t d_buffer_size;

      acquisition_mode_t d_acquisition_mode;
      double d_poll_rate;
      downsampling_mode_t d_downsampling_mode;
      uint32_t d_downsampling_factor;

      // Number of channels and ports
      int d_ai_channels;
      int d_ports;

      // Channel and trigger settings
      std::array<channel_setting_t, MAX_SUPPORTED_AI_CHANNELS> d_channel_settings;
      std::array<port_setting_t, MAX_SUPPORTED_PORTS> d_port_settings;
      trigger_setting_t d_trigger_settings;

      std::vector<uint32_t> d_status;

      // Flags
      bool d_initialized;
      bool d_armed;
      bool d_auto_arm;
      bool d_trigger_once;
      bool d_was_triggered_once;
      bool d_timebase_published;

     private:

      // Acquisition, note boost constructs are used in order for the GR
      // scheduler to be able to interrupt worker thread on stop.
      boost::condition_variable d_data_rdy_cv;
      bool d_data_rdy;
      boost::mutex d_mutex;
      std::error_code d_data_rdy_errc;

      // Worker stuff
      rapid_block_state_t d_bstate;
      streaming_state_t d_sstate;

      int d_trigger_state;

      std::vector<std::vector<float>> d_ai_buffers;
      std::vector<std::vector<float>> d_ai_error_buffers;
      std::vector<std::vector<uint8_t>> d_port_buffers;

      // A vector holding status information for pre-trigger number of samples located in
      // the buffer
      std::vector<uint32_t> d_status_pre;

      int d_read_idx;
      uint32_t d_buffer_samples;

      err::error_buffer_t d_errors;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_DIGITIZER_BLOCK_IMPL_H */

