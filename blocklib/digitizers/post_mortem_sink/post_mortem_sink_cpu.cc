/* -*- c++ -*- */
#include "post_mortem_sink_cpu.h"
#include "post_mortem_sink_cpu_gen.h"

#include "utils.h"

namespace gr::digitizers {

post_mortem_sink_cpu::post_mortem_sink_cpu(const block_args &args)
    : INHERITED_CONSTRUCTORS
    , d_buffer_values(args.buffer_size)
    , d_buffer_errors(args.buffer_size)
    , d_acq_info{ .timestamp = -1 }
    , d_metadata{ .unit = args.unit, .name = args.name } {
}

work_return_t post_mortem_sink_cpu::work(work_io &wio) {
    std::scoped_lock lock{ d_mutex };

    auto             ninput_items   = wio.inputs()[0].n_items;

    const auto       reading_errors = wio.inputs().size() > 1;
    const auto       buffer_size    = pmtf::get_as<std::size_t>(*this->param_buffer_size);

    // Data is frozen, copy data to outputs and return
    if (d_frozen) {
        if (wio.outputs().size() >= 1) {
            memcpy(wio.outputs()[0].items<float>(), wio.inputs()[0].items<float>(), ninput_items * sizeof(float));
        }
        if (reading_errors && wio.outputs().size() == 2) {
            memcpy(wio.outputs()[1].items<float>(), wio.inputs()[1].items<float>(), ninput_items * sizeof(float));
        }

        wio.consume_each(ninput_items); // TODO(PORT) consume_each not in baseline
        wio.produce_each(ninput_items);
        d_nitems_read = wio.inputs()[0].nitems_read() + ninput_items;

        return work_return_t::OK;
    }

    // For simplicity perform only a single memcpy at once
    assert(d_write_index < buffer_size);
    ninput_items = std::min(ninput_items, buffer_size - d_write_index);

    memcpy(&d_buffer_values[d_write_index], wio.inputs()[0].items<float>(), ninput_items * sizeof(float));
    if (wio.outputs().size() > 1) {
        memcpy(wio.outputs()[0].items<float>(), wio.inputs()[0].items<float>(), ninput_items * sizeof(float));
    }
    if (reading_errors) {
        memcpy(&d_buffer_errors[d_write_index], wio.inputs()[1].items<float>(), ninput_items * sizeof(float));
        if (wio.outputs().size() == 2) {
            memcpy(wio.outputs()[1].items<float>(), wio.inputs()[1].items<float>(), ninput_items * sizeof(float));
        }
    }

    // Update write index and wrap around if needed
    d_write_index += ninput_items;
    if (d_write_index == buffer_size) {
        d_write_index = 0;
    }

    // Acquisition info, store the last one
    const auto tags = filter_tags(wio.inputs()[0].tags_in_window(0, ninput_items), acq_info_tag_name);
    if (!tags.empty()) {
        const auto tag    = tags.at(tags.size() - 1);
        d_acq_info        = decode_acq_info_tag(tag);
        d_acq_info_offset = tag.offset();
    }

    wio.consume_each(ninput_items); // TODO(PORT) consume_each not in baseline
    wio.produce_each(ninput_items);
    d_nitems_read = wio.inputs()[0].nitems_read() + ninput_items;

    return work_return_t::OK;
}

signal_metadata_t post_mortem_sink_cpu::get_metadata() const {
    // TODO(PORT) do we really need this? why not make name/unit gettable? (and remove the lock)
    std::scoped_lock lock{ d_mutex };
    return d_metadata;
}

void post_mortem_sink_cpu::freeze_buffer() {
    std::scoped_lock lock{ d_mutex };
    d_frozen = true;
}

post_mortem_data_t post_mortem_sink_cpu::get_post_mortem_data(std::size_t nr_items_to_read) {
    post_mortem_data_t ret;

    std::scoped_lock   lock(d_mutex);

    const auto         buffer_size = pmtf::get_as<std::size_t>(*this->param_buffer_size);

    if (d_nitems_read < buffer_size) {
        nr_items_to_read = std::min(nr_items_to_read, d_nitems_read);
    } else {
        nr_items_to_read = std::min(nr_items_to_read, buffer_size);
    }

    ret.values.resize(nr_items_to_read);
    ret.errors.resize(nr_items_to_read);

    auto transfered = size_t{ 0 };
    auto index      = static_cast<int64_t>(d_write_index) - static_cast<int64_t>(nr_items_to_read);

    if (index < 0) {
        auto from = static_cast<int64_t>(buffer_size) + index;
        auto size = index * (-1);
        memcpy(ret.values.data(), &d_buffer_values[from], size * sizeof(float));
        memcpy(ret.errors.data(), &d_buffer_errors[from], size * sizeof(float));
        transfered += size;
    }

    auto size = nr_items_to_read - transfered;
    auto from = d_write_index - size;
    memcpy(&ret.values[transfered], &d_buffer_values[from], size * sizeof(float));
    memcpy(&ret.errors[transfered], &d_buffer_errors[from], size * sizeof(float));

    // For now we simply copy over the last status
    ret.info.timebase     = d_acq_info.timebase;
    ret.info.user_delay   = d_acq_info.user_delay;
    ret.info.actual_delay = d_acq_info.actual_delay;
    ret.info.status       = d_acq_info.status;

    // Calculate timestamp
    if (d_acq_info.timestamp < 0) {
        ret.info.timestamp = -1; // timestamp is invalid
    } else {
        assert(nr_items_to_read <= d_nitems_read);
        auto offset_first_sample = d_nitems_read - nr_items_to_read;
        if (offset_first_sample >= d_acq_info_offset) {
            auto delta         = d_acq_info.timebase * (offset_first_sample - d_acq_info_offset) * 1000000000.0;
            ret.info.timestamp = d_acq_info.timestamp + static_cast<uint64_t>(delta);
        } else {
            auto delta         = d_acq_info.timebase * (d_acq_info_offset - offset_first_sample) * 1000000000.0;
            ret.info.timestamp = d_acq_info.timestamp - static_cast<uint64_t>(delta);
        }
    }

    d_frozen = false;

    return ret;
}

} // namespace gr::digitizers
