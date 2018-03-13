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


#ifndef INCLUDED_DIGITIZERS_BLOCK_SCALING_OFFSET_H
#define INCLUDED_DIGITIZERS_BLOCK_SCALING_OFFSET_H

#include <digitizers/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief Scales and offsets the input signal. All values are
     * passed through. Scaling and offset parameters are settable.
     *
     * Accepts two inputs and produces two outputs. the first input
     * gets scaled and offsetted, while the second input is only scaled.
     *
     * \ingroup digitizers
     */
    class DIGITIZERS_API block_scaling_offset : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<block_scaling_offset> sptr;

      /*!
       * \brief Create a scaling and offset block.
       *
       * \param scale multiplier of the signal
       * \param offset offsetting factor after scaling.
       */
      static sptr make(double scale, double offset);

      /*!
       * \brief Update parameters for scaling and offset.
       *
       * \param scale multiplier of the signal
       * \param offset offsetting factor after scaling.
       */
      virtual void update_design(double scale,
          double offset) = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_BLOCK_SCALING_OFFSET_H */

