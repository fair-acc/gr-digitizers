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
      return gnuradio::get_initial_sptr
        (new time_domain_sink_impl(name, unit, samp_rate, output_package_size, mode));
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
        d_callback(nullptr),
        d_user_data(nullptr),
        d_acq_info_tags(2048),        // some small number of acq_info tags TODO: Make Magicnumber configurable
        d_lost_count(0)
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

      const auto samp0_count = nitems_read(0);

      // consume all acq_info tags
      std::vector<gr::tag_t> tags;
      get_tags_in_range(tags, 0, samp0_count, samp0_count + ninput_items,
              pmt::string_to_symbol(acq_info_tag_name));
      for (const auto &tag : tags) {
        d_acq_info_tags.push_back(decode_acq_info_tag(tag));
      }

      // consume buffer by buffer
      auto iterations = ninput_items / d_output_package_size;
      for (size_t iteration = 0; iteration < iterations; iteration++)
      {
        float *values;
        float *errors;
        measurement_info_t* meta;

        /* The callback is responsible to allign some memory segment to our pointers (arguments are of type pointer to pointer) */
        d_cb_get_package_buffers(&values, &errors, &meta);

        /* Copy the data */
        const float *input = static_cast<const float *>(input_items[0]);
        memcpy(values, &input[iteration * d_output_package_size], d_output_package_size * sizeof(float));
        if (input_items.size() > 1)
        {
          const float *input_errors = static_cast<const float *>(input_items[1]);
          memcpy(&errors, &input_errors[iteration * d_output_package_size], d_output_package_size * sizeof(float));
        }

        // measurement metadata
        auto start_range = samp0_count + static_cast<uint64_t>(iteration * d_output_package_size);
        auto acq_info = calculate_acq_info_for_range(start_range, start_range + d_output_package_size, d_acq_info_tags, d_samp_rate);

        meta->timebase = acq_info.timebase;
        meta->user_delay = acq_info.user_delay;
        meta->actual_delay = acq_info.actual_delay;
        meta->timestamp = acq_info.timestamp;
        meta->trigger_timestamp = acq_info.trigger_timestamp;
        meta->status = acq_info.status;
        meta->samples_lost = d_lost_count * d_output_package_size;

        d_lost_count = 0;

        d_cb_copy_package_finished();
      } // for each iteration (or buffer)

      return ninput_items;
    }

    signal_metadata_t
    time_domain_sink_impl::get_metadata()
    {
      return d_metadata;
    }

    void
    time_domain_sink_impl::set_callbacks(cb_get_package_buffers_t cb_get_package_buffers, cb_copy_package_finished_t cb_copy_package_finished)
    {
      d_cb_get_package_buffers = cb_get_package_buffers;
      d_cb_copy_package_finished = cb_copy_package_finished;
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

