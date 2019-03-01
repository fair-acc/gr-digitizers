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

#ifndef INCLUDED_DIGITIZERS_WR_RECEIVER_F_IMPL_H
#define INCLUDED_DIGITIZERS_WR_RECEIVER_F_IMPL_H

#include <digitizers/wr_receiver_f.h>
#include <digitizers/tags.h>
#include "utils.h"

namespace gr {
  namespace digitizers {

    class wr_receiver_f_impl : public wr_receiver_f
    {
     private:
      concurrent_queue<wr_event_t> d_event_queue;

     public:
      wr_receiver_f_impl();
      ~wr_receiver_f_impl();

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items) override;

      bool start() override;

      bool add_timing_event(const std::string &event_id, int64_t wr_trigger_stamp, int64_t wr_trigger_stamp_utc) override;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_WR_RECEIVER_F_IMPL_H */

