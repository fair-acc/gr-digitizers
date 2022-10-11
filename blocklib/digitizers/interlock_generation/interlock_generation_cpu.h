#pragma once

#include <gnuradio/digitizers/interlock_generation.h>
#include <gnuradio/digitizers/tags.h>

namespace gr::digitizers {

template<class T>
class interlock_generation_cpu : public interlock_generation<T> {
public:
    interlock_generation_cpu(const typename interlock_generation<T>::block_args &args);

    bool          start() override;

    work_return_t work(work_io &wio) override;

private:
    bool       d_interlock_issued = false;
    acq_info_t d_acq_info;
};

} // namespace gr::digitizers
