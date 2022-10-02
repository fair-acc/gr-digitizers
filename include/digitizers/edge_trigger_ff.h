/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_EDGE_TRIGGER_FF_H
#define INCLUDED_DIGITIZERS_EDGE_TRIGGER_FF_H

#include <digitizers/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief Edge detector with UDP notifications
     *
     * Outputs value one or zero based on a threshold values (hi, lo), similar to GR's Threshold
     * block. That is if the value of input signal exceeds the the threshold value hi, value one
     * is send down the stream and if input signal goes below low threshold value zero is send
     * down the stream.
     *
     * This feature allows the users to compare actual signal and the state detected by this block.
     *                                           ______________
     *                                          |              |
     * ------+--------------------------------->|              |
     *       |     ______________               |   Qt GUI     |
     *       |    |              |          +-->|              |
     *       |    |              |          |   |______________|
     *       +--->| Edge Trigger |----------+
     *            |              |
     *            |______________|
     *
     * In addition, whenever a FIRST rising or falling edge is detected following a trigger event
     * (determined based on the trigger tag), an UDP datagram is send to all receiving host. The
     * format of the datagram is shown below:
     *
     * \code
     * <edgeDetect
     *        edge="rising"
     *        val="0.6545"
     *        timingEventTimeStamp="<timestamp>"
     *        retriggerEventTimeStamp="<timestamp>"
     *        delaySinceLastTimingEvent="<delay in nanoseconds>"
     *        samplesSinceLastTimingEvent="<samples>" />
     * \endcode
     *
     * In order to get precise trigger timestamp this block waits to receive a WR tag before it sends
     * an edge_detect tag down the stream or sends out an UDP datagram.
     *
     * Note all timestamps are assumed to be integers, namely int64_t, representing number of
     * nanoseconds since UNIX epoch (UTC nanoseconds).
     *
     * This block uses the absolute timestamp of the last trigger event (wr_event.timestamp) and number
     * of samples passed since then to calculate retrigger event timestamp and delay.
     *
     *
     *             [wr_event]
     * [trigger]       |      +--------------------+
     *    |            |      |                    |
     * ---+-------------------+                    +----------------- (input signal)
     *
     *    <----- delay ------>
     *
     * Note, user delay provided with the acq_info tag is accounted for when calculating the absolute
     * timestamp of the retrigger event:
     *
     * retriggerEventTimeStamp = timingEventTimeStamp + detected_delay + user_delay
     *
     * This is needed because the timestamp of the timing event does not contain a component (user delay)
     * foreseen to be used to compensate for cable delays and similar.
     *
     * Pre-trigger samples are ignored because this block cannot work with negative delays.
     *
     * This block generates the following tag:
     *  - edge_detect
     * Note this tag might not be attached to the exact offset where the edge has been detected but at
     * some point later when a WR event tag is received. Tag propagation policy is set to TPP_DONT.
     *
     * Output port is optional.
     *
     * \ingroup digitizers
     */
    class DIGITIZERS_API edge_trigger_ff : virtual public gr::block
    {
     public:
      typedef std::shared_ptr<edge_trigger_ff> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::edge_trigger_ff.
       *
       * \param sampling sampling rate in Hz
       * \param lo low threshold
       * \param hi high threshold
       * \param initial_state initial state of the output
       * \param send_udp true to send edge detect datagrams, else false
       * \param host_list comma separated list of hosts/ports, e.g. "127.0.0.1:33433, 10.5.2.33:55800"
       * \param send_udp_on_raising_edge send datagram on rising or on falling edge
       * \param timeout timeout in seconds, amount of time to wait to receive WR event or edge trigger
       * \return shared ptr
       */
      static sptr make(float sampling,
              float lo, float hi,
              float initial_state=0,
              bool send_udp=true,
              std::string host_list="localhost:2025",
              bool send_udp_on_raising_edge=true,
              float timeout=0.01f);

      virtual void set_send_udp(bool send_state) = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_EDGE_TRIGGER_FF_H */

