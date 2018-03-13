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

#ifndef INCLUDED_DIGITIZERS_FUNCTION_FF_IMPL_H
#define INCLUDED_DIGITIZERS_FUNCTION_FF_IMPL_H

#include <digitizers/function_ff.h>
#include <digitizers/tags.h>
#include <boost/thread/mutex.hpp>

namespace gr {
  namespace digitizers {

    class function_ff_impl : public function_ff
    {
     private:
      acq_info_t d_acq_info;

      std::vector<float> d_ref, d_min, d_max;
      std::vector<int64_t> d_timing;

      boost::mutex d_mutex;

     public:
      function_ff_impl(int decimation);
      ~function_ff_impl();

      bool start() override;

      void set_function(const std::vector<float> &timing,
              const std::vector<float> &ref,
              const std::vector<float> &min,
              const std::vector<float> &max) override;

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items) override;

    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_FUNCTION_FF_IMPL_H */

