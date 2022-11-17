#include "signal_averager_cpu.h"
#include "signal_averager_cpu_gen.h"

#include "tags.h"

namespace gr::digitizers {

signal_averager_cpu::signal_averager_cpu(block_args args)
    : INHERITED_CONSTRUCTORS {
    set_tag_propagation_policy(tag_propagation_policy_t::TPP_CUSTOM);
    set_relative_rate(1. / args.window_size); // TODO(PORT) This doesn't seem to have any effect?
}

work_return_t signal_averager_cpu::work(work_io &wio) {
    const auto decim         = pmtf::get_as<std::size_t>(*this->param_window_size);

    auto       noutput_items = std::numeric_limits<std::size_t>::max();
    for (auto &w : wio.inputs()) {
        noutput_items = std::min(noutput_items, w.n_items / decim);
    }
    if (noutput_items == 0) {
        return work_return_t::INSUFFICIENT_INPUT_ITEMS;
    }
    for (auto &w : wio.outputs()) {
        noutput_items = std::min(noutput_items, w.n_items);
    }

    for (std::size_t port = 0; port < wio.outputs().size(); port++) {
        auto        out  = wio.outputs()[port].items<float>();
        auto        in   = wio.inputs()[port].items<float>();
        std::size_t i_in = 0, i_out = 0;
        for (i_out = 0; i_out < noutput_items; i_out++) {
            float sum = 0.0f;
            for (unsigned temp = 0; temp < decim; temp++)
                sum += in[i_in + temp];

            out[i_out] = sum / static_cast<float>(decim);

            auto tags  = wio.inputs()[port].tags_in_window(i_in, i_in + decim);

            // TODO(PORT) does the comment below still apply to GR 4.0?
            // required to merge acq_infotags due to this bug: https://github.com/gnuradio/gnuradio/issues/2364
            // otherwise we will eat to much memory
            // TODO: Fix bug in gnuradio (https://gitlab.com/al.schwinn/gr-digitizers/issues/33)
            acq_info_t merged_acq_info;
            merged_acq_info.status = 0;
            bool found_acq_info    = false;
            for (auto tag : tags) {
                // std::cout << "tag found: " << tag.key << std::endl;
                // TODO(PORT) this assumes that all tags have no more than one key/value pair
                assert(tag.map().size() == 1);
                if (tag.get(acq_info_tag_name)) {
                    found_acq_info = true;
                    merged_acq_info.status |= decode_acq_info_tag(tag).status;
                } else {
                    tag.set_offset(wio.outputs()[port].nitems_written() + i_out);
                    wio.outputs()[port].add_tag(tag);
                }
            }

            if (found_acq_info) {
                auto new_tag = make_acq_info_tag(merged_acq_info, wio.outputs()[port].nitems_written() + i_out);
                wio.outputs()[port].add_tag(new_tag);
            }

            i_in += decim;
        }
    }

    wio.consume_each(noutput_items * decim);
    wio.produce_each(noutput_items);
    return work_return_t::OK;
}

} // namespace gr::digitizers
