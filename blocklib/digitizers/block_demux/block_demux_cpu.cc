#include "block_demux_cpu.h"
#include "block_demux_cpu_gen.h"

namespace gr::digitizers {

block_demux_cpu::block_demux_cpu(const block_args &args)
    : INHERITED_CONSTRUCTORS {
}

work_return_t block_demux_cpu::work(work_io &wio) {
    const auto in            = wio.inputs()[0].items<char>();
    auto       out           = wio.outputs()[0].items<float>();
    const auto noutput_items = wio.outputs()[0].n_items;

    const auto bit           = pmtf::get_as<std::size_t>(*this->param_bit_to_keep);

    for (std::size_t i = 0; i < noutput_items; i++) {
        out[i] = ((in[i] >> bit) & 1);
    }

    wio.consume_each(noutput_items);
    wio.produce_each(noutput_items);

    return work_return_t::OK;
}

} // namespace gr::digitizers
