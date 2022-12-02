#include "decimate_and_adjust_timebase_cpu.h"
#include "decimate_and_adjust_timebase_cpu_gen.h"

namespace gr::digitizers {

decimate_and_adjust_timebase_cpu::decimate_and_adjust_timebase_cpu(block_args args)
    : INHERITED_CONSTRUCTORS
{
    set_relative_rate(1. / args.decimation);
    set_tag_propagation_policy(tag_propagation_policy_t::TPP_CUSTOM);
}

work_return_t decimate_and_adjust_timebase_cpu::work(work_io& wio)
{
    const auto decimation = pmtf::get_as<std::size_t>(*this->param_decimation);

    if (wio.inputs()[0].n_items < decimation) {
        return work_return_t::INSUFFICIENT_INPUT_ITEMS;
    }

    if (wio.outputs()[0].n_items == 0) {
        return work_return_t::INSUFFICIENT_OUTPUT_ITEMS;
    }

    const auto noutput_items =
        std::min(wio.outputs()[0].n_items, wio.inputs()[0].n_items / decimation);
    // add tags with corrected offset to the output stream
    auto tags = wio.inputs()[0].tags_in_window(0, wio.inputs()[0].n_items);

    for (auto& tag : tags) {
        if (decimation != 0)
            tag.set_offset(uint64_t(tag.offset() / decimation));
        wio.outputs()[0].add_tag(tag);
    }

    wio.consume_each(noutput_items * decimation);
    wio.produce_each(noutput_items);

    return work_return_t::OK;
}

} // namespace gr::digitizers
