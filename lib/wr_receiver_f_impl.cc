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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "wr_receiver_f_impl.h"

namespace gr {
  namespace digitizers {

    wr_receiver_f::sptr
    wr_receiver_f::make()
    {
      return gnuradio::get_initial_sptr
        (new wr_receiver_f_impl());
    }

    /*
     * The private constructor
     */
    wr_receiver_f_impl::wr_receiver_f_impl()
      : gr::sync_block("wr_receiver_f",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(1, 1, sizeof(float)))
    {}

    /*
     * Our virtual destructor.
     */
    wr_receiver_f_impl::~wr_receiver_f_impl()
    {
    }

    int
    wr_receiver_f_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      // Zero the output
      memset(output_items[0], 0, noutput_items * sizeof(float));

      // Attach event tags
      wr_event_t event;
      while (d_event_queue.pop(event))
      {
        auto tag = make_wr_event_tag(event, nitems_written(0));
        add_item_tag(0, tag);
      }

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

    bool wr_receiver_f_impl::start()
    {
      d_event_queue.clear();
      return true;
    }

    bool
    wr_receiver_f_impl::add_timing_event(const std::string &event_id, int64_t wr_trigger_stamp, int64_t wr_trigger_stamp_utc)
    {
      wr_event_t event;
      event.event_id = event_id;
      event.wr_trigger_stamp = wr_trigger_stamp;
      event.wr_trigger_stamp_utc = wr_trigger_stamp_utc;
      d_event_queue.push(event);
      return true;
    }

  } /* namespace digitizers */
} /* namespace gr */

