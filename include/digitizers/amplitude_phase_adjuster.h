/* -*- c++ -*- */
/* 
 * Copyright 2018 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
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
      typedef boost::shared_ptr<amplitude_phase_adjuster> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::amplitude_phase_adjuster.
       *
       * To avoid accidental use of raw pointers, digitizers::amplitude_phase_adjuster's
       * constructor is in a private implementation
       * class. digitizers::amplitude_phase_adjuster::make is the public interface for
       * creating new instances.
       *
       * \param amplitude_usr User set amplitude multiplier
       * \param phi_usr User set phase shift offset(deg)
       * \param phi_fq_usr User set phase shift multiplier(deg)
       */
      static sptr make(double ampl_usr, double phi_usr, double phi_fq_usr);

      /*!
       * \brief Update parameters for the adjustment.
       *
       * \param amplitude_usr User set amplitude multiplier
       * \param phi_usr User set phase shift offset(deg)
       * \param phi_fq_usr User set phase shift multiplier(deg)
       */
      virtual void update_design(double ampl_usr,
              double phi_usr,
              double phi_fq_usr) = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_AMPLITUDE_PHASE_ADJUSTER_H */

