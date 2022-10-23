#pragma once

#include <gnuradio/digitizers/block_spectral_peaks.h>
#include <gnuradio/digitizers/median_and_average.h>
#include <gnuradio/digitizers/peak_detector.h>

namespace gr::digitizers {

class block_spectral_peaks_cpu : public block_spectral_peaks {
public:
    explicit block_spectral_peaks_cpu(const block_args &args);

private:
    median_and_average::sptr d_med_avg;
    peak_detector::sptr      d_peaks;
};

} // namespace gr::digitizers
