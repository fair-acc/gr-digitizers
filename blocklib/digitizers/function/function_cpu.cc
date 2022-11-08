#include "function_cpu.h"
#include "function_cpu_gen.h"

namespace gr::digitizers {

function_cpu::function_cpu(block_args args)
    : INHERITED_CONSTRUCTORS
    , d_ref(args.ref)
    , d_min(args.min)
    , d_max(args.max) {
    set_tag_propagation_policy(tag_propagation_policy_t::TPP_DONT);

    if (args.min.empty() || args.min.size() != args.ref.size() || args.max.size() != args.ref.size() || (args.timing.size() < (args.ref.size() - 1))) {
        throw std::runtime_error("invalid function provided, length mismatch");
    }

    // convert seconds to nanoseconds
    d_timing.reserve(args.timing.size());
    for (const auto t : args.timing) {
        d_timing.push_back(static_cast<int64_t>(t * 1000000000.0));
    }
}

bool function_cpu::start() {
    // reset state
    d_acq_info.timestamp = -1;

    return true;
}

work_return_t function_cpu::work(work_io &wio) {
    const auto decimation   = pmtf::get_as<std::size_t>(*this->param_decimation);

    const auto ninput_items = wio.inputs()[0].n_items;

    if (ninput_items < decimation) {
        return work_return_t::INSUFFICIENT_INPUT_ITEMS;
    }

    const auto min_noutput_items = std::min({ wio.outputs()[0].n_items, wio.outputs()[1].n_items, wio.outputs()[2].n_items });

    if (min_noutput_items == 0) {
        return work_return_t::INSUFFICIENT_OUTPUT_ITEMS;
    }

    const auto noutput_items = std::min(min_noutput_items, ninput_items / decimation);

    auto       out_ref       = wio.outputs()[0].items<float>();
    auto       out_min       = wio.outputs()[1].items<float>();
    auto       out_max       = wio.outputs()[2].items<float>();

    if (d_ref.size() == 1) {
        for (std::size_t i = 0; i < noutput_items; i++) {
            out_ref[i] = d_ref[0];
            out_min[i] = d_min[0];
            out_max[i] = d_max[0];
        }
    } else {
        const auto samp0_count = wio.inputs()[0].nitems_read();

        for (std::size_t i = 0; i < noutput_items; i++) {
            auto input_idx = i * decimation;

            // Get acq_info tags in range
            auto tags     = filter_tags(wio.inputs()[0].tags_in_window(input_idx, input_idx + decimation), acq_info_tag_name);

            auto acq_info = d_acq_info;

            if (!tags.empty()) {
                // use the first tag, save the last one
                if (tags.at(0).offset() == (samp0_count + input_idx)) {
                    acq_info = decode_acq_info_tag(tags.at(0));
                }

                d_acq_info = decode_acq_info_tag(tags.back());
            }

            // no timing
            if (acq_info.timestamp == -1) {
                out_ref[i] = d_ref[0];
                out_min[i] = d_min[0];
                out_max[i] = d_max[0];
            } else {
                d_logger->error("function_ff_impl needs to be re-implemented, there is no 'last_beam_in_timestamp' any more in acq_info");

                /*
                // timestamp of the first sample in working range
                auto timestamp = acq_info.timestamp + static_cast<int64_t>(((samp0_count + input_idx) - tags.at(0).offset) * acq_info.timebase * 1000000000.0);

                if (timestamp < acq_info.last_beam_in_timestamp)
                {
                  out_ref[i] = d_ref[0];
                  out_min[i] = d_min[0];
                  out_max[i] = d_max[0];
                }
                else
                {
                  auto distance_from_beam_in = timestamp - acq_info.last_beam_in_timestamp;

                  // find left and right index
                  auto it = std::find_if(d_timing.begin(), d_timing.end(),
                      [distance_from_beam_in] (const int64_t t)
                  {
                      return t >= distance_from_beam_in;
                  });

                  auto idx_r = static_cast<int>(std::distance(d_timing.begin(), it));

                  if (idx_r == static_cast<int>(d_timing.size())) {
                    out_ref[i] = d_ref[idx_r - 1];
                    out_min[i] = d_min[idx_r - 1];
                    out_max[i] = d_max[idx_r - 1];
                  }
                  else if (idx_r == 0) {
                    out_ref[i] = d_ref[0];
                    out_min[i] = d_min[0];
                    out_max[i] = d_max[0];
                  }
                  else {
                    auto idx_l = idx_r - 1;

                    // perform linear interpolation
                    auto k = static_cast<float>(distance_from_beam_in - d_timing[idx_l])
                              / static_cast<float>(d_timing[idx_r] - d_timing[idx_l]);

                    out_ref[i] = d_ref[idx_l] + (d_ref[idx_r] - d_ref[idx_l]) * k;
                    out_min[i] = d_min[idx_l] + (d_min[idx_r] - d_min[idx_l]) * k;
                    out_max[i] = d_max[idx_l] + (d_max[idx_r] - d_max[idx_l]) * k;
                  }
                }*/
            }
        }
    }

    wio.consume_each(noutput_items * decimation);
    wio.produce_each(noutput_items);

    return work_return_t::OK;
}

} // namespace gr::digitizers
