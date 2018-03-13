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


#ifndef INCLUDED_DIGITIZERS_MEDIAN_AND_AVERAGE_H
#define INCLUDED_DIGITIZERS_MEDIAN_AND_AVERAGE_H

#include <digitizers/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief This block filters the signal by first passing it into a median filter
     * that takes care of high signal spikes, and then averages the result to result
     * in a smooth estimation of the signals comonents. Operates with vectors, for
     * easier transmission of the full spectrum we're estimating at a given point.
     *
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API median_and_average : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<median_and_average> sptr;

      /*!
       * \brief Creates a block that filters the input signal.
       *
       * \param vec_len length of the input vector
       * \param n_med size of the median filter window.
       * \param n_lp size of the averaging filter window.
       */
      static sptr make(int vec_len, int n_med, int n_lp);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_MEDIAN_AND_AVERAGE_H */

