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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "block_amplitude_and_phase_impl.h"
#include <gnuradio/filter/firdes.h>

namespace gr {
  namespace digitizers {

    block_amplitude_and_phase::sptr
    block_amplitude_and_phase::make(double samp_rate,
      double delay,
      int decim,
      double gain,
      double up_freq,
      double tr_width,
      int hilbert_window)
    {
      return gnuradio::get_initial_sptr
        (new block_amplitude_and_phase_impl(samp_rate, delay, decim, gain, up_freq, tr_width, hilbert_window));
    }

    /*
     * The private constructor
     */
    block_amplitude_and_phase_impl::block_amplitude_and_phase_impl(double samp_rate,
      double delay,
      int decim,
      double gain,
      double up_freq,
      double tr_width,
      int hilbert_window)
      : gr::hier_block2("block_amplitude_and_phase",
          gr::io_signature::make(2, 2, sizeof(float)),
          gr::io_signature::make(2, 2, sizeof(float)))
    {
      d_hil_sig = filter::hilbert_fc::make(hilbert_window);
      d_hil_ref = filter::hilbert_fc::make(hilbert_window);
      d_del = blocks::delay::make(sizeof(float), static_cast<int>(delay*samp_rate));
      d_help = amplitude_and_phase_helper::make();
      d_low_pass = filter::fir_filter_ccf::make(decim, filter::firdes::low_pass(gain, samp_rate, up_freq, tr_width));
      d_com2mp = blocks::complex_to_magphase::make(1);
      d_rad2deg = blocks::multiply_const_ff::make((180.0/M_PI), 1);
      /*Connections*/
      //signal
      connect(self(), 0, d_del, 0);
      connect(d_del, 0, d_hil_sig, 0);
      connect(d_hil_sig, 0, d_help, 0);
      //reference
      connect(self(), 1, d_hil_ref, 0);
      connect(d_hil_ref, 0, d_help , 1);
      //low pass
      connect(d_help, 0, d_low_pass, 0);
      connect(d_low_pass, 0, d_com2mp, 0);
      connect(d_com2mp, 1, d_rad2deg, 0);
      //output
      connect(d_com2mp, 0, self(), 0);
      connect(d_rad2deg, 0, self(), 1);
    }

    /*
     * Our virtual destructor.
     */
    block_amplitude_and_phase_impl::~block_amplitude_and_phase_impl()
    {
    }

    void
    block_amplitude_and_phase_impl::update_design(double samp_rate,
      double delay,
      int decim,
      double gain,
      double up_freq,
      double tr_width)
    {
      d_low_pass->set_taps(filter::firdes::low_pass(gain, samp_rate, up_freq, tr_width));
      d_low_pass->set_decimation(decim);
      d_del->set_dly(static_cast<int>(delay*samp_rate));
    }
  } /* namespace digitizers */
} /* namespace gr */

