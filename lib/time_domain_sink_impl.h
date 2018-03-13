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

#ifndef INCLUDED_DIGITIZERS_TIME_DOMAIN_SINK_IMPL_H
#define INCLUDED_DIGITIZERS_TIME_DOMAIN_SINK_IMPL_H

#include <digitizers/time_domain_sink.h>
#include <digitizers/tags.h>
#include <utils.h>

namespace gr {
	namespace digitizers {

    class time_domain_sink_impl : public time_domain_sink
    {
     private:
      time_sink_mode_t d_sink_mode;

      // buffer stuff, vectors abused to allocate memory
      std::vector<float> d_buffer_values;
      std::vector<float> d_buffer_errors;
      uint32_t d_buffer_size;
      uint32_t d_size;

      std::vector<gr::tag_t> d_tags;
      std::vector<int64_t> d_snapshot_points; // in nanoseconds

      // timestamp and status associated with the samples in the buffer
      int64_t  d_timestamp;
      uint32_t d_status;

      // acquisition info tag info
      acq_info_t d_acq_info;

      signal_metadata_t d_metadata;
      boost::mutex d_mutex;

      // callback stuff
      data_available_cb_t d_callback;
      void *d_user_data;

      /// Fast data stuff
      int d_samples_left;
      bool d_collecting_samples;
      bool d_waiting_readout;

     public:
      
      time_domain_sink_impl(std::string name, std::string unit, size_t buffer_size, time_sink_mode_t mode);

      ~time_domain_sink_impl();

      int work(int noutput_items, gr_vector_const_void_star &input_items,
              gr_vector_void_star &output_items) override;

      void set_snapshot_points(const std::vector<float> &points) override;

      signal_metadata_t get_metadata() override;

      size_t get_items(size_t nr_items_to_read, float *values, float *errors, measurement_info_t *info) override;

      size_t get_items_count() override;

      void set_items_available_callback(data_available_cb_t callback, void *ptr) override;

      size_t get_buffer_size() override;

      time_sink_mode_t get_sink_mode() override;

      bool is_data_rdy() override;

      void notify_data_ready() override;

     private:

      void handle_acq_info_tags(int ninput_items);

      int work_slow_data(int noutput_items, gr_vector_const_void_star &input_items,
              gr_vector_void_star &output_items);

      int work_fast_data(int noutput_items, gr_vector_const_void_star &input_items,
              gr_vector_void_star &output_items);

      int work_snapshot_data(int noutput_items, gr_vector_const_void_star &input_items,
              gr_vector_void_star &output_items);

      size_t get_slow_data_items(size_t nr_items_to_read,
              float *values, float *errors, measurement_info_t *info);

      size_t get_fast_data_items(size_t nr_items_to_read,
                    float *values, float *errors, measurement_info_t *info);

    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_TIME_DOMAIN_SINK_IMPL_H */

