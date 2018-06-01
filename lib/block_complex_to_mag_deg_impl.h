/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_BLOCK_COMPLEX_TO_MAG_DEG_IMPL_H
#define INCLUDED_DIGITIZERS_BLOCK_COMPLEX_TO_MAG_DEG_IMPL_H

#include <digitizers/block_complex_to_mag_deg.h>

namespace gr {
  namespace digitizers {

    class block_complex_to_mag_deg_impl : public block_complex_to_mag_deg
    {
     private:
      int d_vec_len;

     public:
      block_complex_to_mag_deg_impl(int vel_len);
      ~block_complex_to_mag_deg_impl();

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_BLOCK_COMPLEX_TO_MAG_DEG_IMPL_H */

