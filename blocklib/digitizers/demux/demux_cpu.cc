#include "demux_cpu.h"
#include "demux_cpu_gen.h"

namespace gr::digitizers {

demux_cpu::demux_cpu(const block_args& args) : INHERITED_CONSTRUCTORS
{
#ifdef PORT_DISABLED // check how/if to port this (d_my_history = args.pre_trigger_window
                     // + args.post_trigger_window)
    // actual history size is in fact N - 1
    set_history(d_my_history + 1);
#endif
    // TODO(PORT) I tried to replace the built-in history by checking the data available
    // and return INSUFFICIENT_INPUT_ITEMS until enough data is there (without consuming),
    // but that doesn't seems enough to let the runtime give us enough data eventually.
    // Consumed data can be overwritten (ringbuffer), so I'm not sure how to implement
    // this without copying the data into our own buffer.

    // allows us to send a complete data chunk down the stream
    set_output_multiple(args.pre_trigger_window + args.post_trigger_window);
    set_tag_propagation_policy(tag_propagation_policy_t::TPP_DONT);
}

bool demux_cpu::start()
{
    d_state = extractor_state::WaitTrigger;
    return true;
}

work_return_t demux_cpu::work(work_io& wio)
{
    int retval = 0;
    const auto samp0_count = wio.inputs()[0].nitems_read();

    const auto post_trigger_window =
        std::get<std::size_t>(*this->param_post_trigger_window);
    const auto pre_trigger_window =
        std::get<std::size_t>(*this->param_pre_trigger_window);

    auto noutput_items = wio.outputs()[0].n_items;

    // Consume samples until a trigger tag is detected
    if (d_state == extractor_state::WaitTrigger) {
        std::vector<gr::tag_t> trigger_tags;
#ifdef PORT_DISABLED // TODO(PORT) tag handling
        get_tags_in_range(trigger_tags,
                          0,
                          samp0_count,
                          samp0_count + noutput_items,
                          pmt::string_to_symbol(trigger_tag_name));
#endif
        for (const auto& trigger_tag : trigger_tags) {
            if (trigger_tag.offset() > pre_trigger_window) {
                d_last_trigger_offset = trigger_tag.offset();
                d_trigger_tag_data = decode_trigger_tag(trigger_tag);
                d_state = extractor_state::CalcOutputRange;
                // GR_LOG_DEBUG(d_logger, "demux detected trigger at offset: " +
                // std::to_string(d_last_trigger_offset));

                // store as well aqc info tags for that trigger
                const auto offs =
                    static_cast<int64_t>(trigger_tag.offset()) - samp0_count;
                const auto info_tags = filter_tags(
                    wio.inputs()[0].tags_in_window(offs - pre_trigger_window,
                                                   offs + post_trigger_window),
                    acq_info_tag_name);

                for (const auto& info_tag : info_tags) {
                    int64_t rel_offset = info_tag.offset() - trigger_tag.offset();
                    d_acq_info_tags.push_back(
                        { decode_acq_info_tag(info_tag), rel_offset });
                }
                break; // found a trigger
                       // TODO: Instead of breaking here, we should make to report a
                       // warning/error if a trigger got skipped (e.g. if triggers are too
                       // tight together)
            }
        }
    }

    if (d_state == extractor_state::CalcOutputRange) {
        // relative offset of the first sample based on count0
        int relative_trigger_offset =
            -static_cast<int>(samp0_count - d_last_trigger_offset) - pre_trigger_window;

        if (relative_trigger_offset < (-static_cast<int>(d_my_history))) {
            d_logger->error("Can't extract data, not enough history available");
            d_state = extractor_state::WaitTrigger;

            wio.consume_each(noutput_items);
            return work_return_t::OK; // TODO correct code?
        }

        d_trigger_start_range = wio.inputs()[0].nitems_read() + relative_trigger_offset;
        d_trigger_end_range =
            d_trigger_start_range + (pre_trigger_window + post_trigger_window);

        d_state = extractor_state::WaitAllData;
    }

    if (d_state == extractor_state::WaitAllData) {
        if (samp0_count < d_trigger_end_range) {
            noutput_items = std::min(d_trigger_end_range - samp0_count, noutput_items);
        }
        else {
            noutput_items = 0; // Don't consume anything in this iteration, output
                               // triggered data first
            d_state = extractor_state::OutputData;
        }
    }

    if (d_state == extractor_state::OutputData) {
        assert(samp0_count >= d_trigger_end_range);
        auto samples_2_copy = pre_trigger_window + post_trigger_window;
        assert(d_my_history > (samp0_count - d_trigger_end_range));
        auto start_index =
            d_my_history - (samp0_count - d_trigger_end_range) - samples_2_copy;
        assert(start_index >= 0 && start_index < (d_my_history + noutput_items));
        memcpy(wio.outputs()[0].items<char>(),
               wio.inputs()[0].items<char>() + start_index * sizeof(float),
               samples_2_copy * sizeof(float));
        if (wio.inputs().size() > 1 && wio.outputs().size() > 1) {
            memcpy(wio.outputs()[1].items<char>(),
                   wio.inputs()[1].items<char>() + start_index * sizeof(float),
                   samples_2_copy * sizeof(float));
        }

        retval = samples_2_copy;

        // add trigger tag
        auto tag = make_trigger_tag(
            d_trigger_tag_data, wio.outputs()[0].nitems_written() + pre_trigger_window);
        wio.outputs()[0].add_tag(tag);

        for (auto& info_tag : d_acq_info_tags) {
            int64_t rel_offset = info_tag.second;
            auto tag = make_acq_info_tag(info_tag.first,
                                         wio.outputs()[0].nitems_written() +
                                             pre_trigger_window + rel_offset);
            wio.outputs()[0].add_tag(tag);
        }

        d_acq_info_tags.clear();

        d_state = extractor_state::WaitTrigger;
    }

    wio.consume_each(noutput_items);
    wio.produce_each(retval);

    return work_return_t::OK;
}

} /* namespace gr::digitizers */
