#include "freq_sink_cpu.h"
#include "freq_sink_cpu_gen.h"

namespace gr::digitizers {

freq_sink_cpu::freq_sink_cpu(const block_args& args)
    : INHERITED_CONSTRUCTORS,
      d_metadata{ .name = args.name },
      d_measurement_buffer(args.nbuffers)
{
    // initialize buffers
    for (std::size_t i = 0; i < args.nbuffers; i++) {
        auto ptr = std::make_shared<freq_domain_buffer_t>(args.nmeasurements, args.nbins);
        d_measurement_buffer.return_free_buffer(ptr);
    }

    set_output_multiple(args.nmeasurements);
}

work_return_t freq_sink_cpu::work(work_io& wio)
{
    const auto magnitude = wio.inputs()[0].items<float>();
    const auto phase = wio.inputs()[1].items<float>();
    const auto freqs = wio.inputs()[2].items<float>();

    const auto noutput_items =
        wio.inputs()[0].n_items; // TODO(PORT) we don't have an output, is this correct?
                                 // call it ninput_items?

    const auto samp_rate = std::get<float>(*this->param_sample_rate);
    const auto nmeasurements = std::get<std::size_t>(*this->param_nmeasurements);
    const auto nbins = std::get<std::size_t>(*this->param_nbins);

    assert(noutput_items % nmeasurements == 0);

    const auto samp0_count = wio.inputs()[0].nitems_read();

    // consume all acq_info tags
    const auto tags =
        filter_tags(wio.inputs()[0].tags_in_window(0, noutput_items), acq_info_tag_name);

    for (const auto& tag : tags) {
        d_acq_info_tags.push_back(decode_acq_info_tag(tag));
    }

    const auto floats_per_buffer = nmeasurements * nbins;

    // consume buffer by buffer
    const auto iterations = noutput_items / nmeasurements;
    for (size_t iteration = 0; iteration < iterations; iteration++) {
        auto measurement = d_measurement_buffer.get_free_buffer();
        if (!measurement) {
            d_lost_count++;
            continue;
        }

        assert(measurement->metadata.size() == nmeasurements);
        assert(measurement->freq.size() == floats_per_buffer);
        assert(measurement->phase.size() == floats_per_buffer);
        assert(measurement->magnitude.size() == floats_per_buffer);

        // copy over the data
        memcpy(&measurement->freq[0],
               &freqs[iteration * floats_per_buffer],
               floats_per_buffer * sizeof(float));
        memcpy(&measurement->magnitude[0],
               &magnitude[iteration * floats_per_buffer],
               floats_per_buffer * sizeof(float));
        memcpy(&measurement->phase[0],
               &phase[iteration * floats_per_buffer],
               floats_per_buffer * sizeof(float));

        // measurement metadata
        for (std::size_t i = 0; i < nmeasurements; i++) {
            auto offset =
                samp0_count + static_cast<uint64_t>(iteration * nmeasurements + i);
            auto acq_info = calculate_acq_info_for_vector(offset);

            measurement->metadata[i].timebase = 1.0f / samp_rate;
            measurement->metadata[i].timestamp = acq_info.timestamp;
            measurement->metadata[i].trigger_timestamp = 0;
            measurement->metadata[i].status = acq_info.status;
            measurement->metadata[i].number_of_bins = nbins;

            measurement->metadata[i].lost_count = d_lost_count * nmeasurements;
            d_lost_count = 0;
        }

        d_measurement_buffer.add_measurement(measurement);

        if (d_callback) {
            data_available_event_t args;
            const int64_t trigger_timestamp =
                measurement->metadata[0].trigger_timestamp != -1
                    ? measurement->metadata[0].trigger_timestamp
                    : measurement->metadata[0].timestamp;
            d_callback(trigger_timestamp, d_metadata.name, d_user_data);
        }
    } // for each iteration (or buffer)

    wio.produce_each(noutput_items);
    return work_return_t::OK;
}

signal_metadata_t freq_sink_cpu::get_metadata() const
{
    // TODO(PORT) do we really need this? why not make name/unit gettable?
    return d_metadata;
}

void freq_sink_cpu::set_callback(
    std::function<void(int64_t, std::string, void*)> callback, void* user_data)
{
    d_callback = callback;
    d_user_data = user_data;
}

spectra_measurement_t freq_sink_cpu::get_measurements(std::size_t nr_measurements)
{
    spectra_measurement_t ret;

    auto buffer = d_measurement_buffer.get_measurement();
    if (!buffer) {
        return ret;
    }

#ifdef PORT_DISABLED // TODO(PORT) add less cryptic clear() method?
    // We were instructed to drop the data
    if (metadata == nullptr || frequency == nullptr || magnitude == nullptr ||
        phase == nullptr) {
        d_measurement_buffer.return_free_buffer(buffer);
        return 0;
    }
#endif
    nr_measurements =
        std::min(nr_measurements, std::get<std::size_t>(*this->param_nmeasurements));
    const auto nbins = std::get<std::size_t>(*this->param_nbins);

    const auto data_size = nr_measurements * nbins * sizeof(float);

    ret.frequency.resize(nr_measurements * nbins);
    ret.magnitude.resize(nr_measurements * nbins);
    ret.phase.resize(nr_measurements * nbins);

    memcpy(ret.frequency.data(), &buffer->freq[0], data_size);
    memcpy(ret.magnitude.data(), &buffer->magnitude[0], data_size);
    memcpy(ret.phase.data(), &buffer->phase[0], data_size);

    ret.metadata.resize(nr_measurements);

    memcpy(ret.metadata.data(),
           &buffer->metadata[0],
           nr_measurements * sizeof(spectra_measurement_t::metadata_t));

    d_measurement_buffer.return_free_buffer(buffer);

    return ret;
}

acq_info_t freq_sink_cpu::calculate_acq_info_for_vector(uint64_t offset) const
{
    acq_info_t result{};
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
    //        auto delta_ns = static_cast<int64_t>((delta_samples / d_samp_rate) *
    //        1000000000.0f); result.timestamp += delta_ns;
    //      }
    //
    //      result.timebase = 1.0f / d_samp_rate;

    return result;
}

} // namespace gr::digitizers
