/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_WR_RECEIVER_F_H
#define INCLUDED_DIGITIZERS_WR_RECEIVER_F_H

#include <digitizers/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief A GNU Radio source for injecting White Rabbit timing events. This block receives
     * information about the timing event and adds the tag to output stream. Otherwise it behaves
     * exactly as a null source.
     *
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API wr_receiver_f : virtual public gr::sync_block
    {
     public:
      typedef std::shared_ptr<wr_receiver_f> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::wr_receiver_f.
       *
       * To avoid accidental use of raw pointers, digitizers::wr_receiver_f's
       * constructor is in a private implementation
       * class. digitizers::wr_receiver_f::make is the public interface for
       * creating new instances.
       */
      static sptr make();

      /*!
       * \brief Add information about the timing event.
       *
       * \param event_id An arbitrary event descriptor
       * \param wr_trigger_stamp event timestamp, TAI ns
       * \param wr_trigger_stamp_utc event timestamp UTC
       */
      virtual bool add_timing_event(const std::string &event_id, int64_t wr_trigger_stamp, int64_t wr_trigger_stamp_utc) = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_WR_RECEIVER_F_H */

