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
    time_domain_sink::make(std::string name, std::string unit, float samp_rate, size_t buffer_size, size_t nr_buffers, time_sink_mode_t mode)
    {
      return gnuradio::get_initial_sptr
        (new time_domain_sink_impl(name, unit, samp_rate, buffer_size, nr_buffers, mode));
    }

    /*
     * The private constructor
     */
    time_domain_sink_impl::time_domain_sink_impl(std::string name, std::string unit, float samp_rate,
            size_t buffer_size, size_t nr_buffers, time_sink_mode_t mode)
      : gr::sync_block("time_domain_sink",
              gr::io_signature::make(1, 2, sizeof(float)),
              gr::io_signature::make(0, 0, 0)),
        d_samp_rate(samp_rate),
        d_sink_mode(mode),
        d_buffer_size(buffer_size),
        d_callback(nullptr),
        d_user_data(nullptr),
        d_measurement_buffer(nr_buffers),
        d_acq_info_tags(2048),        // some small number of acq_info tags
        d_lost_count(0)
    {
      d_metadata.name = name;
      d_metadata.unit = unit;

      // initialize buffer
      for (size_t i = 0; i < nr_buffers; i++) {
        auto ptr = boost::make_shared<time_domain_measurement_t>();
        ptr->values.resize(d_buffer_size);
        ptr->errors.resize(d_buffer_size);
        memset(&ptr->errors[0], 0, sizeof(float) * d_buffer_size);

        d_measurement_buffer.return_free_buffer(ptr);
      }

      // To simplify data copy in chunks
      set_output_multiple(d_buffer_size);

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
    time_domain_sink_impl::work(int ninput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      assert(ninput_items % d_buffer_size == 0);

      const auto samp0_count = nitems_read(0);

      // consume all acq_info tags
      std::vector<gr::tag_t> tags;
      get_tags_in_range(tags, 0, samp0_count, samp0_count + ninput_items,
              pmt::string_to_symbol(acq_info_tag_name));
      for (const auto &tag : tags) {
        d_acq_info_tags.push_back(decode_acq_info_tag(tag));
      }

      // consume buffer by buffer
      auto iterations = ninput_items / d_buffer_size;
      for (size_t iteration = 0; iteration < iterations; iteration++)
      {
          /* ###################################### */

          d_callback()
          /* ###################################### */
        auto measurement = d_measurement_buffer.get_free_buffer();
        if (!measurement) {
          d_lost_count++;
          continue;
        }

        assert(measurement->values.size() == d_buffer_size);
        assert(measurement->errors.size() == d_buffer_size);

        // copy over the data
        const float *input = static_cast<const float *>(input_items[0]);
        memcpy(&measurement->values[0], &input[iteration * d_buffer_size], d_buffer_size * sizeof(float));
        if (input_items.size() > 1) {
          const float *input_errors = static_cast<const float *>(input_items[1]);
          memcpy(&measurement->errors[0], &input_errors[iteration * d_buffer_size], d_buffer_size * sizeof(float));
        }

        // measurement metadata
        auto start_range = samp0_count + static_cast<uint64_t>(iteration * d_buffer_size);
        auto acq_info = calculate_acq_info_for_range(start_range, start_range + d_buffer_size,
                d_acq_info_tags, d_samp_rate);

        measurement->mdata.timebase = acq_info.timebase;
        measurement->mdata.user_delay = acq_info.user_delay;
        measurement->mdata.actual_delay = acq_info.actual_delay;
        measurement->mdata.timestamp = acq_info.timestamp;
        measurement->mdata.trigger_timestamp = acq_info.trigger_timestamp;
        measurement->mdata.status = acq_info.status;
        measurement->mdata.samples_lost = d_lost_count * d_buffer_size;

        d_measurement_buffer.add_measurement(measurement);
        d_lost_count = 0;

        if (d_callback != nullptr) {
          data_available_event_t args;
          args.trigger_timestamp = measurement->mdata.trigger_timestamp != -1
                  ? measurement->mdata.trigger_timestamp : measurement->mdata.timestamp;
          args.signal_name = d_metadata.name;
          d_callback(&args, d_user_data);
        }

      } // for each iteration (or buffer)

      return ninput_items;
    }

    signal_metadata_t
    time_domain_sink_impl::get_metadata()
    {
      return d_metadata;
    }

    size_t
    time_domain_sink_impl::get_items(size_t nr_items_to_read,
            float *values, float *errors, measurement_info_t *info)
    {
      auto buffer = d_measurement_buffer.get_measurement();
      if (!buffer) {
        return 0;
      }

      // If any pointer is null, we were instructed to drop the data
      if (values == nullptr || errors == nullptr || info == nullptr) {
        d_measurement_buffer.return_free_buffer(buffer);
        return 0;
      }

      if (nr_items_to_read > d_buffer_size) {
        nr_items_to_read = d_buffer_size;
      }

      std::copy(buffer->values.begin(), buffer->values.begin() + nr_items_to_read, values);
      std::copy(buffer->errors.begin(), buffer->errors.begin() + nr_items_to_read, errors);

      *info = buffer->mdata;

      d_measurement_buffer.return_free_buffer(buffer);

      return nr_items_to_read;
    }

    void
    time_domain_sink_impl::set_callback(data_available_cb_t callback, void *ptr)
    {
      d_callback = callback;
      d_user_data = ptr;
    }

    size_t
    time_domain_sink_impl::get_buffer_size()
    {
      return d_buffer_size;
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

