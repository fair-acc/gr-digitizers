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

#ifndef INCLUDED_DIGITIZERS_FREQ_SINK_F_IMPL_H
#define INCLUDED_DIGITIZERS_FREQ_SINK_F_IMPL_H

#include <digitizers/freq_sink_f.h>
#include <digitizers/tags.h>

#include <boost/thread/mutex.hpp>
#include <vector>

namespace gr {
  namespace digitizers {

    class freq_sink_f_impl : public freq_sink_f
    {
     private:
      freq_sink_mode_t d_mode;
      signal_metadata_t d_metadata;

      // callback stuff
      data_available_cb_t d_callback;
      void *d_user_data;

      // buffers
      std::vector<spectra_measurement_t> d_buffer_measurment;
      std::vector<float> d_buffer_freq;
      std::vector<float> d_buffer_magnitude;
      std::vector<float> d_buffer_phase;
      std::vector<acq_info_t> d_acq_info;

      int d_nbins;
      int d_size;    // measurements currently in the buffer
      int d_nmeasurements;

      std::vector<int64_t> d_snapshot_points; // relative in nanoseconds

      boost::mutex d_mutex;
      bool d_waiting_readout;

     public:
      freq_sink_f_impl(std::string name, std::string unit, int nbins,
         int nmeasurements, freq_sink_mode_t mode);
      ~freq_sink_f_impl();

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items) override;



      void set_snapshot_points(const std::vector<float> &points) override;

      signal_metadata_t get_metadata() override;

      size_t get_measurements(size_t nr_measurements,
         std::vector<spectra_measurement_t> &measurements,
         std::vector<float> &frequency,
         std::vector<float> &magnitude,
         std::vector<float> &phase) override;

      void set_measurements_available_callback(data_available_cb_t callback, void *ptr) override;

      int get_nbins() override;
      int get_nmeasurements() override;

      freq_sink_mode_t get_mode() override;

      void notify_data_ready() override;

      bool start() override;

     private:
      void clear_buffers();

      int work_snapshot(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);

      // Where all the action really happens
      int work_stream(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);

    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_FREQ_SINK_F_IMPL_H */

