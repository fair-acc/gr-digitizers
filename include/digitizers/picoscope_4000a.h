/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_PICOSCOPE_4000A_H
#define INCLUDED_DIGITIZERS_PICOSCOPE_4000A_H

#include <digitizers/api.h>
#include <digitizers/digitizer_block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief This block connects to the picoscope device with the given serial number.
     * Settable parameters for this block are auto arm, which is used in the start().
     * All other settings are accessible through the digitizer_block interface.
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API picoscope_4000a : virtual public digitizer_block
    {
     public:
      typedef std::shared_ptr<picoscope_4000a> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::picoscope_4000a.
       *
       * To avoid accidental use of raw pointers, digitizers::picoscope_4000a's
       * constructor is in a private implementation
       * class. digitizers::picoscope_4000a::make is the public interface for
       * creating new instances.
       */
      static sptr make(std::string serial_number, bool auto_arm=true);
    };

  } // namespace digitizers
} // namespace gr


#endif /* INCLUDED_DIGITIZERS_PICOSCOPE_4000A_H */

