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
      return gnuradio::make_block_sptr<power_calc_impl>(
        alpha);
    }

    /*
     * The private constructor
     */
    power_calc_impl::power_calc_impl(double alpha)
      : gr::sync_block("power_calc",
              gr::io_signature::make(2 /* min inputs */, 2 /* max inputs */, sizeof(gr_complex)),
              gr::io_signature::make(8 /* min outputs */, 8 /*max outputs */, sizeof(float)))
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

    void power_calc_impl::magnitude_generic(float* mag_in, const gr_complex* val_in, int noutput_items)
    {
      // gr_complex* val = const_cast<gr_complex*>(val_in);
      const float* complexVectorPtr = (float*)val_in;
      float* magnitudeVectorPtr = mag_in;
      unsigned int number = 0;
      // std::cout << "Calc ONE Square root: \n";
      // std::cout << noutput_items << "\n";
      for(number = 0; number < noutput_items; number++)
      {
        const float real = *complexVectorPtr++;
        const float imag = *complexVectorPtr++;
        //float product = (float)((real*real) + (imag*imag));
        //std::cout << "Square root " << number;
        //std::cout << product << '\n';
        *magnitudeVectorPtr++ = sqrtf((real*real) + (imag*imag));
        //*magnitudeVectorPtr++ = sqrtf(15.4863);
        //std::cout << sqrtf(15.4863) << '\n';
      }
    }

    void power_calc_impl::calc_rms(float* rms_out, float* mag_in, int noutput_items)
    {
        for (int i = 0; i < noutput_items; i++) 
        {
          double mag_sqrd = mag_in[i] * mag_in[i];
          d_avg = d_beta * d_avg + d_alpha * mag_sqrd;
          rms_out[i] = sqrtf(d_avg);
        }
    }

    void power_calc_impl::calc_phi(float* phi_out, const gr_complex* u_in, const gr_complex* i_in, int noutput_items)
    {
        for (int i = 0; i < noutput_items; i++) 
        { 
          float vultage_phi = (float)(gr::fast_atan2f(u_in[i]));
          float current_phi = (float)(gr::fast_atan2f(i_in[i]));
          phi_out[i] = vultage_phi - current_phi;
        }
    }

    void power_calc_impl::calc_active_power(float* p_out, float* rms_u, float* rms_i, float* phi_out, int noutput_items)
    {
        for (int i = 0; i < noutput_items; i++) 
        {
          p_out[i] = (float)(rms_u[i] * rms_i[i] * cos(phi_out[i]));
        }
    }

    void power_calc_impl::calc_reactive_power(float* q_out, float* rms_u, float* rms_i, float* phi_out, int noutput_items)
    {
        for (int i = 0; i < noutput_items; i++) 
        {
          q_out[i] = (float)(rms_u[i] * rms_i[i] * sin(phi_out[i]));
        }
    }

    void power_calc_impl::calc_apparent_power(float* s_out, float* rms_u, float* rms_i, int noutput_items)
    {
        for (int i = 0; i < noutput_items; i++) 
        {
          s_out[i] = (float)(rms_u[i] * rms_i[i]);
        }
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

      float* mag_u_in = (float*)output_items[4];// (float*)malloc(noutput_items*sizeof(float));
      float* mag_i_in = (float*)output_items[5];// (float*)malloc(noutput_items*sizeof(float));

      float* rms_u = (float*)output_items[6];// (float*)malloc(noutput_items*sizeof(float));
      float* rms_i = (float*)output_items[7];// (float*)malloc(noutput_items*sizeof(float));

      //magnitude_generic(mag_u_in, u_in, noutput_items);
      //magnitude_generic(mag_i_in, i_in, noutput_items);
      volk_32fc_magnitude_32f_u(mag_u_in, u_in, noutput_items);
      volk_32fc_magnitude_32f_u(mag_i_in, i_in, noutput_items);

      calc_rms(rms_u, mag_u_in, noutput_items);
      calc_rms(rms_i, mag_i_in, noutput_items);

      calc_phi(phi_out, u_in, i_in, noutput_items);

      calc_active_power(p_out, rms_u, rms_i, phi_out, noutput_items);
      calc_reactive_power(q_out, rms_u, rms_i, phi_out, noutput_items);
      calc_apparent_power(s_out, rms_u, rms_i, noutput_items);

      // free(mag_u_in);
      // free(mag_i_in);
      // free(rms_u);
      // free(rms_i);

      return noutput_items;
    }

  } /* namespace digitizers_39 */
} /* namespace gr */

