#pragma once

#include <gnuradio/digitizers/time_domain_sink.h>

namespace gr::digitizers {

template <class T>
class time_domain_sink_cpu : public time_domain_sink<T>
{
public:
    time_domain_sink_cpu(const typename time_domain_sink<T>::block_args& args);

    work_return_t work(work_io& wio) override;

    void set_callback(
        std::function<void(
            std::vector<float>, std::vector<float>, std::vector<gr::tag_t>, void*)> cb,
        void* user_data) override;

private:
    std::function<void(
        std::vector<float>, std::vector<float>, std::vector<gr::tag_t>, void*)>
        d_cb_copy_data;
    void* d_userdata = nullptr;
};

} // namespace gr::digitizers
