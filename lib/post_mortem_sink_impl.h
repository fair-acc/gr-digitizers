/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
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
      float d_samp_rate;

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
      
      post_mortem_sink_impl(std::string name, std::string unit, float samp_rate, size_t buffer_size);

      ~post_mortem_sink_impl();

      int work(int noutput_items, gr_vector_const_void_star &input_items,
              gr_vector_void_star &output_items) override;

      signal_metadata_t get_metadata() override;

      size_t get_buffer_size() override;

      void freeze_buffer() override;

      size_t get_items(size_t nr_items_to_read, float *values, float *errors, measurement_info_t *info) override;

      float get_sample_rate() override;

     private:

      void decode_tags(int ninput_items);

    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_POST_MORTEM_SINK_IMPL_H */

