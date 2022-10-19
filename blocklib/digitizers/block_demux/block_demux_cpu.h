#pragma once

#include <gnuradio/digitizers/block_demux.h>

namespace gr::digitizers {

template<class T>
class block_demux_cpu : public block_demux<T> {
public:
    block_demux_cpu(const typename block_demux<T>::block_args &args);

    work_return_t work(work_io &wio) override;
};

} // namespace gr::digitizers
