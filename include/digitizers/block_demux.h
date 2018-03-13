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


#ifndef INCLUDED_DIGITIZERS_BLOCK_DEMUX_H
#define INCLUDED_DIGITIZERS_BLOCK_DEMUX_H

#include <digitizers/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief The block recieves a char(8bit) input and extracts
     * the n-th bit the user wants. If this bit is zero, then zero is passed along,
     * otherwise 255.
     *
     * \ingroup digitizers
     */
    class DIGITIZERS_API block_demux : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<block_demux> sptr;

      /*!
       * \brief Creates the demux block, where only one bit is kept.
       *
       * \param bit_to_keep Which bit should be extracted from the sequence.
       */
      static sptr make(int bit_to_keep);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_BLOCK_DEMUX_H */

