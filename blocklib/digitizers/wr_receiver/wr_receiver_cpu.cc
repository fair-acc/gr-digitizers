#include "wr_receiver_cpu.h"
#include "wr_receiver_cpu_gen.h"

#include "tags.h"

namespace gr::digitizers {

wr_receiver_cpu::wr_receiver_cpu(const block_args& args) : INHERITED_CONSTRUCTORS {}

work_return_t wr_receiver_cpu::work(work_io& wio)
{
    const auto noutput_items = wio.outputs()[0].n_items;

    // Zero the output
    memset(wio.outputs()[0].items<float>(), 0, noutput_items * sizeof(float));

    // Attach event tags
    wr_event_t event;
    while (d_event_queue.pop(event)) {
        auto tag = make_wr_event_tag(event, wio.outputs()[0].nitems_written());
        wio.outputs()[0].add_tag(tag);
    }

    wio.produce_each(noutput_items);
    return work_return_t::OK;
}

bool wr_receiver_cpu::add_timing_event(std::string event_id,
                                       int64_t wr_trigger_stamp,
                                       int64_t wr_trigger_stamp_utc)
{
    d_event_queue.push({ .event_id = std::move(event_id),
                         .wr_trigger_stamp = wr_trigger_stamp,
                         .wr_trigger_stamp_utc = wr_trigger_stamp_utc });

    return true;
}

// TODO(PORT) this was ::start(), but then the queue gets cleared asynchronously when
// running the flowgraph, which made this hard to test. Assuming that subsequent starts
// are preceded by a stop()
bool wr_receiver_cpu::stop()
{
    d_event_queue.clear();
    return true;
}

} /* namespace gr::digitizers */
