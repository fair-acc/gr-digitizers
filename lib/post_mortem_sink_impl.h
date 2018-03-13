/* -*- c++ -*- */
/* 
 * Copyright 2017 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_DIGITIZERS_POST_MORTEM_SINK_IMPL_H
#define INCLUDED_DIGITIZERS_POST_MORTEM_SINK_IMPL_H

#include <digitizers/post_mortem_sink.h>
#include <digitizers/tags.h>
#include <utils.h>

namespace gr {
	namespace digitizers {

    class post_mortem_sink_impl : public post_mortem_sink
    {
     private:

      // Long term buffer used for the purpose of post-mortem and sequence
      // acquisition. Note we use two buffers for simplicity. Otherwise we
      // would need to have some more advanced status tracking.
      std::vector<float> d_buffer_values;
      std::vector<float> d_buffer_errors;
      size_t d_buffer_size;
      size_t d_write_index;

      // last acquisition info tag
      acq_info_t d_acq_info;

      // metadata
      signal_metadata_t d_metadata;

      // concurrency
      boost::mutex d_mutex;

      bool d_frozen;

     public:
      
      post_mortem_sink_impl(std::string name, std::string unit, size_t buffer_size);

      ~post_mortem_sink_impl();

      int work(int noutput_items, gr_vector_const_void_star &input_items,
              gr_vector_void_star &output_items) override;

      signal_metadata_t get_metadata() override;

      size_t get_buffer_size() override;

      void freeze_buffer() override;

      size_t get_items(size_t nr_items_to_read, float *values, float *errors, measurement_info_t *info) override;

     private:

      void decode_tags(int ninput_items);

    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_POST_MORTEM_SINK_IMPL_H */

