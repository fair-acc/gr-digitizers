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


#ifndef INCLUDED_DIGITIZERS_SIMULATION_SOURCE_H
#define INCLUDED_DIGITIZERS_SIMULATION_SOURCE_H

#include <digitizers/api.h>
#include <digitizers/digitizer_block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief Simulates a device simmilar to a picoscope oscilloscope.
     * User needs to set the data with the function exposed, for the block
     * to recurrantly print to the outputs.
     * \ingroup digitizers
     */
    class DIGITIZERS_API simulation_source : virtual public digitizer_block
    {
     public:
      typedef boost::shared_ptr<simulation_source> sptr;

      /*!
       * \brief Creates a simulator device. Before start is called the vectors
       * need to be specified via set_data!
       */
      static sptr make();

      /*!
       * \brief Sets the vectors and the overflow pattern to be passed to the output
       * recursively.
       *
       * \param ch_a_vec Vector that is passed on the channel a
       * \param ch_b_vec Vector that is passed on the channel b
       * \param ch_c_vec Vector that is passed on the channel c
       * \param overflow_pattern desired overflow events on all channels.
       */
      virtual void set_data(std::vector<float> ch_a_vec,
          std::vector<float> ch_b_vec,
          std::vector<uint8_t> port_vec,
          int overflow_pattern) = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_SIMULATION_SOURCE_H */

