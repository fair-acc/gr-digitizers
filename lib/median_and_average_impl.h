/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_MEDIAN_AND_AVERAGE_IMPL_H
#define INCLUDED_DIGITIZERS_MEDIAN_AND_AVERAGE_IMPL_H

#include <digitizers/median_and_average.h>
#include <algorithm>
#include "utils.h"
namespace gr {
  namespace digitizers {

    class median_and_average_impl : public median_and_average
    {
     private:
      // Nothing to declare in this block.
      int d_median;
      int d_average;
      int d_vec_len;
     public:
      median_and_average_impl(int vec_len, int n_med, int n_lp);
      ~median_and_average_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_MEDIAN_AND_AVERAGE_IMPL_H */

