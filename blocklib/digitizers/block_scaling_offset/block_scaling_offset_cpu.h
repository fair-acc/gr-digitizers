#pragma once

#include <gnuradio/digitizers/block_scaling_offset.h>

namespace gr::digitizers {

class block_scaling_offset_cpu : public block_scaling_offset
{
public:
    explicit block_scaling_offset_cpu(const block_args& args);

    work_return_t work(work_io& wio) override;
};

} // namespace gr::digitizers
