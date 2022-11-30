#pragma once

#include <gnuradio/digitizers/interlock_generation.h>
#include <gnuradio/digitizers/tags.h>

#include <functional>

namespace gr::digitizers {

class interlock_generation_cpu : public interlock_generation
{
public:
    explicit interlock_generation_cpu(const block_args& args);

    bool start() override;

    void set_callback(std::function<void(int64_t, void*)> cb, void* user_data) override;

    work_return_t work(work_io& wio) override;

private:
    bool d_interlock_issued = false;
    acq_info_t d_acq_info;
    std::function<void(int64_t, void*)> d_callback;
    void* d_user_data = nullptr;
};

} // namespace gr::digitizers
