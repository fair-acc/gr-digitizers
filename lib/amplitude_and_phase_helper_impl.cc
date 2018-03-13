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

