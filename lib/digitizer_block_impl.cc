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

#include "digitizer_block_impl.h"
#include <thread>
#include <chrono>
#include <boost/lexical_cast.hpp>
#include <digitizers/tags.h>

namespace gr {
  namespace digitizers {


   /**********************************************************************
    * Error codes
    *********************************************************************/

    struct digitizer_block_err_category : std::error_category
    {
      const char* name() const noexcept override;
      std::string message(int ev) const override;
    };

    const char*
    digitizer_block_err_category::name() const noexcept
    {
     return "digitizer_block";
    }

    std::string
    digitizer_block_err_category::message(int ev) const
    {
      switch (static_cast<digitizer_block_errc>(ev))
      {
       case digitizer_block_errc::Interrupted:
        return "Wit interrupted";

       default:
        return "(unrecognized error)";
      }
   }

   const digitizer_block_err_category __digitizer_block_category {};

   std::error_code
   make_error_code(digitizer_block_errc e)
   {
     return {static_cast<int>(e), __digitizer_block_category};
   }


   /**********************************************************************
    * Structors
    *********************************************************************/

   digitizer_block_impl::digitizer_block_impl(int ai_channels, int di_ports, bool auto_arm) :
       d_samp_rate(10000),
       d_actual_samp_rate(d_samp_rate),
       d_samples(10000),
       d_pre_samples(1000),
       d_nr_captures(1),
       d_buffer_size(10000),
       d_acquisition_mode(acquisition_mode_t::STREAMING),
       d_poll_rate(0.001),
       d_downsampling_mode(downsampling_mode_t::DOWNSAMPLING_MODE_NONE),
       d_downsampling_factor(1),
       d_ai_channels(ai_channels),
       d_ports(di_ports),
       d_channel_settings(),
       d_port_settings(),
       d_trigger_settings(),
       d_status(ai_channels),
       d_initialized(false),
       d_armed(false),
       d_auto_arm(auto_arm),
       d_trigger_once(false),
       d_was_triggered_once(false),
       d_timebase_published(false),
       d_data_rdy(false),
       d_trigger_state(0),
       d_read_idx(0),
       d_buffer_samples(0),
       d_errors(128)
   {
     d_ai_buffers = std::vector<std::vector<float>>(d_ai_channels);
     d_ai_error_buffers = std::vector<std::vector<float>>(d_ai_channels);

     if (di_ports) {
       d_port_buffers = std::vector<std::vector<uint8_t>>(di_ports);
     }

     assert(d_ai_channels < MAX_SUPPORTED_AI_CHANNELS);
     assert(d_ports < MAX_SUPPORTED_PORTS);
   }

   digitizer_block_impl::~digitizer_block_impl()
   {
   }

   /**********************************************************************
    * Helpers
    **********************************************************************/

   uint32_t
   digitizer_block_impl::get_pre_trigger_samples_with_downsampling() const
   {
     auto count = d_pre_samples;
     if (d_downsampling_mode != downsampling_mode_t::DOWNSAMPLING_MODE_NONE) {
       count = count / d_downsampling_factor;
     }
     return count;
   }

   uint32_t
   digitizer_block_impl::get_post_trigger_samples_with_downsampling() const
   {
     auto count = d_samples;
     if (d_downsampling_mode != downsampling_mode_t::DOWNSAMPLING_MODE_NONE) {
       count = count / d_downsampling_factor;
     }
     return count;
   }

   uint32_t
   digitizer_block_impl::get_block_size() const
   {
     return d_samples + d_pre_samples;
   }

   uint32_t
   digitizer_block_impl::get_block_size_with_downsampling() const
   {
       auto count = get_pre_trigger_samples_with_downsampling()
               + get_post_trigger_samples_with_downsampling();
       return count;
   }

   double
   digitizer_block_impl::get_timebase_with_downsampling() const
   {
     if (d_downsampling_mode == downsampling_mode_t::DOWNSAMPLING_MODE_NONE) {
       return 1.0 / d_actual_samp_rate;
     }
     else {
       return d_downsampling_factor / d_actual_samp_rate;
     }
   }

   int
   digitizer_block_impl::find_stream_trigger(int n)
   {
     assert(n > 0);
     assert(d_trigger_settings.is_enabled());

     int from_idx = d_read_idx + d_pre_samples;
     int to_idx = from_idx + n;

     if (d_trigger_settings.is_analog()) {
       const auto aichan = convert_to_aichan_idx(d_trigger_settings.source);
       float *buffer = &d_ai_buffers.at(aichan)[0];

       if (d_trigger_settings.direction == TRIGGER_DIRECTION_HIGH) {
         for(auto idx = from_idx; idx < to_idx; idx++) {
           if(buffer[idx] >= d_trigger_settings.threshold) {
             return idx;
           }
         }
       }
       else if (d_trigger_settings.direction == TRIGGER_DIRECTION_RISING) {

         auto band = d_channel_settings[aichan].actual_range / 100.0;
         auto lo = std::max(static_cast<float>(d_trigger_settings.threshold - band),
                 d_channel_settings[aichan].offset - d_channel_settings[aichan].actual_range);

         for(int idx = from_idx; idx < to_idx; idx++) {

           if(!d_trigger_state && buffer[idx] >= d_trigger_settings.threshold) {
             d_trigger_state = 1;
             return idx;
           }
           else if(d_trigger_state && buffer[idx] <= lo) {
             d_trigger_state = 0;
           }
         }
       }
       else if (d_trigger_settings.direction == TRIGGER_DIRECTION_FALLING) {

         auto band = d_channel_settings[aichan].actual_range / 100.0;
         auto hi = std::min(static_cast<float>(d_trigger_settings.threshold + band),
                 d_channel_settings[aichan].offset + d_channel_settings[aichan].actual_range);

         for(int idx = from_idx; idx < to_idx; idx++) {
           if(d_trigger_state && buffer[idx] <= d_trigger_settings.threshold) {
             d_trigger_state = 0;
             return idx;
           }
           else if(!d_trigger_state && buffer[idx] >= hi) {
             d_trigger_state = 1;
           }
         }
       }
       else if (d_trigger_settings.direction == TRIGGER_DIRECTION_LOW) {
         for(auto idx = from_idx; idx < to_idx; idx++) {
           if(buffer[idx] <= d_trigger_settings.threshold) {
             return idx;
           }
         }
       }
     }
     else {
       auto port = d_trigger_settings.pin_number / 8;
       auto pin = d_trigger_settings.pin_number % 8;
       auto mask = 1 << pin;

       uint8_t *buffer = &d_port_buffers.at(port)[0];

       if (d_trigger_settings.direction == TRIGGER_DIRECTION_HIGH) {
         for(auto idx = from_idx; idx < to_idx; idx++) {
           if(buffer[idx] & mask) {
             return idx;
           }
         }
       }
       else if (d_trigger_settings.direction == TRIGGER_DIRECTION_RISING) {

         for(int idx = from_idx; idx < to_idx; idx++) {
           if(!d_trigger_state && (buffer[idx] & mask)) {
             d_trigger_state = 1;
             return idx;
           }
           else if(d_trigger_state && !(buffer[idx] & mask)) {
             d_trigger_state = 0;
           }
         }
       }
       else if (d_trigger_settings.direction == TRIGGER_DIRECTION_FALLING) {

         for(int idx = from_idx; idx < to_idx; idx++) {
           if(d_trigger_state && !(buffer[idx] & mask)) {
             d_trigger_state = 0;
             return idx;
           }
           else if(!d_trigger_state && (buffer[idx] & mask)) {
             d_trigger_state = 1;
           }
         }
       }
       else if (d_trigger_settings.direction == TRIGGER_DIRECTION_LOW) {
         for(auto idx = from_idx; idx < to_idx; idx++) {
           if(!(buffer[idx] & mask)) {
             return idx;
           }
         }
       }
     }

     return -1;
   }

   void
   digitizer_block_impl::reset_stream_trigger()
   {
     assert(d_trigger_settings.is_enabled());

     if (d_trigger_settings.is_analog()) {
       auto aichan = convert_to_aichan_idx(d_trigger_settings.source);
       d_trigger_state = d_ai_buffers.at(aichan)[d_read_idx] >= d_trigger_settings.threshold;
     }
     else {
       auto port = d_trigger_settings.pin_number / 8;
       auto pin = d_trigger_settings.pin_number % 8;
       auto mask = 1 << pin;

       d_trigger_state = d_port_buffers.at(port)[d_read_idx] & mask;
     }
   }

   /**********************************************************************
    * Public API
    **********************************************************************/

   acquisition_mode_t
   digitizer_block_impl::get_acquisition_mode()
   {
     return d_acquisition_mode;
   }

   void
   digitizer_block_impl::set_samples(int samples, int pre_samples)
   {
     if (samples < 1) {
       throw std::invalid_argument("post-trigger samples can't be less than one");
     }

     if (pre_samples < 0) {
       throw std::invalid_argument("pre-trigger samples can't be less than zero");
     }

     d_samples = static_cast<uint32_t>(samples);
     d_pre_samples = static_cast<uint32_t>(pre_samples);
   }

   void
   digitizer_block_impl::set_samp_rate(double rate)
   {
     if (rate <= 0.0) {
       throw std::invalid_argument("sample rate should be greater than zero");
     }
     d_samp_rate = rate;
     d_actual_samp_rate = rate;
   }

   double
   digitizer_block_impl::get_samp_rate()
   {
     return d_actual_samp_rate;
   }

   void
   digitizer_block_impl::set_buffer_size(int buffer_size)
   {
     if (buffer_size < 0) {
       throw std::invalid_argument("buffer size can't be negative");
     }

     d_buffer_size = static_cast<uint32_t>(buffer_size);
   }

   void
   digitizer_block_impl::set_auto_arm(bool auto_arm)
   {
     d_auto_arm = auto_arm;
   }

   void
   digitizer_block_impl::set_trigger_once(bool once)
   {
     d_trigger_once = once;
   }

   // Poll rate is in seconds
   void
   digitizer_block_impl::set_streaming(double poll_rate)
   {
     if (poll_rate < 0.0) {
       throw std::invalid_argument("poll rate can't be negative");
     }

     d_acquisition_mode = acquisition_mode_t::STREAMING;
     d_poll_rate = poll_rate;
   }

   void
   digitizer_block_impl::set_rapid_block(int nr_captures)
   {
     if (nr_captures < 1) {
       throw std::invalid_argument("nr waveforms should be at least one");
     }

     d_acquisition_mode = acquisition_mode_t::RAPID_BLOCK;
     d_nr_captures = static_cast<uint32_t>(nr_captures);
   }

   void
   digitizer_block_impl::set_downsampling(downsampling_mode_t mode, int downsample_factor)
   {
     if (mode == downsampling_mode_t::DOWNSAMPLING_MODE_NONE) {
         downsample_factor = 1;
     }
     else if (downsample_factor < 2) {
       throw std::invalid_argument("downsampling factor should be at least 2");
     }

     d_downsampling_mode = mode;
     d_downsampling_factor = static_cast<uint32_t>(downsample_factor);
   }

   int
   digitizer_block_impl::convert_to_aichan_idx(const std::string &id) const
   {
     if (id.length() != 1) {
       throw std::invalid_argument("aichan id should be a single character: " + id);
     }

     int idx = std::toupper(id[0]) - 'A';
     if (idx < 0 || idx > MAX_SUPPORTED_AI_CHANNELS) {
       throw std::invalid_argument("invalid aichan id: " + id);
     }

     return idx;
   }

   void
   digitizer_block_impl::set_aichan(const std::string &id, bool enabled, double range, bool dc_coupling, double range_offset)
   {
     auto idx = convert_to_aichan_idx(id);
     d_channel_settings[idx].range = range;
     d_channel_settings[idx].offset = range_offset;
     d_channel_settings[idx].enabled = enabled;
     d_channel_settings[idx].dc_coupled = dc_coupling;
   }

   void
   digitizer_block_impl::set_aichan_range(const std::string &id, double range, double range_offset)
   {
     auto idx = convert_to_aichan_idx(id);
     d_channel_settings[idx].range = range;
     d_channel_settings[idx].offset = range_offset;
   }

   void
   digitizer_block_impl::set_aichan_trigger(const std::string &id, trigger_direction_t direction, double threshold)
   {
     convert_to_aichan_idx(id); // Just to verify id

     d_trigger_settings.source = id;
     d_trigger_settings.threshold = threshold;
     d_trigger_settings.direction = direction;
     d_trigger_settings.pin_number = 0; // not used
   }

   int
   digitizer_block_impl::convert_to_port_idx(const std::string &id) const
   {
     if (id.length() != 5) {
       throw std::invalid_argument("invalid port id: " + id + ", should be of the following format 'port<d>'");
     }

     int idx = boost::lexical_cast<int>(id[4]);
     if (idx < 0 || idx > MAX_SUPPORTED_PORTS) {
       throw std::invalid_argument("invalid port number: " + id);
     }

     return idx;
   }

   void
   digitizer_block_impl::set_diport(const std::string &id, bool enabled, double thresh_voltage)
   {
     auto port_number = convert_to_port_idx(id);

     d_port_settings[port_number].logic_level = thresh_voltage;
     d_port_settings[port_number].enabled = enabled;
   }

   void
   digitizer_block_impl::set_di_trigger(uint32_t pin, trigger_direction_t direction)
   {
     d_trigger_settings.source = TRIGGER_DIGITAL_SOURCE;
     d_trigger_settings.threshold = 0.0; // not used
     d_trigger_settings.direction = direction;
     d_trigger_settings.pin_number = pin;
   }

   void
   digitizer_block_impl::disable_triggers()
   {
     d_trigger_settings.source = TRIGGER_NONE_SOURCE;
   }

   void
   digitizer_block_impl::initialize()
   {
     if (d_initialized) {
       return;
     }

     auto ec = driver_initialize();
     if (ec) {
       d_errors.push(err::error_info_t{get_timestamp_utc_ns(), ec});
       throw std::runtime_error("initialize failed: " + to_string(ec));
     }

     d_initialized = true;
   }

   void
   digitizer_block_impl::configure()
   {
     if (!d_initialized) {
       throw std::runtime_error("initialize first");;
     }

     if (d_armed) {
       throw std::runtime_error("disarm first");
     }

     auto ec = driver_configure();
     if (ec) {
       d_errors.push(err::error_info_t{get_timestamp_utc_ns(), ec});
       throw std::runtime_error("configure failed: " + to_string(ec));
     }

     if (d_acquisition_mode == acquisition_mode_t::STREAMING) {
       d_sstate.initialize(d_ai_channels, d_ports, d_buffer_size);
       initialize_stream_buffers();
     }
   }

   void
   digitizer_block_impl::arm()
   {
     if (d_armed) {
       return;
     }

     auto ec = driver_arm();
     if (ec) {
       d_errors.push(err::error_info_t{get_timestamp_utc_ns(), ec});
       throw std::runtime_error("arm failed: " + to_string(ec));
     }

     d_armed = true;
     d_timebase_published = false;
   }

   bool
   digitizer_block_impl::is_armed()
   {
     return d_armed;
   }

   void
   digitizer_block_impl::disarm()
   {
     if (!d_armed) {
       return;
     }

     auto ec = driver_disarm();
     if (ec) {
       d_errors.push(err::error_info_t{get_timestamp_utc_ns(), ec});
       GR_LOG_WARN(d_logger, "disarm failed: " + to_string(ec));
     }

     d_armed = false;
   }

   void
   digitizer_block_impl::close()
   {
     // Interrupt waiting function (workaround). From the scheduler point of view this is not
     // needed because it makes sure that the worker thread gets interrupted before the stop
     // method is called. But we have this in place in order to allow for manual intervention.
     /*{
       boost::mutex::scoped_lock lock(d_mutex);
       d_data_rdy = true;
       d_data_rdy_errc = digitizer_block_errc::Stopped;
     }

     d_data_rdy_cv.notify_all();*/
     notify_data_ready(digitizer_block_errc::Stopped);
     if (!d_initialized) {
       return;
     }

     disarm();

     auto ec = driver_close();
     if (ec) {
       d_errors.push(err::error_info_t{get_timestamp_utc_ns(), ec});
       GR_LOG_WARN(d_logger, "close failed: " + to_string(ec));
     }

     d_initialized = false;
   }

   std::vector<err::error_info_t>
   digitizer_block_impl::get_errors()
   {
     return d_errors.get();
   }

   bool
   digitizer_block_impl::start()
   {
     initialize();
     configure();

     // Needed in case start/run is called multiple times without destructing the flowgraph
     d_was_triggered_once = false;
     d_data_rdy_errc = std::error_code {};
     d_data_rdy = false;

     if(d_auto_arm && d_acquisition_mode == acquisition_mode_t::STREAMING) {
       while(true) {
         try {
           arm();
           break;
         }
         catch (...) { return false; } //retry
       }
     }

     return true;
   }

   bool
   digitizer_block_impl::stop()
   {
     close();
     return true;
   }

   /**********************************************************************
    * Driver interface
    **********************************************************************/

   void
   digitizer_block_impl::notify_data_ready(std::error_code ec)
   {
     if(ec) {
       d_errors.push(err::error_info_t{get_timestamp_utc_ns(), ec});
     }

     {
       boost::mutex::scoped_lock lock(d_mutex);
       d_data_rdy = true;
       d_data_rdy_errc = ec;
     }

     d_data_rdy_cv.notify_one();
   }

   std::error_code
   digitizer_block_impl::wait_data_ready()
   {
     boost::mutex::scoped_lock lock(d_mutex);

     d_data_rdy_cv.wait(lock, [this] { return d_data_rdy; });
     return d_data_rdy_errc;
   }

   void digitizer_block_impl::clear_data_ready()
   {
     boost::mutex::scoped_lock lock(d_mutex);

     d_data_rdy = false;
     d_data_rdy_errc = std::error_code {};
   }


   /**********************************************************************
    * GR worker functions
    **********************************************************************/

   int
   digitizer_block_impl::work_rapid_block(int noutput_items, gr_vector_void_star &output_items)
   {
     if (d_bstate.state == rapid_block_state_t::WAITING) {

       if (d_trigger_once &&  d_was_triggered_once) {
         return -1;
       }

       if (d_auto_arm) {
         disarm();
         while(true) {
           try {
             arm();
             break;
           }
           catch (...) {
             return -1;
           }
         }
       }

       // Wait conditional variable, when waken clear it
       auto ec = wait_data_ready();
       clear_data_ready();

       // Stop requested
       if (ec == digitizer_block_errc::Stopped) {
         GR_LOG_INFO(d_logger, "stop requested");
         return -1;
       }
       else if (ec) {
         GR_LOG_ERROR(d_logger, "error occurred while waiting for data: " + to_string(ec));
         return 0;
       }

       // we assume all the blocks are ready
       d_bstate.initialize(d_nr_captures);
     }

     if (d_bstate.state == rapid_block_state_t::READING_PART1) {

       // If d_trigger_once is true we will signal all done in the next iteration
       // with the block state set to WAITING
       d_was_triggered_once = true;

       auto samples_to_fetch = get_block_size();
       auto downsampled_samples = get_block_size_with_downsampling();

       // Instruct the driver to prefetch samples. Drivers might choose to ignore this call
       auto ec = driver_prefetch_block(samples_to_fetch, d_bstate.waveform_idx);
       if (ec) {
         d_errors.push(err::error_info_t{get_timestamp_utc_ns(), ec});
         return -1;
       }

       // Initiate state machine for the current waveform. Note state machine track
       // and adjust the waveform index.
       d_bstate.set_waveform_params(0, downsampled_samples);

       // We are good to read first batch of samples
       noutput_items = std::min(noutput_items, d_bstate.samples_left);

       ec = driver_get_rapid_block_data(d_bstate.offset,
               noutput_items, d_bstate.waveform_idx, output_items, d_status);
       if (ec) {
         d_errors.push(err::error_info_t{get_timestamp_utc_ns(), ec});
         return -1;
       }

       // Attach trigger info to value outputs and to all ports
       auto vec_idx = 0;
       for (auto i = 0; i < d_ai_channels
             && vec_idx < (int)output_items.size(); i++, vec_idx+=2) {
         if (!d_channel_settings[i].enabled) {
           continue;
         }

         auto trigger_tag = make_trigger_tag(
                 get_pre_trigger_samples_with_downsampling(),
                 get_post_trigger_samples_with_downsampling(),
                 d_status[i],
                 1.0 / d_actual_samp_rate,
                 get_timestamp_utc_ns());
         trigger_tag.offset = nitems_written(0);

         add_item_tag(vec_idx, trigger_tag);
       }

       auto trigger_tag = make_trigger_tag(
             get_pre_trigger_samples_with_downsampling(),
             get_post_trigger_samples_with_downsampling(),
             0,
             1.0 / d_actual_samp_rate,
             get_timestamp_utc_ns());
       trigger_tag.offset = nitems_written(0);

       for (auto i = 0; i < d_ports
             && vec_idx < (int)output_items.size(); i++, vec_idx++) {
         if (!d_port_settings[i].enabled) {
           continue;
         }

         add_item_tag(vec_idx, trigger_tag);
       }

       // update state
       d_bstate.update_state(noutput_items);

       return noutput_items;
     }
     else if (d_bstate.state == rapid_block_state_t::READING_THE_REST) {

       noutput_items = std::min(noutput_items, d_bstate.samples_left);

       auto ec = driver_get_rapid_block_data(d_bstate.offset, noutput_items,
               d_bstate.waveform_idx, output_items, d_status);
       if (ec) {
         d_errors.push(err::error_info_t{get_timestamp_utc_ns(), ec});
         return -1;
       }

       // update state
       d_bstate.update_state(noutput_items);

       return noutput_items;
     }

     return -1;
   }

   void
   digitizer_block_impl::initialize_stream_buffers()
   {
     for (auto i = 0; i < d_ai_channels; i++) {
       if(d_channel_settings[i].enabled) {
         d_ai_buffers.at(i).reserve(d_buffer_size);
         d_ai_error_buffers.at(i).reserve(d_buffer_size);
       }
     }

     for (auto i = 0; i < d_ports; i++) {
       if(d_port_settings[i].enabled) {
         d_port_buffers.at(i).reserve(d_buffer_size);
       }
     }

     d_status = std::vector<uint32_t>(d_ai_channels);
     d_status_pre = std::vector<uint32_t>(d_ai_channels);

     for (auto &e : d_status) { e = 0; }
     for (auto &e : d_status_pre) { e = 0; }

     d_read_idx = 0;
     d_buffer_samples = 0;
   }

   /*!
    * \brief Shift all data to the left (well individual vectors).
    */
   void
   digitizer_block_impl::shift_stream_data(uint32_t samples_to_keep)
   {
     assert(d_buffer_samples > samples_to_keep);

     auto src_idx = d_buffer_samples - samples_to_keep;

     for (auto i = 0; i < d_ai_channels; i++) {
       if(d_channel_settings[i].enabled) {
         auto & vec = d_ai_buffers.at(i);
         std::memmove(&vec[0], &vec[src_idx], samples_to_keep * sizeof(float));

         auto & err_vec = d_ai_error_buffers.at(i);
         std::memmove(&err_vec[0], &err_vec[src_idx], samples_to_keep * sizeof(float));
       }
     }

     for (auto i = 0; i < d_ports; i++) {
       if(d_port_settings[i].enabled) {
         auto & vec = d_port_buffers.at(i);
         std::memmove(&vec[0], &vec[src_idx], samples_to_keep);
       }
     }

     // Shift status as well
     assert(d_status.size() == d_status_pre.size());

     for (size_t i = 0; i < d_status.size(); i++) {
       d_status_pre[i] = d_status[i];
       d_status[i] = 0;
     }

     d_buffer_samples = samples_to_keep;
   }


   /*!
    * Logic and assumptions.
    *
    * The state of this function is maintained in a struct called stream_state_t. Before this
    * function is called for the first time the d_sstate struct needs to be initialize (buffers
    * allocated, read-, trigger- and other indexes set to 0, etc).
    *
    * In regards to the buffering strategy, vectors are used for simplicity. More performant
    * solution would be to use circular buffers but this would make data transfer more complex.
    * Currently we simply shift samples to the left (explanation is given below).
    *
    * What adds the most complexity to this function is trigger handling logic. In order to be
    * able to attach a trigger tag, note trigger tag is attached to the first pre-trigger sample
    * and not to the sample where a trigger is detected. For this reason d_pre_samples number of
    * samples needs to be kept in the local buffer.
    */
   int
   digitizer_block_impl::work_stream(int noutput_items, gr_vector_void_star &output_items)
   {
     while (d_buffer_samples <= d_pre_samples) {

       if(driver_get_streaming_samples() == 0) {
         auto ec = driver_poll();
         if (ec) {
           d_errors.push(err::error_info_t{get_timestamp_utc_ns(), ec});
           return -1;
         }

         std::this_thread::sleep_for(std::chrono::microseconds((long)(d_poll_rate * 1000000)));
       }

       gr_vector_void_star ai_buffers(d_ai_channels);
       gr_vector_void_star ai_error_buffers(d_ai_channels);

       for (auto i = 0; i < d_ai_channels; i++) {
         if (d_channel_settings[i].enabled) {
           ai_buffers[i] = &d_ai_buffers.at(i)[d_buffer_samples];
           ai_error_buffers[i] = &d_ai_error_buffers.at(i)[d_buffer_samples];
         }
         else {
           ai_buffers[i] = nullptr;
           ai_error_buffers[i] = nullptr;
         }
       }

       gr_vector_void_star port_buffers(d_ports);

       for (auto i = 0; i < d_ports; i++) {
         if (d_port_settings[i].enabled) {
           port_buffers[i] = &d_port_buffers.at(i)[d_buffer_samples];
         }
         else {
           port_buffers[i] = nullptr;
         }
       }

       size_t actual;
       auto ec = driver_get_streaming_data(ai_buffers,
             ai_error_buffers,
             port_buffers,
             d_status,
             d_buffer_size - d_buffer_samples,
             actual);
       if (ec) {
         d_errors.push(err::error_info_t{get_timestamp_utc_ns(), ec});
         return -1; // stop flowgraph
       }

       d_buffer_samples += actual;
     }

     assert(d_buffer_samples > d_pre_samples);

     // Acquisition tag is attached whenever new data is polled from the driver (read_idx == 0).
     if (d_read_idx == 0) {
       acq_info_t tag_info{};

       tag_info.timestamp = get_timestamp_utc_ns();
       tag_info.timebase = 1.0 / d_actual_samp_rate;
       tag_info.user_delay = 0.0;
       tag_info.actual_delay = 0.0;
       tag_info.samples = (uint32_t)d_buffer_samples - d_pre_samples;
       tag_info.status = 0;
       tag_info.offset = nitems_written(0);
       tag_info.triggered_data = false;

       for (int i = 0; i < d_ai_channels && (i * 2) < (int)output_items.size(); i++) {
         tag_info.status = d_status.at(i);
         auto tag = make_acq_info_tag(tag_info);
         add_item_tag(i * 2, tag);
       }
     }

     noutput_items = std::min(noutput_items, (int)(d_buffer_samples - d_pre_samples - d_read_idx));

     assert (noutput_items > 0);

     // This simplifies the below code, that is the d_sstate.trigger_samples tells us how many
     // samples there is left to output before searching for another trigger (edge).
     if (d_trigger_settings.is_enabled() && d_sstate.trigger_samples > 0) {
       noutput_items = std::min(noutput_items, d_sstate.trigger_samples);
     }
     // Ok we need to search for a trigger
     else if (d_trigger_settings.is_enabled() && d_sstate.trigger_samples == 0) {

       auto trig_idx = find_stream_trigger(noutput_items);

       if (trig_idx >= 0) {

         assert(trig_idx >= d_read_idx);
         assert(trig_idx - d_read_idx >= (int)d_pre_samples);

         // samples until the end-of-trigger, note trig_idx points to the item
         // that is located after pre-trigger samples
         d_sstate.trigger_samples = d_samples + (trig_idx - d_read_idx);

         auto offset = nitems_written(0)
               + static_cast<uint64_t>(trig_idx - d_read_idx)
               - static_cast<uint64_t>(d_pre_samples);

         // attach trigger tags (well acq info) to all channels (ignore error outputs)
         auto vec_idx = 0;

         for (auto i = 0; i < d_ai_channels
               && vec_idx < (int)output_items.size(); i++, vec_idx += 2) {
           if (!d_channel_settings[i].enabled) {
             continue;
           }

           auto trigger_tag = make_trigger_tag(
                 get_pre_trigger_samples_with_downsampling(),
                 get_post_trigger_samples_with_downsampling(),
                 d_status[i],
                 1.0 / d_actual_samp_rate,
                 get_timestamp_utc_ns());
           trigger_tag.offset = offset;

           add_item_tag(vec_idx, trigger_tag);
         }

         auto trigger_tag = make_trigger_tag(
               get_pre_trigger_samples_with_downsampling(),
               get_post_trigger_samples_with_downsampling(),
               0,
               1.0 / d_actual_samp_rate,
               get_timestamp_utc_ns());
         trigger_tag.offset = offset;

         for (auto i = 0; i < d_ports
                && vec_idx < (int)output_items.size(); i++, vec_idx++) {
           if (!d_port_settings[i].enabled) {
             continue;
           }

           add_item_tag(vec_idx, trigger_tag);
         }
       }
     }

     auto vec_idx = 0;
     for (auto i = 0; i < d_ai_channels && vec_idx < (int)output_items.size(); i++, vec_idx += 2) {

       if (!d_channel_settings[i].enabled) {
         continue;
       }

       memcpy(output_items.at(vec_idx),
               &d_ai_buffers.at(i)[d_read_idx],
               noutput_items * sizeof(float));
       memcpy(output_items.at(vec_idx + 1),
               &d_ai_error_buffers.at(i)[d_read_idx],
               noutput_items * sizeof(float));
     }

     for (auto i = 0; i < d_ports && vec_idx < (int)output_items.size(); i++, vec_idx++) {

       if (!d_port_settings[i].enabled) {
         continue;
       }

       memcpy(output_items.at(vec_idx),
              &d_port_buffers.at(i)[d_read_idx],
              noutput_items * sizeof(uint8_t));
     }

     // Update read index
     d_read_idx += noutput_items;

     if (d_read_idx == (int)(d_buffer_samples - d_pre_samples)) {
       // buffer is exhausted, shift samples left in the buffer to the left
       shift_stream_data(d_pre_samples);
       d_read_idx = 0;
     }

     // Decrement number of trigger samples we still need to output
     if (d_sstate.trigger_samples > 0) {

       if (d_sstate.trigger_samples > noutput_items) {
         d_sstate.trigger_samples -= noutput_items;
       }
       else {
         d_sstate.trigger_samples = 0;
         reset_stream_trigger();
       }
     }

     return noutput_items;
   }

   int
   digitizer_block_impl::work(int noutput_items,
       gr_vector_const_void_star &input_items,
       gr_vector_void_star &output_items)
   {
     int retval = -1;

     if(d_acquisition_mode == acquisition_mode_t::STREAMING) {
       retval = work_stream(noutput_items, output_items);
     }
     else if(d_acquisition_mode == acquisition_mode_t::RAPID_BLOCK) {
       retval = work_rapid_block(noutput_items, output_items);
     }

     if ((retval > 0) && !d_timebase_published) {
       auto timebase_tag = make_timebase_info_tag(get_timebase_with_downsampling());
       timebase_tag.offset = nitems_written(0);

       for (gr_vector_void_star::size_type i = 0; i < output_items.size(); i++) {
         add_item_tag(i, timebase_tag);
       }

       d_timebase_published = true;
     }

     return retval;
   }

  } /* namespace digitizers */
} /* namespace gr */

