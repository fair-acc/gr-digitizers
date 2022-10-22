#pragma once

#include <gnuradio/digitizers/median_and_average.h>

namespace gr::digitizers {

class median_and_average_cpu : public median_and_average {
public:
    explicit median_and_average_cpu(const block_args &args);

    work_return_t work(work_io &wio) override;
};

} // namespace gr::digitizers
