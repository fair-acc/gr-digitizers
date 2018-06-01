/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_BLOCK_SCALING_OFFSET_IMPL_H
#define INCLUDED_DIGITIZERS_BLOCK_SCALING_OFFSET_IMPL_H

#include <digitizers/block_scaling_offset.h>

namespace gr {
  namespace digitizers {

    class block_scaling_offset_impl : public block_scaling_offset
    {
     private:
      double d_scale;
      double d_offset;

     public:
      block_scaling_offset_impl(double scale, double offset);
      ~block_scaling_offset_impl();

      int work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items);

      void update_design(double scale,
        double offset);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_BLOCK_SCALING_OFFSET_IMPL_H */

