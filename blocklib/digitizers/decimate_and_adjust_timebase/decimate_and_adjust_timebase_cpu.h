#pragma once

#include <gnuradio/digitizers/decimate_and_adjust_timebase.h>

namespace gr::digitizers {

class decimate_and_adjust_timebase_cpu : public virtual decimate_and_adjust_timebase
{
public:
    explicit decimate_and_adjust_timebase_cpu(block_args args);

    work_return_t work(work_io& wio) override;
};

} // namespace gr::digitizers
