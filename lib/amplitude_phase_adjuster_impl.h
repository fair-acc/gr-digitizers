/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_AMPLITUDE_PHASE_ADJUSTER_IMPL_H
#define INCLUDED_DIGITIZERS_AMPLITUDE_PHASE_ADJUSTER_IMPL_H

#include <digitizers/amplitude_phase_adjuster.h>

namespace gr {
  namespace digitizers {

    class amplitude_phase_adjuster_impl : public amplitude_phase_adjuster
    {
     private:
      double d_ampl;
      double d_phi;
      double d_phi_fq_factor;

     public:
      amplitude_phase_adjuster_impl(double ampl_usr, double phi_usr, double phi_fq_usr);
      ~amplitude_phase_adjuster_impl();

      int work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items);

      void update_design(double amplitude_usr,
        double phi_usr,
        double phi_fq_usr);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_AMPLITUDE_PHASE_ADJUSTER_IMPL_H */

