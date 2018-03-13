/* -*- c++ -*- */
/* 
 * Copyright 2018 <+YOU OR YOUR COMPANY+>.
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
      median_filter<float> d_median;
      average_filter<float> d_average;
      int d_vec_len;
      int d_median_prewindow;
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

