/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_STREAM_TO_VECTOR_OVERLAY_FF_IMPL_H
#define INCLUDED_DIGITIZERS_STREAM_TO_VECTOR_OVERLAY_FF_IMPL_H

#include <digitizers/stream_to_vector_overlay_ff.h>
#include <digitizers/tags.h>

namespace gr {
  namespace digitizers {

    class stream_to_vector_overlay_ff_impl : public stream_to_vector_overlay_ff
    {
     private:
      double d_samp_rate;
      double d_delta_t;
      int d_vec_size;
      double d_offset;
      acq_info_t d_acq_info;
      uint64_t d_tag_offset;

      void save_tags(int count);

      void push_tags();
     public:
      stream_to_vector_overlay_ff_impl(int vec_size, double samp_rate, double delta_t);
      ~stream_to_vector_overlay_ff_impl();

      bool start() override;

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_STREAM_TO_VECTOR_OVERLAY_FF_IMPL_H */

