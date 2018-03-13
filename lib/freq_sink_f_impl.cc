/* -*- c++ -*- */
/* 
 * Copyright 2018 <+YOU OR YOUR COMPANY+>.
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
#include "freq_sink_f_impl.h"

namespace gr {
  namespace digitizers {

    freq_sink_f::sptr
    freq_sink_f::make(std::string name, std::string unit, int nbins,
            int nmeasurements, freq_sink_mode_t mode)
    {
      return gnuradio::get_initial_sptr
        (new freq_sink_f_impl(name, unit, nbins, nmeasurements, mode));
    }

    /*
     * The private constructor
     */
    freq_sink_f_impl::freq_sink_f_impl(std::string name, std::string unit,
            int nbins, int nmeasurements, freq_sink_mode_t mode)
      : gr::sync_block("freq_sink_f",
              gr::io_signature::makev(3, 3,
                  std::vector<int>(
                  {
                    nbins * static_cast<int>(sizeof(float)),
                    nbins * static_cast<int>(sizeof(float)),
                    nbins * static_cast<int>(sizeof(float))
                  })),
              gr::io_signature::make(0, 0, 0)),
        d_mode(mode),
        d_metadata({unit, name}),
        d_callback(nullptr),
        d_user_data(nullptr),
        d_nbins(nbins),
        d_size(0),
        d_nmeasurements(nmeasurements),
        d_snapshot_points(),
        d_waiting_readout(false)
    {
      // reserve storage for better performance
      size_t n = nbins * nmeasurements;

      d_buffer_measurment.reserve(n);
      d_buffer_freq.reserve(n);
      d_buffer_magnitude.reserve(n);
      d_buffer_phase.reserve(n);

      d_acq_info.reserve(8);
    }

    /*
     * Our virtual destructor.
     */
    freq_sink_f_impl::~freq_sink_f_impl()
    {
    }

    int
    freq_sink_f_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      if (d_mode == FREQ_SINK_MODE_SNAPSHOT) {
        return work_snapshot(noutput_items, input_items, output_items);
      }
      else {
        return work_stream(noutput_items, input_items, output_items);
      }
    }

    int
    freq_sink_f_impl::work_stream(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const float *magnitude = (const float *) input_items[0];
      const float *phase = (const float *) input_items[1];
      const float *freqs = (const float *) input_items[2];

      // noutput_itmes == number of input vectors
      {
        boost::mutex::scoped_lock lock(d_mutex);

        assert(d_size <= d_nmeasurements);

        if (d_waiting_readout) {
          return 0;
        }

        if (d_size == 0) {
          assert(d_buffer_magnitude.size() == 0);
          assert(d_buffer_phase.size() == 0);
          assert(d_buffer_freq.size() == 0);

          // Starting fresh... Only keep the last acq_info tag
          if (d_acq_info.size() > 1) {
            d_acq_info.erase(d_acq_info.begin(), d_acq_info.end() - 1);
          }
        }

        // Take only as much items we can store in the buffer
        noutput_items = std::min(noutput_items, d_nmeasurements - d_size);

        for (int i = 0; i < noutput_items; i++) {
          // handle acq_info tags
          std::vector<gr::tag_t> tags;
          get_tags_in_range(tags, 0, nitems_read(0) + i, nitems_read(0) + i + 1,
                  pmt::string_to_symbol("acq_info"));

          spectra_measurement_t meas {};
          meas.number_of_bins = d_nbins;
          meas.timestamp = -1;
          meas.trigger_timestamp = -1;

          if(!tags.empty()) {
            if (tags.size() > 1) {
              GR_LOG_WARN(d_logger, "multiple acq_info tags not supported, "
                      "only the first one will be used...");
            }
            auto acq_info = decode_acq_info_tag(tags.at(0));

            meas.trigger_timestamp = acq_info.trigger_timestamp;
            meas.status = acq_info.status;
            meas.timestamp = acq_info.timestamp;

            d_acq_info.clear();
            d_acq_info.push_back(acq_info);
          }
          else if (tags.empty() && !d_acq_info.empty()) {
            auto acq_info = d_acq_info.back();

            meas.trigger_timestamp = acq_info.trigger_timestamp;
            meas.status = acq_info.status;
            meas.timestamp = acq_info.timestamp;

            // correct timestamp
            auto delta = (nitems_read(0) + i) - acq_info.offset;
            auto delta_ns = ((double)delta * (double)d_nbins * acq_info.timebase) * 1000000000.0;
            meas.timestamp += static_cast<int64_t>(delta_ns);
          }

          d_buffer_measurment.push_back(meas);
        }

        // copy values and errors
        auto nsamples = noutput_items * d_nbins;
        d_buffer_magnitude.insert(d_buffer_magnitude.end(), magnitude, magnitude + nsamples);
        d_buffer_phase.insert(d_buffer_phase.end(), phase, phase + nsamples);
        d_buffer_freq.insert(d_buffer_freq.end(), freqs, freqs + nsamples);

        d_size += noutput_items;

        if (d_size == d_nmeasurements)
        {
          d_waiting_readout = true;
        }

        // Release lock before we call a callback
      }

      if (d_size == d_nmeasurements && d_callback != nullptr) {
        data_available_event_t args;
        args.trigger_timestamp = d_buffer_measurment.at(0).trigger_timestamp;
        args.signal_name = d_metadata.name;
        d_callback(&args, d_user_data);
      }

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

    int
    freq_sink_f_impl::work_snapshot(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const float *magnitude = (const float *) input_items[0];
      const float *phase = (const float *) input_items[1];
      const float *freqs = (const float *) input_items[2];

      // noutput_itmes == number of input vectors
      {
        boost::mutex::scoped_lock lock(d_mutex);

        assert(d_size <= d_nmeasurements);

        if (d_waiting_readout) {
          return 0;
        }

        if (d_size == 0) {
          assert(d_buffer_magnitude.size() == 0);
          assert(d_buffer_phase.size() == 0);
          assert(d_buffer_freq.size() == 0);

          // Starting fresh... Only keep the last acq_info tag
          if (d_acq_info.size() > 1) {
            d_acq_info.erase(d_acq_info.begin(), d_acq_info.end() - 1);
          }
        }

        // In snapshot mode we only process a single vector at a time
        noutput_items = 1;

        std::vector<gr::tag_t> tags;
        get_tags_in_range(tags, 0, nitems_read(0), nitems_read(0)+ 1,
                pmt::string_to_symbol("acq_info"));

        if (d_acq_info.empty() && tags.empty()) {
          // no timing info available, not much we can do
          return noutput_items;
        }

        acq_info_t acq_info;
        int64_t timestamp;

        if (!tags.empty()) {
          // use the latest timing info
          if (tags.size() > 1) {
            GR_LOG_WARN(d_logger, "multiple acq_info tags not supported, "
                      "only the first one will be used...");
          }
          acq_info = decode_acq_info_tag(tags.at(0));
          timestamp = acq_info.timestamp;

          d_acq_info.clear();
          d_acq_info.push_back(acq_info);
        }
        else {
          acq_info = d_acq_info.back();

          if (acq_info.timestamp != -1) {
            auto delta = static_cast<double>(nitems_read(0) - acq_info.offset);
            timestamp = acq_info.timestamp + static_cast<int64_t>(
                  delta * d_nbins * acq_info.timebase * 1000000000.0);
          }
          else {
            timestamp = -1;
          }
        }

        if (timestamp == -1) {
          // no timing info available, not much we can do
          return noutput_items;
        }

        // timestamp of the last sample + 1, one sample is added in order to avoid
        // gaps between vectors
        auto ntimestamp = timestamp + static_cast<int64_t>(
                (d_nbins + 1) * acq_info.timebase * 1000000000.0);

        auto it = std::find_if(d_snapshot_points.begin(), d_snapshot_points.end(),
                [timestamp, ntimestamp, &acq_info] (int64_t p)
        {
          auto abs_p_timestamp = acq_info.last_beam_in_timestamp + p;
          return abs_p_timestamp >= timestamp && abs_p_timestamp < ntimestamp;
        });

        if (it == d_snapshot_points.end()) {
            return noutput_items;
        }

        // add vector into a buffer
        spectra_measurement_t meas {};
        meas.number_of_bins = d_nbins;
        meas.timestamp = timestamp;
        meas.trigger_timestamp = acq_info.trigger_timestamp;
        meas.status = acq_info.status;

        // buffer values
        d_buffer_measurment.push_back(meas);
        d_buffer_magnitude.insert(d_buffer_magnitude.end(), magnitude, magnitude + d_nbins);
        d_buffer_phase.insert(d_buffer_phase.end(), phase, phase + d_nbins);
        d_buffer_freq.insert(d_buffer_freq.end(), freqs, freqs + d_nbins);

        d_size += noutput_items;

        if (d_size == d_nmeasurements)
        {
          d_waiting_readout = true;
        }

        // Release lock before we call a callback
      }

      if (d_size == d_nmeasurements && d_callback != nullptr) {
        data_available_event_t args;
        args.trigger_timestamp = d_buffer_measurment.at(0).trigger_timestamp;
        args.signal_name = d_metadata.name;
        d_callback(&args, d_user_data);
      }

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

    void
    freq_sink_f_impl::set_snapshot_points(const std::vector<float> &points)
    {
      boost::mutex::scoped_lock lock(d_mutex);

      d_snapshot_points.clear();

      for (auto p: points) {
        d_snapshot_points.push_back(static_cast<int64_t>(p * 1000000000.0));
      }
    }

    signal_metadata_t
    freq_sink_f_impl::get_metadata()
    {
      return d_metadata;
    }

    size_t
    freq_sink_f_impl::get_measurements(size_t nr_measurements,
       std::vector<spectra_measurement_t> &measurements,
       std::vector<float> &frequency,
       std::vector<float> &magnitude,
       std::vector<float> &phase)
    {
      boost::mutex::scoped_lock lock(d_mutex);

      if (!d_size) {
        d_waiting_readout = false;
        return 0;
      }

      nr_measurements = std::min(nr_measurements, static_cast<size_t>(d_size));

      // copy over data
      measurements.assign(d_buffer_measurment.begin(), d_buffer_measurment.end());
      frequency.assign(d_buffer_freq.begin(), d_buffer_freq.end());
      magnitude.assign(d_buffer_magnitude.begin(), d_buffer_magnitude.end());
      phase.assign(d_buffer_phase.begin(), d_buffer_phase.end());

      // If user does not read-out all samples at once then the data is lost
      clear_buffers();
      d_waiting_readout = false;

      return nr_measurements;
    }

    void
    freq_sink_f_impl::set_measurements_available_callback(data_available_cb_t callback, void *ptr)
    {
      d_callback = callback;
      d_user_data = ptr;
    }

    int
    freq_sink_f_impl::get_nbins()
    {
      return d_nbins;
    }

    int
    freq_sink_f_impl::get_nmeasurements()
    {
      return d_nmeasurements;
    }

    freq_sink_mode_t
    freq_sink_f_impl::get_mode()
    {
      return d_mode;
    }

    void
    freq_sink_f_impl::notify_data_ready()
    {
      data_available_event_t args;

      {
        boost::mutex::scoped_lock lock(d_mutex);
        if (!d_size) {
          return;
        }
        else {
          d_waiting_readout = true;
          args.signal_name = d_metadata.name;
          args.trigger_timestamp = d_buffer_measurment.at(0).trigger_timestamp;
        }
      }

      if (d_callback != nullptr) {
        d_callback(&args, d_user_data);
      }
    }

    bool
    freq_sink_f_impl::start()
    {
      d_waiting_readout = false;

      d_acq_info.clear();
      clear_buffers();

      return true;
    }

    void
    freq_sink_f_impl::clear_buffers()
    {
      d_buffer_measurment.clear();
      d_buffer_freq.clear();
      d_buffer_magnitude.clear();
      d_buffer_phase.clear();

      d_size = 0;
    }

  } /* namespace digitizers */
} /* namespace gr */

