#pragma once

#include <gnuradio/digitizers/freq_sink.h>

#include "utils.h"

namespace gr::digitizers {

template<class T>
class freq_sink_cpu : public freq_sink<T> {
public:
    freq_sink_cpu(const typename freq_sink<T>::block_args &args);

    work_return_t         work(work_io &wio) override;

    signal_metadata_t     get_metadata() const override;
    spectra_measurement_t get_measurements(std::size_t nr_of_measurements) override;

private:
    acq_info_t        calculate_acq_info_for_vector(uint64_t offset) const;

    signal_metadata_t d_metadata;

    // Simple helper structure holding measurement data & metadata
    // TODO(PORT) this could be replaced by the new spectra_measurement_t (now containing data & metadata)
    struct freq_domain_buffer_t {
        freq_domain_buffer_t(std::size_t nmeasurements, std::size_t nbins)
            : metadata(nmeasurements), freq(nmeasurements * nbins), magnitude(nmeasurements * nbins), phase(nmeasurements * nbins), nmeasurements(0) {}

        std::vector<spectra_measurement_t::metadata_t> metadata;
        std::vector<float>                             freq;
        std::vector<float>                             magnitude;
        std::vector<float>                             phase;

        // number of measurements stored in the buffer
        std::size_t nmeasurements;
    };

    measurement_buffer_t<freq_domain_buffer_t> d_measurement_buffer;
    boost::circular_buffer<acq_info_t>         d_acq_info_tags;
    std::size_t                                d_lost_count = 0;
};

} // namespace gr::digitizers
