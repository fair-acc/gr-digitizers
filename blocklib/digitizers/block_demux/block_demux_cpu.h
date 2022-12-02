#pragma once

#include <gnuradio/digitizers/block_demux.h>

namespace gr::digitizers {

class block_demux_cpu : public block_demux
{
public:
    explicit block_demux_cpu(const block_args& args);

    work_return_t work(work_io& wio) override;
};

} // namespace gr::digitizers
