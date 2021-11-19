/* -*- c++ -*- */
/*
 * Copyright 2021 Fair.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/io_signature.h>
#include "power_calc_impl.h"
#include <cstdlib>
//#include <cmath>
#include <gnuradio/math.h>
#include <volk/volk.h>

namespace gr {
  namespace digitizers_39 {

    power_calc::sptr
    power_calc::make(double alpha)
    {
      return gnuradio::make_block_sptr<power_calc_impl>(alpha);
    }

    /*
     * The private constructor
     */
    power_calc_impl::power_calc_impl(double alpha)
      : gr::sync_block("power_calc",
              gr::io_signature::make(2 /* min inputs */, 2 /* max inputs */, sizeof(gr_complex)),
              gr::io_signature::make(4 /* min outputs */, 4 /*max outputs */, sizeof(float)))
    {
      set_alpha(alpha);
    }

    /*
     * Our virtual destructor.
     */
    power_calc_impl::~power_calc_impl()
    {
    }

    void power_calc_impl::calc_rms_u(float* output, const gr_complex* input, int noutput_items)
    {
        for (int i = 0; i < noutput_items; i++) 
        {
          double mag_sqrd = input[i].real() * input[i].real() + input[i].imag() * input[i].imag();
          d_avg_u = d_beta * d_avg_u + d_alpha * mag_sqrd;
          output[i] = sqrt(d_avg_u);
        }
    }

    void power_calc_impl::calc_rms_i(float* output, const gr_complex* input, int noutput_items)
    {
        for (int i = 0; i < noutput_items; i++) 
        {
          double mag_sqrd = input[i].real() * input[i].real() + input[i].imag() * input[i].imag();
          d_avg_i = d_beta * d_avg_i + d_alpha * mag_sqrd;
          output[i] = sqrt(d_avg_i);
        }
    }

    void power_calc_impl::calc_phi(float* phi_out, const gr_complex* u_in, const gr_complex* i_in, int noutput_items)
    {
        for (int i = 0; i < noutput_items; i++)
        { 
          float voltage_phi = (float)(gr::fast_atan2f(u_in[i]));
          float current_phi = (float)(gr::fast_atan2f(i_in[i]));
          float tmp = 0.0;

          tmp = voltage_phi - current_phi;

          // Phase correction
          if (tmp <= (M_PI_2 * -1))
          {
            phi_out[i] = tmp + (M_PI * 2);
          }
          else if (tmp >= M_PI_2)
          {
            phi_out[i] = tmp - (M_PI * 2);
          }
          else
          {
            phi_out[i] = tmp;
          }

          // Signed RMS
          double mag_sqrd = phi_out[i] * phi_out[i];
          d_avg_phi = d_beta * d_avg_phi + d_alpha * mag_sqrd;
          phi_out[i] = copysignl(sqrt(d_avg_phi), phi_out[i]);
        }
    }

    void power_calc_impl::calc_active_power(float* out, float* in_u, float* in_i, float* phi_out, int noutput_items)
    {
        for (int i = 0; i < noutput_items; i++) 
        {
          out[i] = (float)(in_u[i] * in_i[i] * cos(phi_out[i]));
        }
    }

    void power_calc_impl::calc_reactive_power(float* out, float* in_u, float* in_i, float* phi_out, int noutput_items)
    {
        for (int i = 0; i < noutput_items; i++) 
        {
          out[i] = (float)(in_u[i] * in_u[i] * sin(phi_out[i]));
        }
    }

    void power_calc_impl::calc_apparent_power(float* out, float* in_u, float* in_i, int noutput_items)
    {
        for (int i = 0; i < noutput_items; i++) 
        {
          out[i] = (float)(in_u[i] * in_i[i]);
        }
    }

    void power_calc_impl::set_alpha(double alpha)
    {
        d_alpha = alpha;
        d_beta = 1 - d_alpha;
        d_avg_u = 0;
        d_avg_i = 0;
        d_avg_phi = 0;
    }

    int
    power_calc_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const gr_complex* u_in =  (const gr_complex*)input_items[0];
      const gr_complex* i_in = (const gr_complex*)input_items[1];

      float* p_out = (float*)output_items[0];
      float* q_out = (float*)output_items[1];
      float* s_out = (float*)output_items[2];
      float* phi_out = (float*)output_items[3];

      float* rms_u = (float*)malloc(noutput_items*sizeof(float));
      float* rms_i = (float*)malloc(noutput_items*sizeof(float));

      calc_rms_u(rms_u, u_in, noutput_items);
      calc_rms_i(rms_i, i_in, noutput_items);

      calc_phi(phi_out, u_in, i_in, noutput_items);

      calc_active_power(p_out, rms_u, rms_i, phi_out, noutput_items);
      calc_reactive_power(q_out, rms_u, rms_i, phi_out, noutput_items);
      calc_apparent_power(s_out, rms_u, rms_i, noutput_items);

      free(rms_u);
      free(rms_i);

      return noutput_items;
    }

  } /* namespace digitizers_39 */
} /* namespace gr */

