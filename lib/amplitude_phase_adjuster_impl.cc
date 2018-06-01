/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "amplitude_phase_adjuster_impl.h"

namespace gr {
  namespace digitizers {

    amplitude_phase_adjuster::sptr
    amplitude_phase_adjuster::make(double ampl_usr, double phi_usr, double phi_fq_usr)
    {
      return gnuradio::get_initial_sptr
        (new amplitude_phase_adjuster_impl(ampl_usr, phi_usr, phi_fq_usr));
    }

    /*
     * The private constructor
     */
    amplitude_phase_adjuster_impl::amplitude_phase_adjuster_impl(double ampl_usr, double phi_usr, double phi_fq_usr)
      : gr::sync_block("amplitude_phase_adjuster",
              gr::io_signature::make(3, 3, sizeof(float)),
              gr::io_signature::make(2, 2, sizeof(float))),
        d_ampl(ampl_usr),
        d_phi(phi_usr),
        d_phi_fq_factor(phi_fq_usr)
    {}

    amplitude_phase_adjuster_impl::~amplitude_phase_adjuster_impl()
    {
    }

    int
    amplitude_phase_adjuster_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const float *am_i = (const float *) input_items[0];
      const float *ph_i = (const float *) input_items[1];
      const float *fq_ref = (const float *) input_items[2];
      float *am_o = (float *) output_items[0];
      float *ph_o = (float *) output_items[1];
      for(int i = 0; i < noutput_items; i++) {
        am_o[i] = am_i[i] * d_ampl;
        double new_phase = (ph_i[i]- (d_phi + d_phi_fq_factor * fq_ref[i]));

        //fix from -180 -> 180 deg.
        if(new_phase<-180.0) { new_phase = 360.0 + new_phase;}
        else if(new_phase > 180.0) { new_phase = -360.0 - new_phase; }
        ph_o[i] = new_phase;
      }

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }
    void
    amplitude_phase_adjuster_impl::update_design(double amplitude_usr,
      double phi_usr,
      double phi_fq_usr)
    {
      d_ampl = amplitude_usr;
      d_phi = phi_usr;
      d_phi_fq_factor = phi_fq_usr;
    }

  } /* namespace digitizers */
} /* namespace gr */

