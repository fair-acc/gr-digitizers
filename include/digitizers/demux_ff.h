/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_DEMUX_FF_H
#define INCLUDED_DIGITIZERS_DEMUX_FF_H

#include <digitizers/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace digitizers {

   /*!
    * \brief Extracts a subset of triggered data.
    *
    * This block uses trigger tags in order to cut out data windows around them and forwards the cutted windows to the output
    *  [ pre-trigger window |  post-trigger window ]
    *
    * Trigger and AcqInfo tags in each window are as well forwarded to the outputs.
    * Tags are re-created in order to reset the offset. (Just passing them to the output would result in wrong offset values)
    *
    * \ingroup digitizers
    */
    class DIGITIZERS_API demux_ff : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<demux_ff> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::demux_ff.
       *
       * To avoid accidental use of raw pointers, digitizers::demux_ff's
       * constructor is in a private implementation
       * class. digitizers::demux_ff::make is the public interface for
       * creating new instances.
       */
      static sptr make(unsigned post_trigger_window, unsigned pre_trigger_window=0);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_DEMUX_FF_H */

