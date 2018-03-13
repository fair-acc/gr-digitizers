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
      blocks::delay::sptr d_del;
      filter::hilbert_fc::sptr d_hil_sig;
      filter::hilbert_fc::sptr  d_hil_ref;
      amplitude_and_phase_helper::sptr  d_help;
      filter::fir_filter_ccf::sptr  d_low_pass;
      blocks::complex_to_magphase::sptr d_com2mp;
      blocks::multiply_const_ff::sptr d_rad2deg;
     public:
      block_amplitude_and_phase_impl(double samp_rate,
        double delay,
        int decim,
        double gain,
        double up_freq,
        double tr_width,
        int hilbert_window);

      ~block_amplitude_and_phase_impl();

      void update_design(double samp_rate,
        double delay,
        int decim,
        double gain,
        double up_freq,
        double tr_width);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_BLOCK_AMPLITUDE_AND_PHASE_IMPL_H */

