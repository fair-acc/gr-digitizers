#include "interlock_generation_cpu.h"
#include "interlock_generation_cpu_gen.h"
#include "utils.h"

namespace gr::digitizers {

interlock_generation_cpu::interlock_generation_cpu(const block_args &args)
    : INHERITED_CONSTRUCTORS {
}

bool interlock_generation_cpu::start() {
    d_acq_info           = acq_info_t{};
    d_acq_info.timestamp = -1;
    return interlock_generation::start();
}

void interlock_generation_cpu::set_callback(std::function<void(int64_t, void*)> cb, void *user_data)
{
    d_callback = cb;
    d_user_data = user_data;
}

work_return_t interlock_generation_cpu::work(work_io &wio) {
    const auto in            = wio.inputs()[0].items<float>();
    const auto min           = wio.inputs()[1].items<float>();
    const auto max           = wio.inputs()[2].items<float>();

    auto       out           = wio.outputs()[0].items<float>();
    const auto noutput_items = wio.outputs()[0].n_items;
    const auto max_min       = pmtf::get_as<float>(*this->param_max_min);
    const auto max_max       = pmtf::get_as<float>(*this->param_max_max);

    for (std::size_t i = 0; i < noutput_items; i++) {
        // Get acq_info tags in range
        const auto tags      = filter_tags(wio.inputs()[0].tags_in_window(i, i + 1), acq_info_tag_name);

        bool       interlock = false;

        if (max[i] < max_max) {
            if (in[i] >= max[i]) {
                interlock = true;
            }
        }

        if (min[i] > max_min) {
            if (in[i] <= min[i]) {
                interlock = true;
            }
        }

        out[i] = interlock;

        if (d_interlock_issued && !interlock) {
            d_interlock_issued = false;
        } else if (!d_interlock_issued && interlock) {
            // calculate timestamp
            int64_t timestamp = -1;

            if (!tags.empty()) {
                d_acq_info = decode_acq_info_tag(tags.back());
                if (d_acq_info.timestamp != -1) {
                    timestamp = d_acq_info.timestamp + static_cast<int64_t>(((wio.inputs()[0].nitems_read() + static_cast<uint64_t>(i)) - tags.back().offset()) * d_acq_info.timebase * 1000000000.0);
                }
            }
            if (d_callback) {
                d_callback(timestamp, d_user_data);
            }
            d_interlock_issued = true;
        }
    }

    wio.produce_each(noutput_items);

    return work_return_t::OK;
}

} /* namespace gr::digitizers */
