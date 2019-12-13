/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "decimate_and_adjust_timebase_impl.h"
#include <digitizers/tags.h>
#include <boost/optional.hpp>

namespace gr {
  namespace digitizers {

    decimate_and_adjust_timebase::sptr
    decimate_and_adjust_timebase::make(int decimation, double delay, float samp_rate)
    {
      return gnuradio::get_initial_sptr
        (new decimate_and_adjust_timebase_impl(decimation, delay, samp_rate));
    }

    decimate_and_adjust_timebase_impl::decimate_and_adjust_timebase_impl(int decimation, double delay, float samp_rate)
      : gr::sync_decimator("decimate_and_adjust_timebase",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(1, 1, sizeof(float)), decimation),
        d_delay(delay)

    {
      set_relative_rate(1./decimation);
      set_tag_propagation_policy(TPP_CUSTOM);

      assert(samp_rate != 0);
      d_sample_sample_distance_input_ns = 1000000000./samp_rate;
    }

    decimate_and_adjust_timebase_impl::~decimate_and_adjust_timebase_impl()
    {
    }

    int
    decimate_and_adjust_timebase_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const auto decim = decimation();

      float *out = (float *) output_items[0];
      const float *in = (const float *) input_items[0];

      int i_in = 0, i_out = 0;
      for(; i_out < noutput_items; i_out++)
      {
        // Keep one in N functionality
        out[i_out] = in[i_in];

        std::vector<gr::tag_t> tags;
        get_tags_in_range(tags, 0, nitems_read(0) + i_in, nitems_read(0) + i_in + decim );
        // required to merge acq_infotags due to this bug: https://github.com/gnuradio/gnuradio/issues/2364
        // otherwise we will eat to much memory
        // TODO: Fix bug in gnuradio (https://gitlab.com/al.schwinn/gr-digitizers/issues/33)
        acq_info_t merged_acq_info;
        merged_acq_info.status = 0;
        bool found_acq_info = false;
        for(auto tag : tags)
        {
            //std::cout << "tag found: " << tag.key << std::endl;
            if(tag.key == pmt::string_to_symbol(trigger_tag_name))
            {
                trigger_t trigger_tag_data = decode_trigger_tag(tag);
                add_item_tag(0, make_trigger_tag(trigger_tag_data, nitems_written(0) + i_out));
                //std::cout << "trigger tag added" << std::endl;
            }
            else if(tag.key == pmt::string_to_symbol(acq_info_tag_name))
            {
                found_acq_info = true;
                merged_acq_info.status |= decode_acq_info_tag(tag).status;
            }
            else
            {
                tag.offset = nitems_written(0) + i_out;
                add_item_tag(0, tag);
            }
        }
        if(found_acq_info)
            add_item_tag(0, make_acq_info_tag(merged_acq_info, nitems_written(0) + i_out));

        i_in += decim;
      }


      return noutput_items;
    }

  } /* namespace digitizers */
} /* namespace gr */

