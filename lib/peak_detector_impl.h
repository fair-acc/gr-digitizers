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

#ifndef INCLUDED_DIGITIZERS_PEAK_DETECTOR_IMPL_H
#define INCLUDED_DIGITIZERS_PEAK_DETECTOR_IMPL_H

#include <digitizers/peak_detector.h>

namespace gr {
  namespace digitizers {

    class peak_detector_impl : public peak_detector
    {
     private:
      int d_vec_len;
      int d_start_bin;
      int d_end_bin;
      int d_prox;
      double d_freq;

     public:
      peak_detector_impl(double samp_rate,
          int vec_len,
          int start_bin,
          int end_bin,
          int proximity);
      ~peak_detector_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_PEAK_DETECTOR_IMPL_H */

