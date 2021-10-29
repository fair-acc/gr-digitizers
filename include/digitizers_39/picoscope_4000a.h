/* -*- c++ -*- */
/*
 * Copyright 2021 FAIR.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DIGITIZERS_39_PICOSCOPE_4000A_H
#define INCLUDED_DIGITIZERS_39_PICOSCOPE_4000A_H

#include "api.h"
//#include <gnuradio/sync_block.h>
#include <digitizers_39/digitizer_block.h>

namespace gr {
  namespace digitizers_39 {

    /*!
     * \brief <+description of block+>
     * \ingroup digitizers_39
     *
     */
    class DIGITIZERS_39_API picoscope_4000a : virtual public gr::digitizers_39::digitizer_block
    {
     public:
      typedef std::shared_ptr<picoscope_4000a> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers_39::picoscope_4000a.
       *
       * To avoid accidental use of raw pointers, digitizers_39::picoscope_4000a's
       * constructor is in a private implementation
       * class. digitizers_39::picoscope_4000a::make is the public interface for
       * creating new instances.
       */
      static sptr make(std::string serial_number, bool auto_arm);
    };

  } // namespace digitizers_39
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_39_PICOSCOPE_4000A_H */

