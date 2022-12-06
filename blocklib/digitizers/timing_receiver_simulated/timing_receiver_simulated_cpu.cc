#include "timing_receiver_simulated_cpu.h"
#include "timing_receiver_simulated_cpu_gen.h"

#include <stdexcept>

namespace gr::digitizers {

timing_receiver_simulated_cpu::timing_receiver_simulated_cpu(const block_args& args)
    : INHERITED_CONSTRUCTORS, _stop_requested(false)
{
    if (args.simulation_mode == timing_receiver_simulation_mode_t::ZEROMQ &&
        args.zmq_endpoint.empty()) {
        throw std::invalid_argument(
            "ZeroMQ mode requires an ZeroMQ endpoint to listen to (DISH mode)");
    }
}

} // namespace gr::digitizers
