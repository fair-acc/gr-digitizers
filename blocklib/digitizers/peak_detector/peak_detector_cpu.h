#pragma once

#include <gnuradio/digitizers/peak_detector.h>

namespace gr {
namespace digitizers {

template<class T>
class peak_detector_cpu : public peak_detector<T> {
public:
    peak_detector_cpu(const typename peak_detector<T>::block_args &args);

    work_return_t work(work_io &wio) override;

private:
    // Declare private variables here
};

}
} // namespace gr::digitizers
