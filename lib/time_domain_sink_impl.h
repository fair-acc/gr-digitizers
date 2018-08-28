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
      std::size_t d_output_package_size;

      cb_copy_data_t d_cb_copy_data;
      void* d_userdata;

     public:
      
      time_domain_sink_impl(std::string name, std::string unit, float samp_rate, size_t output_package_size, time_sink_mode_t mode);

      ~time_domain_sink_impl();

      int work(int noutput_items, gr_vector_const_void_star &input_items, gr_vector_void_star &output_items) override;

      void set_callback(cb_copy_data_t cb_copy_data, void* userdata) override;

      size_t get_output_package_size() override;

      float get_sample_rate() override;

      time_sink_mode_t get_sink_mode() override;

      signal_metadata_t get_metadata() override;

    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_TIME_DOMAIN_SINK_IMPL_H */

