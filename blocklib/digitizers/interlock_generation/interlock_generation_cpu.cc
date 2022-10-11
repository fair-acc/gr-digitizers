#include "interlock_generation_cpu.h"
#include "interlock_generation_cpu_gen.h"

namespace gr::digitizers {

template<class T>
interlock_generation_cpu<T>::interlock_generation_cpu(const typename interlock_generation<T>::block_args &args)
    : INHERITED_CONSTRUCTORS(T) {
}

template<class T>
bool interlock_generation_cpu<T>::start() {
    d_acq_info           = acq_info_t{};
    d_acq_info.timestamp = -1;
    return interlock_generation<T>::start();
}

template<class T>
work_return_t interlock_generation_cpu<T>::work(work_io &wio) {
    static_assert(std::is_same<T, float>());
    const auto in            = wio.inputs()[0].items<float>();
    const auto min           = wio.inputs()[1].items<float>();
    const auto max           = wio.inputs()[2].items<float>();

    auto       out           = wio.outputs()[0].items<float>();
    const auto noutput_items = wio.outputs()[0].n_items;
    const auto max_min       = pmtf::get_as<float>(*this->param_max_min);
    const auto max_max       = pmtf::get_as<float>(*this->param_max_max);

    for (std::size_t i = 0; i < noutput_items; i++) {
        // Get acq_info tags in range
        std::vector<gr::tag_t> tags;
#ifdef PORT_DISABLED // TODO(PORT) can't find replacement for get_tags_in_window
        get_tags_in_window(tags, 0, i, i + 1, acq_info_tag_name);
#endif
        bool interlock = false;

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
#ifdef PORT_DISABLED // TODO(PORT) port callback
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
#endif
            d_interlock_issued = true;
        }
    }

    wio.produce_each(noutput_items);

    return work_return_t::OK;
}

} /* namespace gr::digitizers */
