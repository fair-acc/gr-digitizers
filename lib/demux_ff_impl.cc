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
#include <algorithm>    // std::max
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
      d_pre_trigger_window_size(pre_trigger_window),
      d_post_trigger_window_size(post_trigger_window)
    {
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
        //ninput_items_required[0] = noutput_items ;
        //ninput_items_required[1] = noutput_items ;
      for (auto & required : ninput_items_required)
      {
        required = noutput_items;
      }
    }

    int
    demux_ff_impl::general_work(int noutput_items,
                               gr_vector_int &ninput_items,
                               gr_vector_const_void_star &input_items,
                               gr_vector_void_star &output_items)
    {
      unsigned nitems_produced = 0;
      uint64_t ninput_items_min;
      uint64_t sample_to_start_processing_abs;

      if (input_items.size() > 1 && output_items.size() > 1)
      {
          ninput_items_min = std::min( ninput_items[0], ninput_items[1]);
          sample_to_start_processing_abs = std::min( nitems_read(0), nitems_read(1));
      }
      else
      {
          ninput_items_min = ninput_items[0];
          sample_to_start_processing_abs = nitems_read(0);
      }

      const uint64_t sample_to_stop_processing_abs = sample_to_start_processing_abs + ninput_items_min;
//      std::cout << "#####################################" << std::endl;
//      std::cout << "ninput_items_min: " << ninput_items_min << std::endl;
//      std::cout << "nitems_read_min: " << nitems_read_min << std::endl;
//      std::cout << "sample_to_start_processing_abs: " << sample_to_start_processing_abs << std::endl;
//      std::cout << "sample_to_stop_processing_abs: " << sample_to_stop_processing_abs << std::endl;
      const float *input_values = reinterpret_cast<const float *>(input_items[0]);
      const float *input_errors = reinterpret_cast<const float *>(input_items[1]);
      float *output_values = reinterpret_cast<float *>(output_items[0]);
      float *output_errors = reinterpret_cast<float *>(output_items[1]);

      std::size_t numberTags = 0;
      std::vector<gr::tag_t> tags;

      get_tags_in_range(tags, 0, sample_to_start_processing_abs, sample_to_stop_processing_abs);
      //std::cout << "tags.size(): " << tags.size() << std::endl;
      for (const auto &tag : tags)
      {
        if(tag.key != pmt::string_to_symbol(trigger_tag_name))
            continue;

        //std::cout << "demux_ff::trigger tag recognized. Offset: " <<  tag.offset << std::endl;

        uint64_t window_start_abs = tag.offset - d_pre_trigger_window_size;
        if( window_start_abs < sample_to_start_processing_abs ) // this tag already was processed in the last iteration
            continue;

        // If our output port is full, or the window is out of bound we cannot process the tag in this iteration.
        if( int(nitems_produced + d_window_size) > noutput_items || window_start_abs + d_window_size >=  sample_to_stop_processing_abs )
        {
            // cosume all samples which occured before this tag-window, than leave the work function
            unsigned nitems_consumed = window_start_abs - sample_to_start_processing_abs;
            consume(0, nitems_consumed);
            if (input_items.size() > 1 && output_items.size() > 1)
                    consume(1, nitems_consumed);
            //std::cout << "exit 2 nitems_consumed: " << nitems_consumed << std::endl;
            return nitems_produced;
        }
//        std::cout << "window_start_abs: " << window_start_abs << std::endl;
//        std::cout << "window_start_rel: " << window_start_rel << std::endl;
//        std::cout << "ninput_items[0]: " << ninput_items[0] << std::endl;
//        std::cout << "ninput_items[1]: " << ninput_items[1] << std::endl;
//        std::cout << "noutput_items: " << noutput_items << std::endl;
//
//        std::cout << "tag.offset: " << tag.offset << std::endl;
//        std::cout << "nitems_read_min: " << nitems_read_min << std::endl;

        uint64_t window_start_rel = window_start_abs - sample_to_start_processing_abs;
        assert(int(nitems_produced + d_window_size) <= noutput_items);
        assert(window_start_rel + d_window_size <=  uint64_t(ninput_items_min));
        
        memcpy(&output_values[nitems_produced], &input_values[window_start_rel], d_window_size * sizeof(float));
        if (input_items.size() > 1 && output_items.size() > 1)
          memcpy(&output_errors[nitems_produced], &input_errors[window_start_rel], d_window_size * sizeof(float));

        //copy all tags of interest in the window
        std::vector<gr::tag_t> tags_in_window;
        get_tags_in_range(tags_in_window, 0, window_start_abs, window_start_abs + d_window_size);
        for (auto &tag_in_window : tags_in_window)
        {
            // re-adjust offset, according to produced items
            uint64_t tag_offset_relative_to_window = tag_in_window.offset - window_start_abs;
            uint64_t new_offset = nitems_written(0) + nitems_produced + tag_offset_relative_to_window;
            // copy our trigger tag
            if(tag_in_window == tag)
            {
//                std::cout << "window_start_abs: " << window_start_abs << std::endl;
//                std::cout << "tag_in_window.offset: " << tag_in_window.offset << std::endl;
//                std::cout << "tag_offset_relative_to_window: " << tag_offset_relative_to_window << std::endl;
//                std::cout << "added tag old offset: " << tag_in_window.offset << std::endl;
//                std::cout << "added tag new offset: " << new_offset << std::endl;

                trigger_t trigger_tag_data = decode_trigger_tag(tag_in_window);
                trigger_tag_data.pre_trigger_samples = d_pre_trigger_window_size;
                trigger_tag_data.post_trigger_samples = d_post_trigger_window_size;
                add_item_tag(0, make_trigger_tag(trigger_tag_data, new_offset ));
                numberTags++;
            }

            // copy all other non-trigger tags
            if(tag_in_window.key != pmt::string_to_symbol(trigger_tag_name))
            {
                tag_in_window.offset = new_offset;
                add_item_tag(0, tag_in_window);
            }
        }

        nitems_produced += d_window_size;
      } // for all trigger tags

      if( ninput_items_min > d_pre_trigger_window_size)
      {
          unsigned nitems_consumed = ninput_items_min - d_pre_trigger_window_size;
          // consume all samples of this iteration (leave some samples on the input .. possibly needed for trigger tag on next iteration)
          consume(0, nitems_consumed);
          if (input_items.size() > 1 && output_items.size() > 1)
                  consume(1, nitems_consumed);
          //std::cout << "exit 2 nitems_consumed: " << nitems_consumed << std::endl;
      }
      return nitems_produced;
    } /* general_work */
  } /* namespace digitizers */
} /* namespace gr */

