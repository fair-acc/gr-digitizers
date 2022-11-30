#include "stream_to_vector_overlay_cpu.h"
#include "stream_to_vector_overlay_cpu_gen.h"

#include "utils.h"

namespace gr::digitizers {

stream_to_vector_overlay_cpu::stream_to_vector_overlay_cpu(const block_args& args)
    : INHERITED_CONSTRUCTORS
{
    d_acq_info.timestamp = -1;

    set_tag_propagation_policy(tag_propagation_policy_t::TPP_DONT);
}

bool stream_to_vector_overlay_cpu::start()
{
    d_acq_info.timestamp = -1;
    return true;
}

work_return_t stream_to_vector_overlay_cpu::work(work_io& wio)
{
    const auto in = wio.inputs()[0].items<float>();
    auto out = wio.outputs()[0].items<float>();

    const auto vec_size = pmtf::get_as<std::size_t>(*this->param_vec_size);
    const auto samp_rate = pmtf::get_as<double>(*this->param_samp_rate);
    const auto delta_t = pmtf::get_as<double>(*this->param_delta_t);

    if (d_offset >= 1.0) {
        // move along until new samples arrive
        const auto consumable_count =
            std::min(wio.inputs()[0].n_items, static_cast<std::size_t>(d_offset));
        save_tags(wio, consumable_count);

        d_offset -= consumable_count;

        wio.consume_each(consumable_count);
        return work_return_t::OK;
    }
    else if (wio.inputs()[0].n_items < vec_size) {
        // make sure we have enough input data to copy
        return work_return_t::INSUFFICIENT_INPUT_ITEMS;
    }
    else {
        // enough samples on input!
        save_tags(wio, vec_size);
        /*
        for(int i = 0; i < d_vec_size; i++){
            out[i] = in[i];
        }
        */

        memcpy(out, in, vec_size * sizeof(float));
        push_tags(wio, samp_rate);
        wio.consume_each(0);
        d_offset += delta_t * samp_rate;

        wio.produce_each(1);
        return work_return_t::OK;
    }
}

void stream_to_vector_overlay_cpu::save_tags(work_io& wio, std::size_t count)
{
    const auto this_tags =
        filter_tags(wio.inputs()[0].tags_in_window(0, count), acq_info_tag_name);
    if (!this_tags.empty()) {
        d_acq_info = decode_acq_info_tag(this_tags.back());
        d_tag_offset = this_tags.back().offset();
    }
}

void stream_to_vector_overlay_cpu::push_tags(work_io& wio, double samp_rate)
{
    // fix offset.
    if (d_acq_info.timestamp != -1) {
        d_acq_info.timestamp +=
            (wio.inputs()[0].nitems_read() - d_tag_offset) * samp_rate;
    }
    tag_t tag = make_acq_info_tag(d_acq_info, wio.outputs()[0].nitems_written());
    wio.outputs()[0].add_tag(tag);
}

} // namespace gr::digitizers
