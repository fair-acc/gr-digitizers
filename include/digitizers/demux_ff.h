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
    * This block uses the following tags in order to extract triggered data:
    *  - trigger tag: This tag indicates the exact location of the trigger (e.g. attached based on D0)
    *  - wr_event tag: Timing information (typically received some time later)
    *  - edge_detect tag: This one is is needed only if realignment is required
    *  - acq_info tag: Status information, constant user delays (optional, default user delay is 0s)
    *
    * Based on pre-trigger and post-trigger window configuration parameters and user delays provided
    * with the acq_info tag a subset of samples is extracted as shown below:
    *
    *   [ all pre-trigger samples       |   all post-trigger samples                ]
    * step a)
    *   .          [ pre-trigger window |  post-trigger window           ]          .
    * step b)
    *                                   ----> actual delay
    *   .          .    [                   |                                ]      .
    *
    * In step a) a desired pre-trigger and post-trigger window is determined and in step b) a complete
    * window is shifted by actual_delay including: user defined delay and realignment delay.
    *
    * The following tags are generated:
    *  - acq_info
    *  - trigger
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
      static sptr make(float samp_rate, unsigned history, unsigned post_trigger_window, unsigned pre_trigger_window=0);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_DEMUX_FF_H */

