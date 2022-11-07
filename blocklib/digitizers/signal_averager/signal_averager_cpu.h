#pragma once

#include <gnuradio/digitizers/signal_averager.h>

namespace gr::digitizers {

class signal_averager_cpu : public virtual signal_averager {
public:
    explicit signal_averager_cpu(block_args args);
    work_return_t work(work_io &wio) override;

private:
    // private variables here
};

} // namespace gr::digitizers
