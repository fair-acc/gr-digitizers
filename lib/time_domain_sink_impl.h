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
      typedef void (*cb_get_buffers_t)(float **values,float **errors, measurement_info_t **metadata);

     private:
      float d_samp_rate;
      time_sink_mode_t d_sink_mode;
      signal_metadata_t d_metadata;
      std::size_t d_buffer_size;

      // callback and user-provided ptr
      cb_get_buffers_t d_callback;
      void *d_user_data;

      boost::circular_buffer<acq_info_t> d_acq_info_tags;

     public:
      
      time_domain_sink_impl(std::string name, std::string unit, float samp_rate,
              size_t buffer_size, size_t nr_buffers, time_sink_mode_t mode);

      ~time_domain_sink_impl();

      int work(int noutput_items, gr_vector_const_void_star &input_items,
              gr_vector_void_star &output_items) override;

      void set_callback(cb_get_buffers_t cb);

      size_t get_buffer_size() override;

      float get_sample_rate() override;

      time_sink_mode_t get_sink_mode() override;

    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_TIME_DOMAIN_SINK_IMPL_H */

