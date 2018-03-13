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


#ifndef INCLUDED_DIGITIZERS_STATUS_H
#define INCLUDED_DIGITIZERS_STATUS_H

#include <digitizers/api.h>

namespace gr {
namespace digitizers {


    /*!
     * \brief Channel-related status flags (bit-enum).
     */
    enum DIGITIZERS_API channel_status_t
    {
       // Overvoltage has occurred on the channel.
       CHANNEL_STATUS_OVERFLOW = 0x01,

       // Not enough pre- or post-trigger samples available to perform realignment or/and user delay.
       CHANNEL_STATUS_REALIGNMENT_ERROR = 0x02,

       // Insufficient buffer size to extract all samples
       CHANNEL_STATUS_NOT_ALL_DATA_EXTRACTED = 0x04
    };

    enum DIGITIZERS_API algorithm_id_t
    {
      FIR_LP=0,
      FIR_BP,
      FIR_CUSTOM,
      FIR_CUSTOM_FFT,
      IIR_LP,
      IIR_HP,
      IIR_CUSTOM,
      AVERAGE
    };

  }
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_STATUS_H */

