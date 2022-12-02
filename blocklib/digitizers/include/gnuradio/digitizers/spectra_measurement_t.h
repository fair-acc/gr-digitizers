#pragma once

#include <gnuradio/digitizers/api.h>

#include <vector>

namespace gr::digitizers {

/*!
 * \brief Spectra measurement metadata.
 *
 * Note, trigger_timestamp is the the same timestamp as provided to the time-realignment
 * block (via the add_timing_event method), else it contains the local timestamp of the
 * first post-trigger sample.
 *
 * Member-variable timestamp contains timestamp of the first pre-trigger sample, or if
 * there is none the timestamp of the first sample.
 *
 * \ingroup digitizers
 */

// TODO(PORT) this was magnitude only, added freq/mag/phase, review if this structure
// makes any sense
struct DIGITIZERS_API spectra_measurement_t {
    struct metadata_t {
        float timebase;            // distance between measurements (in seconds)
        int64_t timestamp;         // nanoseconds UTC
        int64_t trigger_timestamp; // nanoseconds UTC
        uint32_t status;
        uint32_t number_of_bins; // number of bins
        uint32_t lost_count;     // number of measurements lost
    };

    inline std::size_t nmeasurements() const { return metadata.size(); }

    std::vector<metadata_t> metadata;
    std::vector<float> frequency;
    std::vector<float> magnitude;
    std::vector<float> phase;
};

} // namespace gr::digitizers
