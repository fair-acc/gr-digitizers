/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include <digitizers/status.h>
#include "time_domain_sink_impl.h"

#include <boost/make_shared.hpp>

namespace gr {
  namespace digitizers {

    time_domain_sink::sptr
    time_domain_sink::make(std::string name, std::string unit, float samp_rate, size_t output_package_size, time_sink_mode_t mode)
    {
      return gnuradio::get_initial_sptr(new time_domain_sink_impl(name, unit, samp_rate, output_package_size, mode));
    }

    /*
     * The private constructor
     */
    time_domain_sink_impl::time_domain_sink_impl(std::string name, std::string unit, float samp_rate, size_t output_package_size, time_sink_mode_t mode)
      : gr::sync_block("time_domain_sink",
              gr::io_signature::make(1, 2, sizeof(float)),
              gr::io_signature::make(0, 0, 0)),
        d_samp_rate(samp_rate),
        d_sink_mode(mode),
        d_output_package_size(output_package_size),
        d_cb_copy_data(nullptr)
    {
      d_metadata.name = name;
      d_metadata.unit = unit;

      // To simplify data copy in chunks
      set_output_multiple(d_output_package_size);

      // This is a sink
      set_tag_propagation_policy(tag_propagation_policy_t::TPP_DONT);
    }

    /*
     * Our virtual destructor.
     */
    time_domain_sink_impl::~time_domain_sink_impl()
    {
    }

    int
    time_domain_sink_impl::work(int ninput_items, gr_vector_const_void_star &input_items, gr_vector_void_star &output_items)
    {
      assert(ninput_items % d_output_package_size == 0);

      if(d_cb_copy_data == nullptr )
      {
          GR_LOG_WARN(d_logger, "Callback for sink '" + d_metadata.name + "' is not initialized");
          return ninput_items;
      }

      const float *input_values = static_cast<const float *>(input_items[0]);
      const float *input_errors = static_cast<const float *>(input_items[1]);

      std::size_t  input_errors_size = 0;
      if (input_items.size() > 1)
          input_errors_size = d_output_package_size;

      auto tag_index = nitems_read(0);

      std::vector<gr::tag_t> tags;

      // consume package by package
      for (size_t i = 0; i < ninput_items; i+= d_output_package_size)
      {
        /* get tags for this package */
        get_tags_in_range(tags, 0, tag_index, tag_index + d_output_package_size - 1, pmt::string_to_symbol(acq_info_tag_name));
        tag_index += d_output_package_size;

        /* trigger callback of host application to copy the data*/
        d_cb_copy_data(&input_values[i],
                      d_output_package_size,
                     &input_errors[i],
                     input_errors_size,
                     tags,
                     d_userdata);
      }

      return ninput_items;
    }

    signal_metadata_t
    time_domain_sink_impl::get_metadata()
    {
      return d_metadata;
    }

    void
    time_domain_sink_impl::set_callback(cb_copy_data_t cb_copy_data, void* userdata)
    {
      d_cb_copy_data = cb_copy_data;
      d_userdata = userdata;
    }

    size_t
    time_domain_sink_impl::get_output_package_size()
    {
      return d_output_package_size;
    }

    time_sink_mode_t
    time_domain_sink_impl::get_sink_mode()
    {
      return d_sink_mode;
    }

    float
    time_domain_sink_impl::get_sample_rate()
    {
      return d_samp_rate;
    }

  } /* namespace digitizers */
} /* namespace gr */

