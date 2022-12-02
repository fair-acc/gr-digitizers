#pragma once

#include <gnuradio/digitizers/stream_to_vector_overlay.h>

#include "tags.h"

namespace gr::digitizers {

class stream_to_vector_overlay_cpu : public stream_to_vector_overlay
{
public:
    explicit stream_to_vector_overlay_cpu(const block_args& args);

    bool start() override;

    work_return_t work(work_io& wio) override;

private:
    void save_tags(work_io& wio, std::size_t count);
    void push_tags(work_io& wio, double samp_rate);

    double d_offset = 0;
    acq_info_t d_acq_info;
    uint64_t d_tag_offset = 0;
};

} // namespace gr::digitizers
