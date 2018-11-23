/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "freq_sink_f_impl.h"
#include <boost/make_shared.hpp>

namespace gr {
  namespace digitizers {

    freq_sink_f::sptr
    freq_sink_f::make(std::string name, float samp_rate, size_t nbins,
            size_t nmeasurements, size_t nbuffers, freq_sink_mode_t mode)
    {
      return gnuradio::get_initial_sptr
        (new freq_sink_f_impl(name, samp_rate, nbins, nmeasurements, nbuffers, mode));
    }

    /*
     * The private constructor
     */
    freq_sink_f_impl::freq_sink_f_impl(std::string name, float samp_rate, size_t nbins,
            size_t nmeasurements, size_t nbuffers, freq_sink_mode_t mode)
      : gr::sync_block("freq_sink_f",
              gr::io_signature::makev(3, 3,
                  std::vector<int>(
                  {
                    static_cast<int>(nbins * sizeof(float)),
                    static_cast<int>(nbins * sizeof(float)),
                    static_cast<int>(nbins * sizeof(float))
                  })),
              gr::io_signature::make(0, 0, 0)),
        d_mode(mode),
        d_metadata({"", name}),
        d_samp_rate(samp_rate),
        d_callback(nullptr),
        d_user_data(nullptr),
        d_nbins(nbins),
        d_nmeasurements(nmeasurements),
        d_nbuffers(nbuffers),
        d_measurement_buffer(nbuffers),
        d_acq_info_tags(4096), // some small number of acq_info tags
        d_lost_count(0)
    {
      // initialize buffers
      for (unsigned i = 0; i < nbuffers; i++) {
        auto ptr = boost::make_shared<freq_domain_buffer_t>(nmeasurements, nbins);
        d_measurement_buffer.return_free_buffer(ptr);
      }

      set_output_multiple(nmeasurements);
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
      const float *magnitude = (const float *) input_items[0];
      const float *phase = (const float *) input_items[1];
      const float *freqs = (const float *) input_items[2];

      assert(noutput_items % d_nmeasurements == 0);

      const auto samp0_count = nitems_read(0);

      // consume all acq_info tags
      std::vector<gr::tag_t> tags;
      get_tags_in_range(tags, 0, samp0_count, samp0_count + noutput_items,
              pmt::string_to_symbol(acq_info_tag_name));
      for (const auto &tag : tags) {
        d_acq_info_tags.push_back(decode_acq_info_tag(tag));
      }

      const auto floats_per_buffer = d_nmeasurements * d_nbins;

      // consume buffer by buffer
      auto iterations = noutput_items / d_nmeasurements;
      for (size_t iteration = 0; iteration < iterations; iteration++)
      {
        auto measurement = d_measurement_buffer.get_free_buffer();
        if (!measurement) {
          d_lost_count++;
          continue;
        }

        assert(measurement->freq.size() == floats_per_buffer);
        assert(measurement->phase.size() == floats_per_buffer);
        assert(measurement->magnitude.size() == floats_per_buffer);

        // copy over the data
        memcpy(&measurement->freq[0], &freqs[iteration * floats_per_buffer], floats_per_buffer * sizeof(float));
        memcpy(&measurement->magnitude[0], &magnitude[iteration * floats_per_buffer], floats_per_buffer * sizeof(float));
        memcpy(&measurement->phase[0], &phase[iteration * floats_per_buffer], floats_per_buffer * sizeof(float));

        // measurement metadata
        for (unsigned i = 0; i < d_nmeasurements; i++) {
          auto offset = samp0_count + static_cast<uint64_t>(iteration * d_nmeasurements + i);
          auto acq_info = calculate_acq_info_for_vector(offset);

          measurement->metadata[i].timebase = 1.0f / d_samp_rate;
          measurement->metadata[i].timestamp = acq_info.timestamp;
          measurement->metadata[i].trigger_timestamp = 0;
          measurement->metadata[i].status = acq_info.status;
          measurement->metadata[i].number_of_bins = d_nbins;

          measurement->metadata[i].lost_count = d_lost_count * d_nmeasurements;
          d_lost_count = 0;
        }

        d_measurement_buffer.add_measurement(measurement);

        if (d_callback != nullptr) {
          data_available_event_t args;
          args.trigger_timestamp = measurement->metadata[0].trigger_timestamp != -1
                  ? measurement->metadata[0].trigger_timestamp
                  : measurement->metadata[0].timestamp;
          args.signal_name = d_metadata.name;
          d_callback(&args, d_user_data);
        }

      } // for each iteration (or buffer)

      return noutput_items;
    }

    signal_metadata_t
    freq_sink_f_impl::get_metadata()
    {
      return d_metadata;
    }

    size_t
    freq_sink_f_impl::get_measurements(size_t nr_measurements,
            spectra_measurement_t *metadata, float *frequency, float *magnitude, float *phase)
    {
      auto buffer = d_measurement_buffer.get_measurement();
      if (!buffer) {
        return 0;
      }

      // We were instructed to drop the data
      if (metadata == nullptr || frequency == nullptr
              || magnitude == nullptr || phase == nullptr)
      {
        d_measurement_buffer.return_free_buffer(buffer);
        return 0;
      }

      if (nr_measurements > d_nmeasurements) {
        nr_measurements = d_nmeasurements;
      }

      const auto data_size = nr_measurements * d_nbins * sizeof(float);
      memcpy(frequency, &buffer->freq[0],      data_size);
      memcpy(magnitude, &buffer->magnitude[0], data_size);
      memcpy(phase,     &buffer->phase[0],     data_size);

      const auto metadata_size = nr_measurements * sizeof(spectra_measurement_t);
      memcpy(metadata, &buffer->metadata[0], metadata_size);

      d_measurement_buffer.return_free_buffer(buffer);

      return nr_measurements;
    }

    void
    freq_sink_f_impl::set_callback(data_available_cb_t callback, void *ptr)
    {
      d_callback = callback;
      d_user_data = ptr;
    }

    size_t
    freq_sink_f_impl::get_nbins()
    {
      return d_nbins;
    }

    size_t
    freq_sink_f_impl::get_nmeasurements()
    {
      return d_nmeasurements;
    }

    float
    freq_sink_f_impl::get_sample_rate()
    {
      return d_samp_rate;
    }

    freq_sink_mode_t
    freq_sink_f_impl::get_sink_mode()
    {
      return d_mode;
    }

    acq_info_t
    freq_sink_f_impl::calculate_acq_info_for_vector(uint64_t offset) const
    {
      acq_info_t result {};
      result.timestamp = -1;

//      for (const auto & info : d_acq_info_tags) {
//        if (info.offset <= offset) {
//          result = info;  // keep replacing until the first in the range is found
//        }
//      }

//      // update timestamp
//      if (result.timestamp != -1) {
//        auto delta_samples = offset >= result.offset
//                ? static_cast<float>(offset - result.offset)
//                : -static_cast<float>(result.offset - offset);
//        auto delta_ns = static_cast<int64_t>((delta_samples / d_samp_rate) * 1000000000.0f);
//        result.timestamp += delta_ns;
//      }
//
//      result.timebase = 1.0f / d_samp_rate;

      return result;
    }

  } /* namespace digitizers */
} /* namespace gr */

