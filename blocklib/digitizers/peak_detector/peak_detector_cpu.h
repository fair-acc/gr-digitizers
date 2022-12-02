#pragma once

#include <gnuradio/digitizers/peak_detector.h>

namespace gr::digitizers {

class peak_detector_cpu : public peak_detector
{
public:
    peak_detector_cpu(const block_args& args);

    work_return_t work(work_io& wio) override;
};

} // namespace gr::digitizers
