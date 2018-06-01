/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_BLOCK_DEMUX_H
#define INCLUDED_DIGITIZERS_BLOCK_DEMUX_H

#include <digitizers/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief The block recieves a char(8bit) input and extracts
     * the n-th bit the user wants. If this bit is zero, then zero is passed along,
     * otherwise 255.
     *
     * \ingroup digitizers
     */
    class DIGITIZERS_API block_demux : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<block_demux> sptr;

      /*!
       * \brief Creates the demux block, where only one bit is kept.
       *
       * \param bit_to_keep Which bit should be extracted from the sequence.
       */
      static sptr make(int bit_to_keep);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_BLOCK_DEMUX_H */

