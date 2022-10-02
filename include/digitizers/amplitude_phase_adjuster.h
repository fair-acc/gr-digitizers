/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_AMPLITUDE_PHASE_ADJUSTER_H
#define INCLUDED_DIGITIZERS_AMPLITUDE_PHASE_ADJUSTER_H

#include <digitizers/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief Adjust the amplitude and phase signals. By multiplying and offseting them
     * by the user specified amount.
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API amplitude_phase_adjuster : virtual public gr::sync_block
    {
     public:
      typedef std::shared_ptr<amplitude_phase_adjuster> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::amplitude_phase_adjuster.
       *
       * To avoid accidental use of raw pointers, digitizers::amplitude_phase_adjuster's
       * constructor is in a private implementation
       * class. digitizers::amplitude_phase_adjuster::make is the public interface for
       * creating new instances.
       *
       * \param ampl_usr User set amplitude multiplier
       * \param phi_usr User set phase shift offset(deg)
       * \param phi_fq_usr User set phase shift multiplier(deg)
       */
      static sptr make(double ampl_usr, double phi_usr, double phi_fq_usr);

      /*!
       * \brief Update parameters for the adjustment.
       *
       * \param ampl_usr User set amplitude multiplier
       * \param phi_usr User set phase shift offset(deg)
       * \param phi_fq_usr User set phase shift multiplier(deg)
       */
      virtual void update_design(double ampl_usr, double phi_usr, double phi_fq_usr) = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_AMPLITUDE_PHASE_ADJUSTER_H */

