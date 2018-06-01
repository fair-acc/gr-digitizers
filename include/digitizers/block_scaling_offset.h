/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_BLOCK_SCALING_OFFSET_H
#define INCLUDED_DIGITIZERS_BLOCK_SCALING_OFFSET_H

#include <digitizers/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief Scales and offsets the input signal. All values are
     * passed through. Scaling and offset parameters are settable.
     *
     * Accepts two inputs and produces two outputs. the first input
     * gets scaled and offsetted, while the second input is only scaled.
     *
     * \ingroup digitizers
     */
    class DIGITIZERS_API block_scaling_offset : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<block_scaling_offset> sptr;

      /*!
       * \brief Create a scaling and offset block.
       *
       * \param scale multiplier of the signal
       * \param offset offsetting factor after scaling.
       */
      static sptr make(double scale, double offset);

      /*!
       * \brief Update parameters for scaling and offset.
       *
       * \param scale multiplier of the signal
       * \param offset offsetting factor after scaling.
       */
      virtual void update_design(double scale,
          double offset) = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_BLOCK_SCALING_OFFSET_H */

