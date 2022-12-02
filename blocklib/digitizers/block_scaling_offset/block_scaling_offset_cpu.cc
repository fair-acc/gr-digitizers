#include "block_scaling_offset_cpu.h"
#include "block_scaling_offset_cpu_gen.h"

namespace gr::digitizers {

block_scaling_offset_cpu::block_scaling_offset_cpu(const block_args& args)
    : INHERITED_CONSTRUCTORS
{
}

work_return_t block_scaling_offset_cpu::work(work_io& wio)
{
    const auto in_sig = wio.inputs()[0].items<float>();
    const auto in_err = wio.inputs()[0].items<float>();
    auto out_sig = wio.outputs()[0].items<float>();
    auto out_err = wio.outputs()[1].items<float>();
    const auto noutput_items = wio.outputs()[0].n_items;
    const auto scale = pmtf::get_as<double>(*this->param_scale);
    const auto offset = pmtf::get_as<double>(*this->param_offset);

    for (std::size_t i = 0; i < noutput_items; i++) {
        out_sig[i] = (in_sig[i] * scale) - offset;
        out_err[i] = in_err[i] * scale;
    }

    wio.consume_each(noutput_items);
    wio.produce_each(noutput_items);

    return work_return_t::OK;
}

} // namespace gr::digitizers
