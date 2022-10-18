#pragma once

#include "tags.h"

#include <gnuradio/digitizers/post_mortem_sink.h>

#include <mutex>

namespace gr::digitizers {

template<class T>
class post_mortem_sink_cpu : public post_mortem_sink<T> {
public:
    post_mortem_sink_cpu(const typename post_mortem_sink<T>::block_args &args);

    work_return_t      work(work_io &wio) override;

    signal_metadata_t  get_metadata() const override;
    void               freeze_buffer() override;
    post_mortem_data_t get_post_mortem_data(std::size_t items_to_read);

private:
    mutable std::mutex d_mutex;

    std::vector<float> d_buffer_values;
    std::vector<float> d_buffer_errors;
    std::size_t        d_nitems_read = 0;
    std::size_t        d_write_index = 0;

    // last acquisition info tag
    acq_info_t d_acq_info;
    uint64_t   d_acq_info_offset = 0;

    // metadata
    signal_metadata_t d_metadata;

    bool              d_frozen = false;
};

} // namespace gr::digitizers
