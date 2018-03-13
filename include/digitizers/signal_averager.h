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


#ifndef INCLUDED_DIGITIZERS_SIGNAL_AVERAGER_H
#define INCLUDED_DIGITIZERS_SIGNAL_AVERAGER_H

#include <digitizers/api.h>
#include <gnuradio/sync_decimator.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief Average samples in window and generate a single sample.
     *
     * The block decimates the data with the step of window size.
     *
     * It supports multiple input signals. Does the same on each channel.
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API signal_averager : virtual public gr::sync_decimator
    {
     public:
      typedef boost::shared_ptr<signal_averager> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::signal_averager.
       *
       * \param num_inputs Number of input signals
       * \param window_size The decimation factor.
       */
      static sptr make(int num_inputs, int window_size);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_SIGNAL_AVERAGER_H */

