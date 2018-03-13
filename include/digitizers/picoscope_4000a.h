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


#ifndef INCLUDED_DIGITIZERS_PICOSCOPE_4000A_H
#define INCLUDED_DIGITIZERS_PICOSCOPE_4000A_H

#include <digitizers/api.h>
#include <digitizers/digitizer_block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief This block connects to the picoscope device with the given serial number.
     * Settable parameters for this block are auto arm, which is used in the start().
     * All other settings are accessible through the digitizer_block interface.
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API picoscope_4000a : virtual public digitizer_block
    {
     public:
      typedef boost::shared_ptr<picoscope_4000a> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::picoscope_4000a.
       *
       * To avoid accidental use of raw pointers, digitizers::picoscope_4000a's
       * constructor is in a private implementation
       * class. digitizers::picoscope_4000a::make is the public interface for
       * creating new instances.
       */
      static sptr make(std::string serial_number, bool auto_arm=true);
    };

  } // namespace digitizers
} // namespace gr


#endif /* INCLUDED_DIGITIZERS_PICOSCOPE_4000A_H */

