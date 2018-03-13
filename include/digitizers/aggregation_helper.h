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


#ifndef INCLUDED_DIGITIZERS_AGGREGATION_HELPER_H
#define INCLUDED_DIGITIZERS_AGGREGATION_HELPER_H

#include <digitizers/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace digitizers {

    /*!
     * A subgroup of the aggregation circuit as specified in the design proposal, implemented
     * as simple function calls instead of connecting blocks.
     *
     * what it does:
     * out = decimate(sqrt(abs(a - b^2) + (sigma_mult * c^2)), decim);
     * \ingroup digitizers
     */
    class DIGITIZERS_API aggregation_helper : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<aggregation_helper> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::aggregation_helper.
       *
       * \param decim The decimation factor
       * \param sigma_mult The sigma multiplier in the function specified in the blocks description
       */
      static sptr make(int decim, float sigma_mult);
      /*!
       * \brief Updates the design in the middle of execution.
       *
       * \param sigma_mult The sigma multiplier in the function specified in the blocks description
       */
      virtual void update_design(float sigma_mult) = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_AGGREGATION_HELPER_H */

