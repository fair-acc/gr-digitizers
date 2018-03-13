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
#include "peak_detector_impl.h"
#include "utils.h"

namespace gr {
  namespace digitizers {

    peak_detector::sptr
    peak_detector::make(double samp_rate, int vec_len, int start_bin, int end_bin, int proximity)
    {
      return gnuradio::get_initial_sptr
        (new peak_detector_impl(samp_rate, vec_len, start_bin, end_bin, proximity));
    }

    /*
     * The private constructor
     */
    peak_detector_impl::peak_detector_impl(double samp_rate,
        int vec_len,
        int start_bin,
        int end_bin,
        int proximity)
      : gr::block("peak_detector",
              gr::io_signature::make(2, 2, sizeof(float) * vec_len),
              gr::io_signature::make(2, 2, sizeof(float))),
              d_vec_len(vec_len), d_start_bin(start_bin),
              d_end_bin(end_bin), d_prox(proximity),
              d_freq(samp_rate)
    {}

    /*
     * Our virtual destructor.
     */
    peak_detector_impl::~peak_detector_impl()
    {
    }

    void
    peak_detector_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      ninput_items_required[0] = 1;
    }

    int
    peak_detector_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const float *actual = (const float *) input_items[0];
      const float *filtered = (const float *) input_items[1];
      float *max_sig = (float *) output_items[0];
      float *width_sig = (float *) output_items[1];
      if(ninput_items[0] <= 0 ||ninput_items[1] <= 0 || noutput_items < 1){
        return 0;
      }

      //find filtered maximum
      float max_fil = filtered[d_start_bin];
      int max_fil_i = d_start_bin;
      for(int i = d_start_bin + 1; i <= d_end_bin; i++) {
        if(max_fil<filtered[i]) {
          max_fil = filtered[i];
          max_fil_i = i;
        }
      }
      //find actual maximum in the proximity of the averaged maximum
      //initialize to something smaller than actual max.
      float max = max_fil;
      int max_i = max_fil_i;
      for(int i = 0; i< d_prox; i++) {
        if(max_fil_i+i < d_vec_len &&
            max<actual[max_fil_i + i]){
          max = actual[max_fil_i + i];
          max_i = max_fil_i + i;
        }
        if(max_fil_i-i >= 0 &&
            max<actual[max_fil_i - i]){
          max = actual[max_fil_i - i];
          max_i = max_fil_i - i;
        }
      }

      //find FWHM for stdev approx.
      double hm=actual[max_i]/2.0;
      int whm_i= max_i;
      for(int i =0; i<d_vec_len && whm_i == max_i; i++){
        if(max_i+i < d_vec_len &&
            hm > actual[max_i+i]){ whm_i = max_i+i; }
        if(max_i-i >= 0 &&
            hm > actual[max_i-i]){ whm_i = max_i-i; }
      }
      double freq_whm = 0.0;
      if(whm_i > max_i) {
        double a = actual[whm_i]-actual[whm_i-1];
        double b = actual[whm_i-1];
        freq_whm= (b + (hm-b))/a;
      }
      if(whm_i < max_i) {
        double a = actual[whm_i+1]-actual[whm_i];
        double b = actual[whm_i];
        freq_whm= (b + (hm-b))/a;
      }

      //fix width to half maximum from bin count to frequency window
      freq_whm *= d_freq/(d_vec_len);

      max_sig[0] = (max_i * d_freq) / (2.0 * d_vec_len);
      width_sig[0] = freq_whm * whm2stdev;
      consume_each (1);
      // Tell runtime system how many output items we produced.
      return 1;
    }

  } /* namespace digitizers */
} /* namespace gr */

