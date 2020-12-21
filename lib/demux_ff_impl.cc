/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include <digitizers/status.h>
#include "demux_ff_impl.h"

namespace gr {
  namespace digitizers {

    demux_ff::sptr
    demux_ff::make(unsigned post_trigger_window, unsigned pre_trigger_window)
    {
      return gnuradio::get_initial_sptr
        (new demux_ff_impl(post_trigger_window, pre_trigger_window));
    }

    /*
     * The private constructor
     */
    demux_ff_impl::demux_ff_impl(unsigned post_trigger_window, unsigned pre_trigger_window)
      : gr::block("demux_ff",
              gr::io_signature::make(1, 2, sizeof(float)),
              gr::io_signature::make(1, 2, sizeof(float))),
      d_my_history(pre_trigger_window + post_trigger_window),
      d_pre_trigger_window(pre_trigger_window),
      d_post_trigger_window(post_trigger_window),
      d_state(extractor_state::WaitTrigger),
      d_last_trigger_offset(0),
      d_trigger_start_range(0),
      d_trigger_end_range(0)
    {
      // actual history size is in fact N - 1
      set_history(d_my_history + 1);

      // allows us to send a complete data chunk down the stream
      set_output_multiple(pre_trigger_window + post_trigger_window);

      set_tag_propagation_policy(TPP_DONT);
    }

    /*
     * Our virtual destructor.
     */
    demux_ff_impl::~demux_ff_impl()
    {
    }

    bool
    demux_ff_impl::start()
    {
      d_state = extractor_state::WaitTrigger;
      return true;
    }

//    void
//    demux_ff_impl::forecast(int noutput_items, gr_vector_int &ninput_items_required)
//    {
//      for (auto & required : ninput_items_required) {
//        required = noutput_items + history() - 1;
//      }
//    }

    int
    demux_ff_impl::general_work(int noutput_items,
                               gr_vector_int &ninput_items,
                               gr_vector_const_void_star &input_items,
                               gr_vector_void_star &output_items)
    {
      int retval = 0;
      const auto samp0_count = nitems_read(0);

      // Consume samples until a trigger tag is detected
      if (d_state == extractor_state::WaitTrigger)
      {
        std::vector<gr::tag_t> trigger_tags;
        get_tags_in_range(trigger_tags, 0, samp0_count, samp0_count + noutput_items, pmt::string_to_symbol(trigger_tag_name));

        for (const auto &trigger_tag : trigger_tags)
        {
          if (trigger_tag.offset > d_pre_trigger_window)
          {
            d_last_trigger_offset = trigger_tag.offset;
            d_trigger_tag_data = decode_trigger_tag(trigger_tag);
            d_state = extractor_state::CalcOutputRange;
            //GR_LOG_DEBUG(d_logger, "demux detected trigger at offset: " + std::to_string(d_last_trigger_offset));

            // store as well aqc info tags for that trigger
            std::vector<gr::tag_t> info_tags;
            get_tags_in_range(info_tags, 0, trigger_tag.offset - d_pre_trigger_window, trigger_tag.offset + d_post_trigger_window, pmt::string_to_symbol(acq_info_tag_name));
            for (const auto &info_tag : info_tags)
            {
              int64_t rel_offset = info_tag.offset - trigger_tag.offset;
              d_acq_info_tags.push_back(std::make_pair(decode_acq_info_tag(info_tag), rel_offset ));
            }
            break; // found a trigger
            //TODO: Instead of breaking here, we should make to report a warning/error if a trigger got skipped (e.g. if triggers are too tight together)
          }
        }
      }

      if (d_state == extractor_state::CalcOutputRange) {

        // relative offset of the first sample based on count0
        int relative_trigger_offset =
                - static_cast<int>(samp0_count - d_last_trigger_offset)
                - d_pre_trigger_window;

        if (relative_trigger_offset < (-static_cast<int>(d_my_history))) {
          GR_LOG_ERROR(d_logger, "Can't extract data, not enough history available");

          d_state = extractor_state::WaitTrigger;

          consume_each(noutput_items);
          return 0;
        }

        d_trigger_start_range = nitems_read(0) + relative_trigger_offset;
        d_trigger_end_range = d_trigger_start_range + (d_pre_trigger_window + d_post_trigger_window);

        d_state = extractor_state::WaitAllData;
      }

      if (d_state == extractor_state::WaitAllData) {
        if (samp0_count < d_trigger_end_range) {
          noutput_items = std::min(static_cast<int>(d_trigger_end_range - samp0_count), noutput_items);
        }
        else {
          noutput_items = 0; // Don't consume anything in this iteration, output triggered data first
          d_state = extractor_state::OutputData;
        }
      }

      if (d_state == extractor_state::OutputData) {

        assert (samp0_count >= d_trigger_end_range);
        auto samples_2_copy = d_pre_trigger_window + d_post_trigger_window;
        assert(d_my_history > (samp0_count - d_trigger_end_range));
        auto start_index = d_my_history - (samp0_count - d_trigger_end_range) - samples_2_copy;
        assert(start_index >= 0 && start_index < (d_my_history + noutput_items));
        memcpy((char *)output_items.at(0),
               (char *)input_items.at(0) + start_index * sizeof(float),
               samples_2_copy * sizeof(float));
        if (input_items.size() > 1 && output_items.size() > 1) {
          memcpy((char *)output_items.at(1),
                 (char *)input_items.at(1) + start_index * sizeof(float),
                 samples_2_copy * sizeof(float));
        }

        retval = samples_2_copy;

        // add trigger tag
        add_item_tag(0, make_trigger_tag(d_trigger_tag_data, nitems_written(0) + d_pre_trigger_window ));
        for (auto &info_tag : d_acq_info_tags)
        {
            int64_t rel_offset = info_tag.second;
            add_item_tag(0, make_acq_info_tag(info_tag.first, nitems_written(0) + d_pre_trigger_window + rel_offset ));
        }
        d_acq_info_tags.clear();

        d_state = extractor_state::WaitTrigger;
      }

      consume_each(noutput_items);
      return retval;
    }

  } /* namespace digitizers */
} /* namespace gr */

