/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_BLOCK_AMPLITUDE_AND_PHASE_IMPL_H
#define INCLUDED_DIGITIZERS_BLOCK_AMPLITUDE_AND_PHASE_IMPL_H

#include <digitizers/block_amplitude_and_phase.h>

#include <gnuradio/blocks/complex_to_magphase.h>
#include <gnuradio/blocks/delay.h>
#include <gnuradio/filter/hilbert_fc.h>
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/blocks/multiply_const_ff.h>
#include "digitizers/amplitude_and_phase_helper.h"

namespace gr {
  namespace digitizers {

    class block_amplitude_and_phase_impl : public block_amplitude_and_phase
    {
     private:
      double d_samp_rate;
      blocks::delay::sptr d_del;
      filter::hilbert_fc::sptr d_hil_sig;
      filter::hilbert_fc::sptr  d_hil_ref;
      amplitude_and_phase_helper::sptr  d_help;
      filter::fir_filter_ccf::sptr  d_low_pass;
     public:
      block_amplitude_and_phase_impl(double samp_rate,
        double delay,
        int decim,
        double gain,
        double up_freq,
        double tr_width,
        int hilbert_window);

      ~block_amplitude_and_phase_impl();

      void update_design(double delay,
        double gain,
        double up_freq,
        double tr_width);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_BLOCK_AMPLITUDE_AND_PHASE_IMPL_H */

