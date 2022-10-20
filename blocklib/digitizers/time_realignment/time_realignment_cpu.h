#pragma once

#include "utils.h"

#include <gnuradio/digitizers/time_realignment.h>

namespace gr::digitizers {

template<class T>
class time_realignment_cpu : public time_realignment<T> {
public:
    time_realignment_cpu(const typename time_realignment<T>::block_args &args);

    work_return_t work(work_io &wio) override;

    bool          add_timing_event(std::string event_id, int64_t wr_trigger_stamp, int64_t wr_trigger_stamp_utc) override;

    void          on_parameter_change(param_action_sptr action) override;

private:
    void    check_pending_event_size();

    void    update_pending_events();

    bool    fill_wr_stamp(trigger_t &trigger_tag_data);

    int64_t get_user_delay_ns() const;

    // stored in ns to allow fast comparioson
    int64_t d_triggerstamp_matching_tolerance_ns;

    // maximum time incoming triggers and samples will be buffered before forwarding them without realligment of the trigger tags
    int64_t  d_max_buffer_time_ns;
    uint64_t d_not_found_stamp_utc;

    // cyclic buffer of white rabbit events
    std::vector<wr_event_t>           d_wr_events;
    size_t                            d_wr_events_size;

    std::vector<wr_event_t>::iterator d_wr_events_write_iter;
    std::vector<wr_event_t>::iterator d_wr_events_read_iter;
};

} // namespace gr::digitizers
