#include "edge_trigger_cpu.h"
#include "edge_trigger_cpu_gen.h"
#include "utils.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/tokenizer.hpp>

namespace gr::digitizers {

// TODO(PORT) this implementation isn't covered by the unit tests and thus
// untested

// To be on the safe side allocate big circular buffers
static constexpr size_t CIRC_BUFFER_SIZE      = 1024;
static constexpr size_t EDGE_CIRC_BUFFER_SIZE = 4096;

edge_trigger_cpu::edge_trigger_cpu(const block_args &args)
    : INHERITED_CONSTRUCTORS
    , d_actual_state(args.initial_state >= args.hi)
    , d_wr_events(CIRC_BUFFER_SIZE)
    , d_triggers(CIRC_BUFFER_SIZE)
    , d_detected_edges(EDGE_CIRC_BUFFER_SIZE) {
    // parse receiving host names and ports
    std::vector<std::string>                      hosts;
    boost::tokenizer<boost::char_separator<char>> tokens(args.host_list, boost::char_separator<char>(", "));
    for (auto &t : tokens) {
        std::string host = t;
        host.erase(std::remove(host.begin(), host.end(), '"'), host.end());
        hosts.push_back(host);
    }

    for (const auto &host : hosts) {
        std::vector<std::string> parts;
        boost::algorithm::split(parts, host, [](char c) { return c == ':'; });

        auto new_client = std::make_shared<udp_sender>(d_io_service, parts.at(0), parts.at(1));
        d_receivers.push_back(new_client);
#ifdef PORT_DISABLED // TODO(PORT) logging
        GR_LOG_DEBUG(d_logger, "edge_trigger_ff::registered host: '" + new_client->host_and_port() + "'");
#endif
    }

    set_tag_propagation_policy(tag_propagation_policy_t::TPP_DONT);
}

bool edge_trigger_cpu::start() {
    d_wr_events.clear();
    d_triggers.clear();
    d_detected_edges.clear();

    return true;
}

work_return_t edge_trigger_cpu::work(work_io &wio) {
    const auto in                       = wio.inputs()[0].items<float>();

    const auto noutput_items            = wio.outputs()[0].n_items;

    auto       count0                   = wio.inputs()[0].nitems_read();

    const auto lo_threshold             = pmtf::get_as<float>(*this->param_lo);
    const auto hi_threshold             = pmtf::get_as<float>(*this->param_hi);
    const auto send_udp_on_raising_edge = pmtf::get_as<bool>(*this->param_send_up_on_raising_edge);
    const auto timeout_samples          = pmtf::get_as<float>(*this->param_samp_rate) * pmtf::get_as<float>(*this->param_timeout);
    const auto all_tags                 = wio.inputs()[0].tags_in_window(0, noutput_items);
    // Consume all the WR events
    const auto events = filter_tags(std::vector<tag_t>(all_tags), wr_event_tag_name);

    for (const auto &tag : events) {
        d_wr_events.push_back(decode_wr_event_tag(tag));
    }

    // Detect all triggers
    const auto triggers = filter_tags(std::vector<tag_t>(all_tags), trigger_tag_name);
    for (const auto &tag : triggers) {
        d_triggers.push_back(tag.offset());
    }

    // Acq info tags are only needed to get info about the user delay
    const auto acq_tags = filter_tags(std::vector<tag_t>(all_tags), acq_info_tag_name);
    if (!acq_tags.empty()) {
        d_acq_info = decode_acq_info_tag(acq_tags.back());
    }

    const bool outputing = !wio.outputs().empty();

    // Do signal processing
    float *out = nullptr;
    if (outputing) {
        out = wio.outputs()[0].items<float>();
    }

    for (std::size_t i = 0; i < noutput_items; i++) {
        bool edge_detected = false;

        if (d_actual_state == false) {
            // actual_state is '0'
            if (in[i] > hi_threshold) {
                edge_detected  = send_udp_on_raising_edge;
                d_actual_state = true;
            }
        } else {
            // actual_state is '1'
            if (in[i] < lo_threshold) {
                edge_detected  = !send_udp_on_raising_edge;
                d_actual_state = false;
            }
        }

        if (outputing) {
            out[i] = static_cast<float>(d_actual_state);
        }

        if (edge_detected) {
            d_detected_edges.push_back((count0 + i));
        }
    }

    // Trigger detection logic
    size_t triggers_consumed = 0;

    for (const auto trigger : d_triggers) {
        // To detect timeout
        const auto samples_since_trigger = count0 <= trigger ? 0 : count0 - trigger;

        // It is assumed that first WR event belongs to the first trigger. We cannot really
        // detect sporadic or misaligned events.
        if (d_wr_events.empty()) {
            if (samples_since_trigger > timeout_samples) {
#ifdef PORT_DISABLED // TODO(PORT) logging
                GR_LOG_ERROR(d_logger, "Timeout receiving WR event for trigger at offset: "
                                               + std::to_string(trigger));
#endif
                triggers_consumed++;
                continue;
            }

            break; // wait another work iteration to receive WR event
        }

        // const auto &wr_event = d_wr_events.front();

        // Edge detection is done only for events that require realignment
        //   if (!wr_event.realignment_required) {
        d_wr_events.pop_front();
        triggers_consumed++;
        //   }
        //        else {
        //          // find first relevant detected edge
        //          bool detected = false;
        //          uint64_t detected_edge = 0;
        //
        //          while (!d_detected_edges.empty()) {
        //            detected_edge = d_detected_edges.front();
        //            d_detected_edges.pop_front();
        //
        //            if (detected_edge >= trigger) {
        //              detected = true;
        //              break;
        //            }
        //          }

        //          if (detected) {
        //            send_edge_detect_info(trigger, detected_edge, wr_event, !output_items.empty());
        //            d_wr_events.pop_front();
        //            triggers_consumed++;
        //          }
        //          else if (samples_since_trigger > d_timeout_samples) {
        //            GR_LOG_ERROR(d_logger, "Timeout detecting edge for trigger at offset: "
        //                    + std::to_string(trigger));
        //            triggers_consumed++;
        //          }
        //        }
    }

    for (size_t i = 0; i < triggers_consumed; i++) {
        d_triggers.pop_front();
    }

    // Tell runtime system how many input items we consumed on
    // each input stream.
    wio.consume_each(noutput_items);
    wio.produce_each(noutput_items);

    return work_return_t::OK;
}

} /* namespace gr::digitizers */
