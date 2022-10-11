#pragma once

#include <gnuradio/digitizers/median_and_average.h>

namespace gr {
namespace digitizers {

template<class T>
class median_and_average_cpu : public median_and_average<T> {
public:
    median_and_average_cpu(const typename median_and_average<T>::block_args &args);

    work_return_t work(work_io &wio) override;

private:
    // Declare private variables here
};

}
} // namespace gr::digitizers
