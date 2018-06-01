/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_AMPLITUDE_AND_PHASE_HELPER_H
#define INCLUDED_DIGITIZERS_AMPLITUDE_AND_PHASE_HELPER_H

#include <digitizers/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief helper circuit for phase detection of a
     * signal with respect to a reference signal. This block is synchronous in nature.
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API amplitude_and_phase_helper : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<amplitude_and_phase_helper> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::amplitude_and_phase_helper.
       * It takes no parameters.
       */
      static sptr make();
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_AMPLITUDE_AND_PHASE_HELPER_H */

