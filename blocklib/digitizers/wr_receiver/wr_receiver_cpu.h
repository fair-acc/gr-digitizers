#pragma once

#include <gnuradio/digitizers/wr_receiver.h>

#include "utils.h"

namespace gr::digitizers {

class wr_receiver_cpu : public wr_receiver {
public:
    explicit wr_receiver_cpu(const block_args &args);

    work_return_t work(work_io &wio) override;

    bool          add_timing_event(std::string event_id, int64_t wr_trigger_stamp, int64_t wr_trigger_stamp_utc) override;

    bool          stop() override;

private:
    concurrent_queue<wr_event_t> d_event_queue;
};

} // namespace gr::digitizers
