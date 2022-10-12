#pragma once

#include "utils.h"

#include <gnuradio/digitizers/demux.h>

namespace gr::digitizers {

template<class T>
class demux_cpu : public demux<T> {
public:
    demux_cpu(const typename demux<T>::block_args &args);

    work_return_t work(work_io &wio) override;

private:
    enum class extractor_state {
        WaitTrigger,
        CalcOutputRange,
        WaitAllData,
        OutputData
    };

    std::size_t d_my_history;

    // <tag, offset-relative-to-trigger-tag>
    std::vector<std::pair<acq_info_t, int64_t>> d_acq_info_tags;

    extractor_state                             d_state = extractor_state::WaitTrigger;

    // absolute sample count where the last timing event appeared
    trigger_t d_trigger_tag_data;
    uint64_t  d_last_trigger_offset = 0;

    uint64_t  d_trigger_start_range = 0;
    uint64_t  d_trigger_end_range   = 0;
};

} // namespace gr::digitizers
