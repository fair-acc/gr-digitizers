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


#ifndef INCLUDED_DIGITIZERS_DECIMATE_AND_ADJUST_TIMEBASE_H
#define INCLUDED_DIGITIZERS_DECIMATE_AND_ADJUST_TIMEBASE_H

#include <digitizers/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief Decimates the samples and fixes tags.
     * Tags are attached to the correspondent decimated sample,
     * timebase_info tags are fixed with the decimation factor.
     *
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API decimate_and_adjust_timebase : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<decimate_and_adjust_timebase> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of
       * digitizers::decimate_and_adjust_timebase.
       *
       * \param decimation The decimation factor.
       */
      static sptr make(int decimation);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_DECIMATE_AND_ADJUST_TIMEBASE_H */

