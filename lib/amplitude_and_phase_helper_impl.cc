/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "amplitude_and_phase_helper_impl.h"

namespace gr {
  namespace digitizers {

    amplitude_and_phase_helper::sptr
    amplitude_and_phase_helper::make()
    {
      return gnuradio::get_initial_sptr
        (new amplitude_and_phase_helper_impl());
    }

    amplitude_and_phase_helper_impl::amplitude_and_phase_helper_impl()
      : gr::sync_block("amplitude_and_phase_helper",
              gr::io_signature::make(2, 2, sizeof(gr_complex)),
              gr::io_signature::make(1, 1, sizeof(gr_complex)))
    {}

    amplitude_and_phase_helper_impl::~amplitude_and_phase_helper_impl()
    {
    }

    int
    amplitude_and_phase_helper_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const gr_complex *inSig = (const gr_complex *) input_items[0];
      const gr_complex *inRef = (const gr_complex *) input_items[1];
      gr_complex *out = (gr_complex *) output_items[0];

      for(int i = 0; i < noutput_items; i ++) {
        out[i].real(inSig[i].imag() * inRef[i].imag());
        out[i].imag(inSig[i].imag() * inRef[i].real());
      }

      return noutput_items;
    }

  } /* namespace digitizers */
} /* namespace gr */

