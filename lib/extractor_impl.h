/* -*- c++ -*- */
/* 
 * Copyright 2017 <+YOU OR YOUR COMPANY+>.
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

#ifndef INCLUDED_DIGITIZERS_EXTRACTOR_IMPL_H
#define INCLUDED_DIGITIZERS_EXTRACTOR_IMPL_H

#include <digitizers/extractor.h>
#include <digitizers/tags.h>
#include "utils.h"
#include <boost/optional.hpp>

namespace gr {
  namespace digitizers {

    struct worker_state_t
    {
      worker_state_t()
          : start_trigger_offset(0),
            trigger_samples(0)
      {}

      uint64_t start_trigger_offset;
      uint32_t trigger_samples;
      acq_info_t acq_info; // tag containging triggered data info
    };

    class extractor_impl : public extractor
    {
     public:
      extractor_impl(float post_trigger_window, float pre_trigger_window=0.0);

      ~extractor_impl();

      bool start() override;

      void forecast(int noutput_items, gr_vector_int &ninput_items_required) override;

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items) override;

     private:
      double d_pre_trigger_window;
      double d_post_trigger_window;

      worker_state_t d_state;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_EXTRACTOR_IMPL_H */

