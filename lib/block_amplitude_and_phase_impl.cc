/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
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
          gr::io_signature::make(1, 1, sizeof(gr_complex))),
       d_samp_rate(samp_rate)
    {
      d_hil_sig = filter::hilbert_fc::make(hilbert_window);
      d_hil_ref = filter::hilbert_fc::make(hilbert_window);
      d_help = amplitude_and_phase_helper::make();
      d_low_pass = filter::fir_filter_ccf::make(decim, filter::firdes::low_pass(gain, samp_rate, up_freq, tr_width));
      /*Connections*/
      //signal
      connect(self(), 0, d_hil_sig, 0);
      connect(d_hil_sig, 0, d_help, 0);
      //reference
      connect(self(), 1, d_hil_ref, 0);
      connect(d_hil_ref, 0, d_help , 1);
      //low pass
      connect(d_help, 0, d_low_pass, 0);
      connect(d_low_pass, 0, self(), 0);
    }

    /*
     * Our virtual destructor.
     */
    block_amplitude_and_phase_impl::~block_amplitude_and_phase_impl()
    {
    }

    void
    block_amplitude_and_phase_impl::update_design(double delay,
            double gain, double up_freq, double tr_width)
    {
      d_low_pass->set_taps(filter::firdes::low_pass(gain, d_samp_rate, up_freq, tr_width));
      d_del->set_dly(static_cast<int>(delay * d_samp_rate));
    }
  } /* namespace digitizers */
} /* namespace gr */

