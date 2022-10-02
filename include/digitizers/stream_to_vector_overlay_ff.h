/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
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
     * delta_samps = samp_rate * delta_t
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
      typedef std::shared_ptr<stream_to_vector_overlay_ff> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::stream_to_vector_overlay_ff.
       *
       * To avoid accidental use of raw pointers, digitizers::stream_to_vector_overlay_ff's
       * constructor is in a private implementation
       * class. digitizers::stream_to_vector_overlay_ff::make is the public interface for
       * creating new instances.
       *
       * \param vec_size output vector size
       * \param samp_rate sample rate of input
       * \param delta_t the time in seconds between each acquisition.
       */
      static sptr make(int vec_size, double samp_rate, double delta_t);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_STREAM_TO_VECTOR_OVERLAY_FF_H */

