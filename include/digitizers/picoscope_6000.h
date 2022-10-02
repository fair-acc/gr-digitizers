/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_PICOSCOPE_6000_H
#define INCLUDED_DIGITIZERS_PICOSCOPE_6000_H

#include <digitizers/api.h>
#include <digitizers/digitizer_block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief  This block connects to the picoscope device with the given serial number.
     * Settable parameters for this block are auto arm, which is used in the start().
     * All other settings are accessible through the digitizer_block interface.
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API picoscope_6000 : virtual public digitizer_block
    {
     public:
      typedef std::shared_ptr<picoscope_6000> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::picoscope_6000.
       *
       * To avoid accidental use of raw pointers, digitizers::picoscope_6000's
       * constructor is in a private implementation
       * class. digitizers::picoscope_6000::make is the public interface for
       * creating new instances.
       */
      static sptr make(std::string serial_number, bool auto_arm=true);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_PICOSCOPE_6000_H */

