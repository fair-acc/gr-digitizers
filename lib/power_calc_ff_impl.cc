/* -*- c++ -*- */
/*
 * Copyright 2022 GSI.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "power_calc_ff_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace pulsed_power_daq {

using input_type = float;
using output_type = float;
power_calc_ff::sptr power_calc_ff::make(double alpha)
{
    return gnuradio::make_block_sptr<power_calc_ff_impl>(alpha);
}


/*
 * The private constructor
 */
power_calc_ff_impl::power_calc_ff_impl(double alpha)
    : gr::sync_block("power_calc_ff",
                     gr::io_signature::make(
                         2 /* min inputs */, 2 /* max inputs */, sizeof(input_type)),
                     gr::io_signature::make(
                         4 /* min outputs */, 4 /*max outputs */, sizeof(output_type)))
{
}

/*
 * Our virtual destructor.
 */
power_calc_ff_impl::~power_calc_ff_impl() {}

    /**
     * @brief Generates a timestamp (per routine) of milliseconds since New York 1970 UTC and splits them into to floats | convert back uint64_t int64 = ((long long) out[0] << 32) | out[1];
     * 
     * @param out The pointer containing 2 values, the first 4 byte (high) and the last 4 byte (low) | out[0]=>high; out[1]=>low
     */
    void power_calc_ff_impl::get_timestamp_ms(float* out)
    {
      uint64_t milliseconds_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
      out[0] = (float)(milliseconds_since_epoch >> 32);
      out[1] = (float)(milliseconds_since_epoch);
    }

    /**
     * @brief Calculates RMS for voltage for the number of items currently available
     * 
     * @param output The input pointer for peak voltage, over number of input items
     * @param input The input pointer for raw voltage, over number of input items
     * @param noutput_items The samples currently available for cumputation
     */
    void power_calc_ff_impl::calc_rms_u(float* output, const float* input, int noutput_items)
    {
        //TODO: wir rechenn hier alle inputs ein. Wenn äquivalent zu power_calc_cc müsste der imaginäre Teil übersprungen werden
        for (int i = 0; i < noutput_items; i++) 
        {
          double mag_sqrd = input[i] * input[i];// + input[i].imag() * input[i].imag();
          d_avg_u = d_beta * d_avg_u + d_alpha * mag_sqrd;
          output[i] = sqrt(d_avg_u);
        }
    }

    /**
     * @brief Calculates RMS for current for the number of items currently available
     * 
     * @param output The input pointer for peak current, over number of input items
     * @param input The input pointer for raw current, over number of input items
     * @param noutput_items The samples currently available for cumputation
     */
    void power_calc_ff_impl::calc_rms_i(float* output, const float* input, int noutput_items)
    {
        for (int i = 0; i < noutput_items; i++) 
        {
          double mag_sqrd = input[i] * input[i];// + input[i].imag() * input[i].imag();
          d_avg_i = d_beta * d_avg_i + d_alpha * mag_sqrd;
          output[i] = sqrt(d_avg_i);
        }
    }

    /**
     * @brief Calculates Phi between voltage and current; Adds phase correction; Flattens the value | Phi = PHIu - PHIi | for the number of items currently available
     * 
     * @param phi_out The output pointer containing Phi over all concurrent Phi voltage and Phi current values
     * @param u_in The input pointer for raw voltage, over number of input items
     * @param i_in The input pointer for raw current, over number of input items
     * @param noutput_items The samples currently available for cumputation
     */
    void power_calc_ff_impl::calc_phi(float* phi_out, const float* voltage_phi, const float* current_phi, int noutput_items)
    {
        for (int i = 0; i < noutput_items; i++)
        { 
          // float voltage_phi = (float)(gr::fast_atan2f(u_in[i]));
          // float current_phi = (float)(gr::fast_atan2f(i_in[i]));
          float tmp = 0.0;

          tmp = voltage_phi[i] - current_phi[i];

          // std::cout << "post:" << '\n';
          // std::cout << tmp << '\n';

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
          // std::cout << "past:" << '\n';
          // std::cout << phi_out[i] << '\n';

          // Signed RMS
          // double mag_sqrd = phi_out[i] * phi_out[i];
          // d_avg_phi = d_beta * d_avg_phi + d_alpha * mag_sqrd;
          // phi_out[i] = copysignl(sqrt(d_avg_phi), phi_out[i]);

          // Single Pole IIR Filter
          d_avg_phi = d_alpha * phi_out[i] + d_beta * d_avg_phi;
          phi_out[i] = d_avg_phi;
        }
    }

    /**
     * @brief Calculates active power | P = RMSu * RMSi * cos(Phi) | for the number of items currently available
     * 
     * @param out The output pointer containing P over all concurrent RMS and Phi values
     * @param voltage The input pointer for peak voltage, over number of input items
     * @param current The input pointer for peak current, over number of input items
     * @param phi_out The input pointer for phi, over number of input items
     * @param noutput_items The samples currently available for cumputation
     */
    void power_calc_ff_impl::calc_active_power(float* out, float* voltage, float* current, float* phi_out, int noutput_items)
    {
        for (int i = 0; i < noutput_items; i++) 
        {
          out[i] = (float)(voltage[i] * current[i] * cos(phi_out[i]));
        }
    }

    /**
     * @brief Calculates reactive power | Q = RMSu * RMSi * sin(Phi) | for the number of items currently available
     * 
     * @param out The output pointer containing Q over all concurrent RMS and Phi values
     * @param voltage The input pointer for peak voltage, over number of input items
     * @param current The input pointer for peak current, over number of input items
     * @param phi_out The input pointer for phi, over number of input items
     * @param noutput_items The samples currently available for cumputation
     */
    void power_calc_ff_impl::calc_reactive_power(float* out, float* voltage, float* current, float* phi_out, int noutput_items)
    {
        for (int i = 0; i < noutput_items; i++) 
        {
          out[i] = (float)(voltage[i] * current[i] * sin(phi_out[i]));
        }
    }

    /**
     * @brief Calculates apperent power | S = RMSu * RMSi | for the number of items currently available
     * 
     * @param out The output pointer containing S over all concurrent RMS values
     * @param voltage The input pointer for peak voltage, over number of input items
     * @param current The input pointer for peak current, over number of input items
     * @param noutput_items The samples currently available for cumputation
     */
    void power_calc_ff_impl::calc_apparent_power(float* out, float* voltage, float* current, int noutput_items)
    {
        for (int i = 0; i < noutput_items; i++) 
        {
          out[i] = (float)(voltage[i] * current[i]);
        }
    }

    /**
     * @brief Sets global alpha, beta und average for all RMS calculations
     * 
     * @param alpha A value > 0 < 1
     */
    void power_calc_ff_impl::set_alpha(double alpha)
    {
        d_alpha = alpha; ///< impacts the "flattening" | default value 0.00001
        d_beta = 1 - d_alpha;
        d_avg_u = 0; ///< RMS average for voltage
        d_avg_i = 0; ///< RMS average for current
        d_avg_phi = 0; ///< RMS | single point iir filter average
    }

    /**
     * @brief Main | Core block routine
     * 
     * @param noutput_items The samples currently available for cumputation
     * @param input_items The item vector containing the input items
     * @param output_items  The item vector that will contain the output items
     * @return number of output items
     */
    int power_calc_ff_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const float* u_in =  (const float*)input_items[0];
      const float* i_in = (const float*)input_items[1];

      const float* u_phase_in =  (const float*)input_items[2];
      const float* i_phase_in = (const float*)input_items[3];

      float* p_out = (float*)output_items[0];
      float* q_out = (float*)output_items[1];
      float* s_out = (float*)output_items[2];
      float* phi_out = (float*)output_items[3];
      // float* timestamp_ms = (float*)output_items[4];
      
      float* rms_u = (float*)malloc(noutput_items*sizeof(float));
      float* rms_i = (float*)malloc(noutput_items*sizeof(float));

      float* voltage_phi = (float*)malloc(noutput_items*sizeof(float));
      float* current_phi = (float*)malloc(noutput_items*sizeof(float));

      calc_rms_u(rms_u, u_in, noutput_items);
      calc_rms_i(rms_i, i_in, noutput_items);

      volk_32f_atan_32f(voltage_phi, u_phase_in, noutput_items);
      volk_32f_atan_32f(current_phi, i_phase_in, noutput_items);

      calc_phi(phi_out, voltage_phi, current_phi, noutput_items);

      calc_active_power(p_out, rms_u, rms_i, phi_out, noutput_items);
      calc_reactive_power(q_out, rms_u, rms_i, phi_out, noutput_items);
      calc_apparent_power(s_out, rms_u, rms_i, noutput_items);

      //std::cout << s_out[0] << '\n';

      free(rms_u);
      free(rms_i);
      free(voltage_phi);
      free(current_phi);

      // get_timestamp_ms(timestamp_ms);

      // std::cout.precision(17);
      // std::cout << "Time:" << '\n';
      // std::cout << timestamp_ms[0] << '\n';
      // std::cout << timestamp_ms[1] << '\n';

      return noutput_items;
    }

} /* namespace pulsed_power_daq */
} /* namespace gr */
