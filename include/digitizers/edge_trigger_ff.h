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


#ifndef INCLUDED_DIGITIZERS_EDGE_TRIGGER_FF_H
#define INCLUDED_DIGITIZERS_EDGE_TRIGGER_FF_H

#include <digitizers/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief Edge detector with UDP notifications
     *
     * Output a 1 or zero based on a threshold values (hi, lo), similar to GR's Threshold block.
     *
     * It sends an UDP datagram to reciving hosts when a FIRST rising or falling edge is detected
     * after the timing event. The format of the datagram is the following:
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
     * Note timestamp is assumed to be an integer, namely int64_t, and it represents number
     * of nanoseconds since UNIX epoch (UTC nanoseconds).
     *
     * The information about the timing event needs to be provided by using the acq_info tag.
     * Initially the timestamp of the last timing event is set to zero. It should be noted that
     * user delay is applied if specified in the acq_info tag.
     *
     * Pre-trigger samples are ignored because the this block cannot work with negative delays.
     *
     * <br>
     * This block generates the following tags:
     *  - edgeDetect : bool Reads true if edge is raising
     *  - sampleOffsetToTiming : long number of samples from the last timing event
     * <br>
     * Note, output port might be left unused.
     *
     * \ingroup digitizers
     */
    class DIGITIZERS_API edge_trigger_ff : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<edge_trigger_ff> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::edge_trigger_ff.
       *
       * \param sampling sampling rate in Hz
       * \param lo low threshold
       * \param hi high threshold
       * \param initial_state initial state of the output
       * \param send_udp ture to send edge detect datagrams, else false
       * \param host_list comma seperated list of hosts/ports, e.g. "127.0.0.1:33433, 10.5.2.33:55800"
       * \return shared ptr
       */
      static sptr make(float sampling, float lo, float hi, float initial_state=0,
              bool send_udp=true, std::string host_list="localhost:2025", bool send_udp_on_raising_edge=true);

      virtual void set_send_udp(bool send_state) = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_EDGE_TRIGGER_FF_H */

