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


#ifndef INCLUDED_DIGITIZERS_INTERLOCK_GENERATION_FF_H
#define INCLUDED_DIGITIZERS_INTERLOCK_GENERATION_FF_H

#include <digitizers/api.h>
#include <gnuradio/sync_block.h>
#include <digitizers/sink_common.h>

namespace gr {
  namespace digitizers {

    /*!
     * Callback specifier. User must implement this type of function,
     * and register it with the block, via set_interlock_callback,
     * each time an interlock is issued. Otherwise warning is printed.
     */
    typedef void (*interlock_cb_t)(int64_t timestamp, void *userdata);

    /*!
     * \brief Issues interlock if the signal is out of bounds. Bounds are passed as center and
     * width vectors. When in normal operation, the last two values are used, when beam_in tag
     * arrives, the block changes its acceptable signal bounds, by iterating through the
     * center and width vectors. each sample's window is linearly interpolated between the
     * previous window, and the next. This way it computes a continuously changing window.
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API interlock_generation_ff : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<interlock_generation_ff> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::stft_goertzl_dynamic.
       *
       * \param samp_rate sampling rate of the signal.
       * \param time_of_transition length in seconds of the window effectiveness
       * \param center Central frequency of the corresponding frequency window
       * \param width Width of the corresponding frequency window
       */
      static sptr make(float max_min, float max_max);

      /*!
       * \brief Register a callable, called the first time an interlock event happens.
       * Resets state, whenever new callback is registered.
       *
       * \param callback callback to be called once the buffer is full
       * \param ptr a void pinter that is passed to the ::freq_data_available_cb_t function
       */
      virtual void set_interlock_callback(interlock_cb_t callback, void *ptr) = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_INTERLOCK_GENERATION_FF_H */

