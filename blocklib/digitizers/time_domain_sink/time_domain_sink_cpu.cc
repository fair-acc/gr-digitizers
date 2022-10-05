#include "time_domain_sink_cpu.h"
#include "time_domain_sink_cpu_gen.h"

namespace gr {
namespace digitizers {

template <class T>
time_domain_sink_cpu<T>::time_domain_sink_cpu(const typename time_domain_sink<T>::block_args& args)
    : INHERITED_CONSTRUCTORS(T)
{
}

template <class T>
work_return_t time_domain_sink_cpu<T>::work(work_io& wio)
{
    // Do work specific code here
    return work_return_t::OK;
}

} /* namespace digitizers */
} /* namespace gr */
