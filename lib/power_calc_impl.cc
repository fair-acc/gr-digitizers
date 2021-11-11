/* -*- c++ -*- */
/*
 * Copyright 2021 Fair.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/io_signature.h>
#include "power_calc_impl.h"
#include <cmath>
#include <gnuradio/math.h>
#include <volk/volk.h>

namespace gr {
  namespace digitizers_39 {

    using input_type = gr_complex;
    using output_type = float;
    power_calc::sptr
    power_calc::make(double alpha)
    {
      return gnuradio::make_block_sptr<power_calc_impl>(
        alpha);
    }


    /*
     * The private constructor
     */
    power_calc_impl::power_calc_impl(double alpha)
      : gr::sync_block("power_calc",
              gr::io_signature::make(2 /* min inputs */, 2 /* max inputs */, sizeof(input_type)),
              gr::io_signature::make(4 /* min outputs */, 4 /*max outputs */, sizeof(output_type)))
    {
      set_alpha(alpha);
    }

    /*
     * Our virtual destructor.
     */
    power_calc_impl::~power_calc_impl()
    {
    }

    void power_calc_impl::set_alpha(double alpha)
    {
        d_alpha = alpha;
        d_beta = 1 - d_alpha;
        d_avg = 0;
    }

    void power_calc_impl::calc_phi(float* u, float* i, int noutput_items)
    {
        for (int i = 0; i < noutput_items; i++) 
          {

          }
    }

    void power_calc_impl::calc_active_power(float* p_out, float* rms_u, float* rms_i, int noutput_items)
    {
        for (int i = 0; i < noutput_items; i++) 
        {
          
        }
    }

    void power_calc_impl::calc_reactive_power(float* q_out, float* rms_u, float* rms_i, int noutput_items)
    {
        for (int i = 0; i < noutput_items; i++) 
        {
          
        }
    }

    void power_calc_impl::calc_apparent_power(float* s_out, float* rms_u, float* rms_i, int noutput_items)
    {
        for (int i = 0; i < noutput_items; i++) 
        {
          s_out[i] = rms_u[i] * rms_i[i];
        }
    }

    int
    power_calc_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const input_type *in = reinterpret_cast<const input_type*>(input_items[0]);
      output_type *out = reinterpret_cast<output_type*>(output_items[0]);

      const input_type* u_in = reinterpret_cast<const input_type*>(input_items[0]);
      const input_type* i_in = reinterpret_cast<const input_type*>(input_items[1]);

      output_type* p_out = reinterpret_cast<output_type*>(output_items[0]);
      output_type* q_out = reinterpret_cast<output_type*>(output_items[1]);
      output_type* s_out = reinterpret_cast<output_type*>(output_items[2]);
      output_type* phi_out = reinterpret_cast<output_type*>(output_items[3]);

      float* mag_u_in;
      float* mag_i_in;

      float* phi_u_in;
      float* phi_i_in;

      float* rms_u;
      float* rms_i;

      volk_32fc_magnitude_32f_u(mag_u_in, u_in, noutput_items);

      volk_32fc_magnitude_32f_u(mag_i_in, i_in, noutput_items);
      
      // The fast_atan2f is faster than Volk
      for (int i = 0; i < noutput_items; i++) {
          phi_u_in[i] = gr::fast_atan2f(u_in[i]);
          phi_i_in[i] = gr::fast_atan2f(i_in[i]);
      }
      
      // RMS
      for (int i = 0; i < noutput_items; i++) {

        // Voltage
        // RMS 
        double mag_sqrd_u = mag_u_in[i] * mag_u_in[i];
        d_avg = d_beta * d_avg + d_alpha * mag_sqrd_u;
        rms_u[i] = sqrt(d_avg);

        // Current
        // RMS 
        double mag_sqrd_i = mag_i_in[i] * mag_i_in[i];
        d_avg = d_beta * d_avg + d_alpha * mag_sqrd_i;
        rms_i[i] = sqrt(d_avg);
      }

      // calc_active_power(p_out, rms_u, rms_i, noutput_items);
      // calc_reactive_power(q_out, rms_u, rms_i, noutput_items);
      calc_apparent_power(s_out, rms_u, rms_i, noutput_items);
      return noutput_items;
    }

  } /* namespace digitizers_39 */
} /* namespace gr */

