/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_TIME_DOMAIN_SINK_IMPL_H
#define INCLUDED_DIGITIZERS_TIME_DOMAIN_SINK_IMPL_H

#include <digitizers/time_domain_sink.h>
#include <digitizers/tags.h>
#include "utils.h"

namespace gr {
	namespace digitizers {

    class time_domain_sink_impl : public time_domain_sink
    {
     private:
      float d_samp_rate;
      time_sink_mode_t d_sink_mode;
      signal_metadata_t d_metadata;
      std::size_t d_buffer_size;

      // callback and user-provided ptr
      data_available_cb_t d_callback;
      void *d_user_data;

      // Simple helper structure holding measurement data & metadata
      struct time_domain_measurement_t
      {
        measurement_info_t mdata;
        std::vector<float> values;
        std::vector<float> errors;
      };

      measurement_buffer_t<time_domain_measurement_t> d_measurement_buffer;
      boost::circular_buffer<acq_info_t> d_acq_info_tags;
      unsigned d_lost_count;

     public:
      
      time_domain_sink_impl(std::string name, std::string unit, float samp_rate,
              size_t buffer_size, size_t nr_buffers, time_sink_mode_t mode);

      ~time_domain_sink_impl();

      int work(int noutput_items, gr_vector_const_void_star &input_items,
              gr_vector_void_star &output_items) override;

      signal_metadata_t get_metadata() override;

      size_t get_items(size_t nr_items_to_read, float *values, float *errors, measurement_info_t *info) override;

      void set_callback(data_available_cb_t callback, void *ptr) override;

      size_t get_buffer_size() override;

      float get_sample_rate() override;

      time_sink_mode_t get_sink_mode() override;

    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_TIME_DOMAIN_SINK_IMPL_H */

