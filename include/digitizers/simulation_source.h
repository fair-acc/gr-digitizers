/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_SIMULATION_SOURCE_H
#define INCLUDED_DIGITIZERS_SIMULATION_SOURCE_H

#include <digitizers/api.h>
#include <digitizers/digitizer_block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief Simulates a device similar to a PicoScope oscilloscope. User needs to set the data
     * similar to the GNU Radio's vector source.
     *
     * Both rapid block and streaming mode acquisition modes are supported. In both modes the
     * simulation source behaves similarly, that is it keeps outputting the provided data buffer
     * again and again.
     *
     * Notes:
     *  - Error estimate is hardcoded to 0.005
     *  - This source sleeps 1 second in between rapid blocks
     *
     * \ingroup digitizers
     */
    class DIGITIZERS_API simulation_source : virtual public digitizer_block
    {
     public:
      typedef boost::shared_ptr<simulation_source> sptr;

      /*!
       * \brief Creates a simulator device. Before start method is called (or graphs is started)
       * the vectors need to be specified via set_data!
       */
      static sptr make();

      /*!
       * \brief Sets the vectors and the overflow pattern to be passed to the output
       * recursively.
       *
       * \param ch_a_vec Vector that is passed on the channel a
       * \param ch_b_vec Vector that is passed on the channel b
       * \param port_vec Vector that is passed on the port
       */
      virtual void set_data(const std::vector<float> &ch_a_vec,
          const std::vector<float> &ch_b_vec,
          const std::vector<uint8_t> &port_vec) = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_SIMULATION_SOURCE_H */

