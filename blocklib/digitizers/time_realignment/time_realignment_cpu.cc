#include "time_realignment_cpu.h"
#include "status.h"
#include "time_realignment_cpu_gen.h"

namespace gr::digitizers {

#define NUMBER_OF_PENDING_TRIGGERS_WARNING 100
#define NUMBER_OF_PENDING_TRIGGERS_ERROR 1000

static constexpr auto BILLION = 1000000000;

time_realignment_cpu::time_realignment_cpu(const block_args &args)
    : INHERITED_CONSTRUCTORS
    , d_wr_events_size(10) // Maximum buffer of 10 WR-Events
{
    wr_event_t empty;
    for (std::size_t i = 0; i < d_wr_events_size; i++)
        d_wr_events.push_back(empty);
    d_wr_events_write_iter               = d_wr_events.begin();
    d_wr_events_read_iter                = d_wr_events.begin();
    d_not_found_stamp_utc                = 0;
    d_triggerstamp_matching_tolerance_ns = static_cast<int64_t>(args.triggerstamp_matching_tolerance * BILLION);
    d_max_buffer_time_ns                 = static_cast<int64_t>(args.max_buffer_time * BILLION);

    // FIXME: Currently time_realiggment does nothing !!
    set_tag_propagation_policy(tag_propagation_policy_t::TPP_ONE_TO_ONE);
}

work_return_t time_realignment_cpu::work(work_io &wio) {
    // TODO(PORT) I don't see how all this logic matches the 2..3/2 input/output stream configuration
    // FIXME: Currently time_realiggment does nothing !!
    std::size_t ninput_items_min;
    //     uint64_t sample_to_start_processing_abs;
    bool errors_connected = wio.inputs().size() > 1 && wio.outputs().size() > 1;
    if (errors_connected) {
        ninput_items_min = std::min(wio.inputs()[0].n_items, wio.inputs()[1].n_items);
        //          sample_to_start_processing_abs = std::min( nitems_read(0), nitems_read(1));
    } else {
        ninput_items_min = wio.inputs()[0].n_items;
        //         sample_to_start_processing_abs = nitems_read(0);
    }
    std::size_t copy_data_len = std::min(wio.outputs()[0].n_items, ninput_items_min);
    //      uint64_t max_sample_to_end_processing_abs = sample_to_start_processing_abs + copy_data_len;
    //
    //      // Get all the tags, for performance reason a member variable is used
    //      std::vector<gr::tag_t> tags;
    //      get_tags_in_range(tags, 0, sample_to_start_processing_abs, max_sample_to_end_processing_abs);
    //      for (auto tag: tags)
    //      {
    //        if (tag.key == pmt::string_to_symbol(trigger_tag_name))
    //        {
    //          //std::cout << "trigger tag incoming. Offset: " << tag.offset << std::endl;
    //          //std::cout << "Trigger Tag incoming : " << get_timestamp_milli_utc() << std::endl;
    //          trigger_t trigger_tag_data = decode_trigger_tag(tag);
    //          //std::cout << "Trigger Stamp        : " << trigger_tag_data.timestamp / 1000000 <<  " ms" << std::endl;
    //          if(fill_wr_stamp(trigger_tag_data))
    //          {
    //              add_item_tag(0, make_trigger_tag(trigger_tag_data,tag.offset)); // add tag to port 0
    //          }
    //          else
    //          {
    //              //std::cout << "tag.offset: " << tag.offset << std::endl;
    //              //std::cout << "sample_to_start_processing_abs: " << sample_to_start_processing_abs << std::endl;
    //
    //              // No WR-Stamp availabe yet. Keep data on the input queue and leave. Better luck on next iteration
    //              copy_data_len = tag.offset - sample_to_start_processing_abs - 1; // only copy all data before the tag
    //              if(copy_data_len <= 0) // nothing more to do
    //              {
    //                  consume(0, 0);
    //                  if (errors_connected)
    //                      consume(1, 0);
    //                  return 0;
    //              }
    //              //std::cout << "tag.offset: " << tag.offset << std::endl;
    //              //std::cout << "sample_to_start_processing_abs: " << sample_to_start_processing_abs << std::endl;
    //              break;
    //          }
    //        }
    //        else
    //        {
    //            //std::cout << "unknown tag incoming" << std::endl;
    //          add_item_tag(0, tag); // forward all others by default
    //        }
    //      }

    // std::cout << "memcpy: copy_data_len: " << copy_data_len << std::endl;

    // copy data
    memcpy(wio.outputs()[0].items<float>(), wio.inputs()[0].items<float>(), copy_data_len * sizeof(float));
    if (errors_connected)
        memcpy(wio.outputs()[1].items<float>(), wio.inputs()[1].items<float>(), copy_data_len * sizeof(float));

    // empty input queues
    wio.inputs()[0].consume(copy_data_len);
    if (errors_connected)
        wio.inputs()[1].consume(copy_data_len);

    wio.produce_each(copy_data_len);
    return work_return_t::OK;
}

bool time_realignment_cpu::add_timing_event(std::string event_id, int64_t wr_trigger_stamp, int64_t wr_trigger_stamp_utc) {
    d_wr_events_write_iter->event_id             = event_id;
    d_wr_events_write_iter->wr_trigger_stamp     = wr_trigger_stamp;
    d_wr_events_write_iter->wr_trigger_stamp_utc = wr_trigger_stamp_utc;
    d_wr_events_write_iter++;
    if (d_wr_events_write_iter == d_wr_events.end())
        d_wr_events_write_iter = d_wr_events.begin();
    if (d_wr_events_write_iter->wr_trigger_stamp == d_wr_events_read_iter->wr_trigger_stamp) {
        d_logger->error("{}: Write Iter reached read iter ...to few trigger tags", name());
        return false;
    }
    return true;

    // std::cout << "WR-Event Processing 5: " << get_timestamp_milli_utc() << std::endl;
}

void time_realignment_cpu::on_parameter_change(param_action_sptr action) {
    if (action->id() == id_triggerstamp_matching_tolerance) {
        d_triggerstamp_matching_tolerance_ns = static_cast<int64_t>(pmtf::get_as<float>(*this->param_triggerstamp_matching_tolerance) * BILLION);
    } else if (action->id() == id_max_buffer_time) {
        d_max_buffer_time_ns = static_cast<int64_t>(pmtf::get_as<float>(*this->param_max_buffer_time) * BILLION);
    }
}

bool time_realignment_cpu::fill_wr_stamp(trigger_t &trigger_tag_data) {
    // we dont have a wr-event for this trigger tag yet
    if (d_wr_events_write_iter->wr_trigger_stamp == d_wr_events_read_iter->wr_trigger_stamp) {
        if (d_not_found_stamp_utc == 0)
            d_not_found_stamp_utc = get_timestamp_nano_utc();

        // TODO(PORT) check that this is is correct (was: abs of unsigned (!) uint64_t)
        if (d_not_found_stamp_utc != 0 && std::abs(static_cast<int64_t>(get_timestamp_nano_utc()) - static_cast<int64_t>(d_not_found_stamp_utc)) > d_max_buffer_time_ns) {
            d_not_found_stamp_utc = 0; // reset stamp
            d_logger->error("{}: No WR-Tag found for trigger tag after waiting {}s. Trigger will be forwarded without realligment. Possibly max_buffer_time needs to be adjusted.", name(), max_buffer_time());
            trigger_tag_data.status |= channel_status_t::CHANNEL_STATUS_TIMEOUT_WAITING_WR_OR_REALIGNMENT_EVENT;
            return true;
        }

        // this may happen often.. possibly better to dont print a warning
        // GR_LOG_WARN(d_logger, "We dont have a wr-event for this trigger tag yet ... buffering will be used");
        return false; // all trigger and samples before this trigger will be kept on input, better luck on the next work call
    }

    while (true) {
        int64_t delta_t = abs(trigger_tag_data.timestamp - d_wr_events_read_iter->wr_trigger_stamp_utc);
        if (delta_t > d_triggerstamp_matching_tolerance_ns) {
            d_logger->warn("{}: WR Stamps was out of matching tolerance by {}µs. Will be ignored", name(), delta_t / 1000);
            d_logger->warn("{}: trigger_tag_data.timestamp                  {}ms", name(), trigger_tag_data.timestamp / 1000000);
            d_logger->warn("{}: d_wr_events_read_iter->wr_trigger_stamp_utc {}ms", name(), d_wr_events_read_iter->wr_trigger_stamp_utc / 1000000);
            //                {
            //                    time_t wrStampSeconds = trigger_tag_data.timestamp / 1000000000;
            //                    struct tm * timeinfo;
            //                    timeinfo = localtime (&wrStampSeconds);
            //                    printf ("trigger_tag_data.timestamp: %s", asctime(timeinfo));
            //                }
            //                {
            //                    time_t wrStampSeconds = d_wr_events_read_iter->wr_trigger_stamp_utc / 1000000000;
            //                    struct tm * timeinfo;
            //                    timeinfo = localtime (&wrStampSeconds);
            //                    printf ("d_wr_events_read_iter->wr_trigger_stamp_utc: %s", asctime(timeinfo));
            //                }
            trigger_tag_data.status |= channel_status_t::CHANNEL_STATUS_TIMEOUT_WAITING_WR_OR_REALIGNMENT_EVENT;
            d_wr_events_read_iter++;
            if (d_wr_events_read_iter == d_wr_events.end())
                d_wr_events_read_iter = d_wr_events.begin();
            if (d_wr_events_write_iter->wr_trigger_stamp == d_wr_events_read_iter->wr_trigger_stamp)
                return true; // Forward the trigger tag with bad status .. for some reason it did not match any of our wr-events
        } else {
            d_not_found_stamp_utc = 0; // reset stamp
            // std::cout << "delta_t [sec]                      : " << delta_t/1000000000.f << std::endl;
            trigger_tag_data.timestamp = d_wr_events_read_iter->wr_trigger_stamp;
            d_wr_events_read_iter++;
            if (d_wr_events_read_iter == d_wr_events.end())
                d_wr_events_read_iter = d_wr_events.begin();
            return true;
        }
    }
}

int64_t time_realignment_cpu::get_user_delay_ns() const {
    return static_cast<int64_t>(pmtf::get_as<float>(*this->param_user_delay) * BILLION);
}

} /* namespace gr::digitizers */
