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
      d_window_size(pre_trigger_window + post_trigger_window),
      d_history_size(d_window_size),
      d_pre_trigger_window_size(pre_trigger_window),
      d_post_trigger_window_size(post_trigger_window),
      d_sample_to_start_processing(pre_trigger_window)
    {
      // gnuradio is a bit strange here .. to get N history items, one needs to set the history to N+1
      set_history(d_history_size + 1);

      // allows us to send a complete data chunk down the stream
      set_output_multiple(d_window_size);

      set_tag_propagation_policy(TPP_DONT);
    }

    /*
     * Our virtual destructor.
     */
    demux_ff_impl::~demux_ff_impl()
    {
    }

    void
    demux_ff_impl::forecast(int noutput_items, gr_vector_int &ninput_items_required)
    {
        ninput_items_required[0] = d_history_size + noutput_items ;
//      for (auto & required : ninput_items_required)
//      {
//        required = d_history_size + noutput_items;
//      }
    }

    int
    demux_ff_impl::general_work(int noutput_items,
                               gr_vector_int &ninput_items,
                               gr_vector_const_void_star &input_items,
                               gr_vector_void_star &output_items)
    {
      unsigned nitems_produced = 0;

      // We start scanning at d_sample_to_start_processing. Everything before was either processed already in the last iteration.
      // We stop scanning at sample_to_stop_processing. Everything afterward will be processed in the next iteration (via history)
      // If a trigger would fall into the first or the last samples, in the current iteration we anyhow dont have enough samples build a full sample package (pre+post samples)
      const uint64_t new_items = ninput_items[0] - d_history_size;
      const uint64_t sample_to_stop_processing = nitems_read(0) + new_items - d_post_trigger_window_size;
      const uint64_t number_samples_to_process = sample_to_stop_processing - d_sample_to_start_processing;

//      std::cout << "ninput_items[0]: " << ninput_items[0] << std::endl;
//      std::cout << "new_items: " << new_items << std::endl;
//      std::cout << "d_sample_to_start_processing: " << d_sample_to_start_processing << std::endl;
//      std::cout << "sample_to_stop_processing: " << sample_to_stop_processing << std::endl;
      const float *input_values = static_cast<const float *>(input_items[0]);
      const float *input_errors = static_cast<const float *>(input_items[1]);

      float *output_values = static_cast<float *>(output_items[0]);
      float *output_errors = static_cast<float *>(output_items[1]);

//      for(unsigned int i=0;i< noutput_items; i++)
//      {
//          int index_total = nitems_read(0) + i - d_history_size;
//          if(index_total < 0)
//              index_total = -1;
//          std::cout << "input_value["<< i << "]("<<  index_total << "): " << input_values[i] << std::endl;
//      }

      std::vector<gr::tag_t> tags;
      std::vector<gr::tag_t> tags_in_window;
      get_tags_in_range(tags, 0, d_sample_to_start_processing, sample_to_stop_processing);
      for (const auto &tag : tags)
      {
        if(tag.key == pmt::string_to_symbol(trigger_tag_name))
        {
            // window_start is relative to the current input_items
            uint64_t window_start_abs = tag.offset - d_pre_trigger_window_size;
            uint64_t window_start_rel = window_start_abs - nitems_read(0) + d_history_size;

//            std::cout << "window_start_abs: " << window_start_abs << std::endl;
//            std::cout << "window_start_rel: " << window_start_rel << std::endl;
//            std::cout << "tag.offset: " << tag.offset << std::endl;
//            std::cout << "d_pre_trigger_window_size: " << d_pre_trigger_window_size << std::endl;
//            std::cout << "nitems_read(0): " << nitems_read(0) << std::endl;
//            std::cout << "d_history_size: " << d_history_size << std::endl;

            assert(window_start_abs >= nitems_read(0));
            assert(window_start_rel + d_window_size <=  uint64_t(ninput_items[0]));

            //copy values
            memcpy(&output_values[nitems_produced], &input_values[window_start_rel], d_window_size * sizeof(float));
            // copy errors, if connected
            if (input_items.size() > 1 && output_items.size() > 1)
              memcpy(&output_errors[nitems_produced], &input_errors[window_start_rel], d_window_size * sizeof(float));

            //copy all tags in the window
            get_tags_in_range(tags_in_window, 0, window_start_abs, window_start_abs + d_window_size);
            for (auto &tag_in_window : tags_in_window)
            {
                // re-adjust offset, according to produced items
                uint64_t tag_offset_relative_to_window = tag_in_window.offset - window_start_abs;
                tag_in_window.offset = nitems_written(0) + nitems_produced + tag_offset_relative_to_window;

                if(tag_in_window.key == pmt::string_to_symbol(trigger_tag_name))
                {
                    trigger_t trigger_tag_data = decode_trigger_tag(tag_in_window);
                    trigger_tag_data.pre_trigger_samples = d_pre_trigger_window_size;
                    trigger_tag_data.post_trigger_samples = d_post_trigger_window_size;
                    add_item_tag(0, make_trigger_tag(trigger_tag_data, tag_in_window.offset ));
                }
                else
                {
                    add_item_tag(0, tag_in_window);
                }
            }
            nitems_produced += d_window_size;
        }
      }
      d_sample_to_start_processing += number_samples_to_process;

      unsigned int items_consumed = ninput_items[0] - d_history_size;
      consume(0, items_consumed);
      // consume errors, if connected
      if (input_items.size() > 1 && output_items.size() > 1)
              consume(1, items_consumed);

      return nitems_produced;
    } /* general_work */
  } /* namespace digitizers */
} /* namespace gr */

