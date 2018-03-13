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


#ifndef INCLUDED_DIGITIZERS_STREAM_TO_VECTOR_OVERLAY_FF_H
#define INCLUDED_DIGITIZERS_STREAM_TO_VECTOR_OVERLAY_FF_H

#include <digitizers/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief creates vectors of size vec_size, where the starts of the vectors are spaced apart
     * for delta_samps. when delta_samps is the same as vec_size, the block block acts as
     * stream_to_vector block, when delta_samps is smallet than vec_size, it overlays the output
     * vectors. When delta_samps is bigger than vec_size, it decimates the input.
     * Grpahical representation:
     * \verbatim
     * Case  delta_samps > vec_size:
     *
     * input:  ------------------------->
     * output: <--v1-->     <--v2-->
     *         |-----------|
     *          ^delta_samps
     *
     *
     * Case  delta_samps = vec_size:
     *
     * input:  ------------------------->
     * output: <--v1--><--v2-->
     *         |------|
     *          ^delta_samps
     *
     *
     * Case  delta_samps < vec_size:
     *
     * input:  ------------------------->
     * output: <--v1-->
     *             <--v2-->
     *         |--|
     *          ^delta_samps
     * \endverbatim
     *
     * Tags:
     * The tags get attached to all vectors that encompass tags. Also, if the tag was in the
     * samples between the last vector, and this one, they also get attched.
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API stream_to_vector_overlay_ff : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<stream_to_vector_overlay_ff> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::stream_to_vector_overlay_ff.
       *
       * To avoid accidental use of raw pointers, digitizers::stream_to_vector_overlay_ff's
       * constructor is in a private implementation
       * class. digitizers::stream_to_vector_overlay_ff::make is the public interface for
       * creating new instances.
       */
      static sptr make(int vec_size, int delta_samps);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_STREAM_TO_VECTOR_OVERLAY_FF_H */

