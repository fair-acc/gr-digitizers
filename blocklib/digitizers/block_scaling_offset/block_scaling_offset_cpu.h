#pragma once

#include <gnuradio/digitizers/block_scaling_offset.h>

namespace gr {
namespace digitizers {

template<class T>
class block_scaling_offset_cpu : public block_scaling_offset<T> {
public:
    block_scaling_offset_cpu(const typename block_scaling_offset<T>::block_args &args);

    work_return_t work(work_io &wio) override;

private:
    // Declare private variables here
};

}
} // namespace gr::digitizers
