#pragma once

#include <gnuradio/digitizers/timing_receiver_simulated.h>

#include <chrono>
#include <thread>

namespace gr::digitizers {

class timing_receiver_simulated_cpu : public timing_receiver_simulated
{
public:
    explicit timing_receiver_simulated_cpu(const block_args& args);

    bool start() override;
    bool stop() override;

private:
    void
    loop(std::string trigger_name, std::chrono::milliseconds interval, double offset);

    std::atomic<bool> d_stop_requested;
    std::jthread d_thread;
};

} // namespace gr::digitizers
