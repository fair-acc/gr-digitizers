/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_EDGE_TRIGGER_RECEIVER_F_H
#define INCLUDED_DIGITIZERS_EDGE_TRIGGER_RECEIVER_F_H

#include <digitizers/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief Listens for UDP packets. when it receives one, it sends a predetermined number of zeroes,
     * with a tag. User can then do everything else. The format of the datagram is shown below:
     *
     * \code
     * <edgeDetect
     *        edge="rising"
     *        val="0.6545"
     *        timingEventTimeStamp="timestamp"
     *        retriggerEventTimeStamp="timestamp"
     *        delaySinceLastTimingEvent="delay in nanoseconds"
     *        samplesSinceLastTimingEvent="samples" />
     * \endcode
     *
     * The stream is composed of only zeroes. It is up to the user to throttle the block.
     * When a datagram with this content arrives, the block attaches a tag to the stream.
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API edge_trigger_receiver_f : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<edge_trigger_receiver_f> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::edge_trigger_receiver_f.
       *
       * To avoid accidental use of raw pointers, digitizers::edge_trigger_receiver_f's
       * constructor is in a private implementation
       * class. digitizers::edge_trigger_receiver_f::make is the public interface for
       * creating new instances.
       */
      static sptr make(std::string addr, int port);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_EDGE_TRIGGER_RECEIVER_F_H */

