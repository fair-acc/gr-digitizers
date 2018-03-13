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

#ifndef INCLUDED_DIGITIZERS_INTERLOCK_GENERATION_FF_IMPL_H
#define INCLUDED_DIGITIZERS_INTERLOCK_GENERATION_FF_IMPL_H

#include <digitizers/interlock_generation_ff.h>
#include <digitizers/tags.h>

namespace gr {
  namespace digitizers {

    class interlock_generation_ff_impl : public interlock_generation_ff
    {
     private:
      float d_max_max; // max value to be used for upper bounds evaluation
      float d_max_min;

      //callback stuff
      bool d_interlock_issued;
      interlock_cb_t d_callback;
      void *d_user_data;

      // timing
      acq_info_t d_acq_info;

     public:
      interlock_generation_ff_impl(float max_min, float max_max);

      ~interlock_generation_ff_impl();

      bool start() override;

      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items) override;

      void set_interlock_callback(interlock_cb_t callback, void *ptr);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_INTERLOCK_GENERATION_FF_IMPL_H */

