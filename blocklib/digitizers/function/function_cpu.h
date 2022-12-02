#pragma once

#include "utils.h"

#include <gnuradio/digitizers/function.h>

namespace gr::digitizers {

class function_cpu : public virtual function
{
public:
    explicit function_cpu(block_args args);

    bool start() override;

    work_return_t work(work_io& wio) override;

private:
    acq_info_t d_acq_info;
    std::vector<float> d_timing;
    std::vector<float> d_ref;
    std::vector<float> d_min;
    std::vector<float> d_max;
};

} // namespace gr::digitizers
