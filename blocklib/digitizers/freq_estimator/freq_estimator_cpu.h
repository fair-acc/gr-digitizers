#pragma once

#include "utils.h"
#include <gnuradio/digitizers/freq_estimator.h>

namespace gr::digitizers {

class freq_estimator_cpu : public freq_estimator {
public:
    explicit freq_estimator_cpu(const block_args &args);

    work_return_t work(work_io &wio) override;

private:
    average_filter<float>  d_sig_avg;
    average_filter<double> d_freq_avg;
    float                  d_avg_freq       = 0.f;
    double                 d_prev_zero_dist = 0.;
    float                  d_old_sig_avg    = 0.f;
    int                    d_counter        = 0;
};

} // namespace gr::digitizers
