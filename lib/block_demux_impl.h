/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_BLOCK_DEMUX_IMPL_H
#define INCLUDED_DIGITIZERS_BLOCK_DEMUX_IMPL_H

#include <digitizers/block_demux.h>

namespace gr {
  namespace digitizers {

    class block_demux_impl : public block_demux
    {
     private:
      uint8_t d_bit;
     public:

      block_demux_impl(int bit_to_keep);

      ~block_demux_impl();

      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_BLOCK_DEMUX_IMPL_H */

