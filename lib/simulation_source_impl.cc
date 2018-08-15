/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "simulation_source_impl.h"
#include <future>
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
              gr::io_signature::makev(1, 5, std::vector<int>({4, 4, 4, 4, 1}))),
      digitizer_block_impl(2, 1, false)
    {
      d_ranges.push_back(range_t(20));

      // Enable all channels and ports
      set_aichan("A", true, 20.0, false);
      set_aichan("B", true, 20.0, false);
      set_diport("port0", true, 0.7);
    }

    /*
     * Our virtual destructor.
     */
    simulation_source_impl::~simulation_source_impl()
    {
    }

    void
    simulation_source_impl::set_data(const std::vector<float> &ch_a_vec, const std::vector<float> &ch_b_vec,
            const std::vector<uint8_t> &port_vec)
    {
      if (d_buffer_size != (uint32_t)ch_a_vec.size())
      {
        std::ostringstream message;
        message << "Exception in " << __FILE__ << ":" << __LINE__ << ": invalid data size provided:" << ch_a_vec.size() << ", expected: " <<  d_buffer_size;
        throw std::runtime_error(message.str());
      }

      if(ch_a_vec.size() != ch_b_vec.size() || ch_a_vec.size() != port_vec.size())
      {
          std::ostringstream message;
          message << "Exception in " << __FILE__ << ":" << __LINE__ << ": all vectors should be of the same size";
          throw std::runtime_error(message.str());
      }

      d_ch_a_data = ch_a_vec;
      d_ch_b_data = ch_b_vec;
      d_port_data = port_vec;
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

    std::string
    simulation_source_impl::get_driver_version()
    {
      return "simulation";
    }

    std::string
    simulation_source_impl::get_hardware_version()
    {
      return "simulation";
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
      if (d_acquisition_mode == acquisition_mode_t::RAPID_BLOCK) {
        // rapid block data gets available after one second
        std::async(std::launch::async, [this] () {
          boost::this_thread::sleep_for(boost::chrono::seconds{1});
          notify_data_ready(std::error_code{});
        });
      }

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
      if (offset + length > d_ch_a_data.size())
      {
        std::ostringstream message;
        message << "Exception in " << __FILE__ << ":" << __LINE__ << ": cannot fetch rapid block data, out of bounds";
        throw std::runtime_error(message.str());
      }

      if (arrays.size() != 5)
      {
        std::ostringstream message;
        message << "Exception in " << __FILE__ << ":" << __LINE__ << ": all channels should be passed in";
        throw std::runtime_error(message.str());
      }

      // Update status first
      for (auto &s : status) {
        s = 0;
      };

      // Copy over the data
      float *val_a = static_cast<float *>(arrays[0]);
      float *err_a = static_cast<float *>(arrays[1]);
      float *val_b = static_cast<float *>(arrays[2]);
      float *err_b = static_cast<float *>(arrays[3]);
      uint8_t *port  = static_cast<uint8_t *>(arrays[4]);

      for (size_t i = 0; i < length; i++) {
        val_a[i] = d_ch_a_data[offset + i];
        err_a[i] = 0.005;

        val_b[i] = d_ch_b_data[offset + i];
        err_b[i] = 0.005;

        port[i] = d_port_data[offset + i];
      }

      return std::error_code {};
    }

    std::error_code
    simulation_source_impl::driver_poll()
    {
      for (size_t i = 0; i < 1000; i++) {
        if (fill_in_data_chunk() == false)
            break;
      }

      return std::error_code {};
    }

    bool
    simulation_source_impl::fill_in_data_chunk()
    {
      auto buffer_size_channel_bytes = (d_buffer_size * sizeof(float));

      auto buffer_size_bytes =
              (buffer_size_channel_bytes * 2 * 2)  // 2 channels, floats, errors & values
            + (d_buffer_size);

      // resize and clear tmp buffer (errors are all zero)
      auto buffer = d_app_buffer.get_free_data_chunk();
      if (buffer == nullptr) {
        return false;
      }

      assert(buffer->d_data.size() == buffer_size_bytes);

      // just in case
      d_ch_a_data.resize(d_buffer_size);
      d_ch_b_data.resize(d_buffer_size);
      d_port_data.resize(d_buffer_size);

      // fill up tmp buffer
      memcpy(&buffer->d_data[buffer_size_channel_bytes * 0], &d_ch_a_data[0], buffer_size_channel_bytes);
      memcpy(&buffer->d_data[buffer_size_channel_bytes * 2], &d_ch_b_data[0], buffer_size_channel_bytes);
      memcpy(&buffer->d_data[buffer_size_channel_bytes * 4], &d_port_data[0], d_buffer_size);

      // error band
      float *err_a = reinterpret_cast<float *>(&buffer->d_data[buffer_size_channel_bytes * 1]);
      float *err_b = reinterpret_cast<float *>(&buffer->d_data[buffer_size_channel_bytes * 3]);
      for (uint32_t i = 0; i < d_buffer_size; i++) {
        err_a[i] = 0.005;
        err_b[i] = 0.005;
      }

      buffer->d_local_timestamp = get_timestamp_utc_ns();
      buffer->d_status = std::vector<uint32_t> { 0, 0 };

      d_app_buffer.add_full_data_chunk(buffer);

      return true;
    }


  } /* namespace digitizers */
} /* namespace gr */

