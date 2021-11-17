/* -*- c++ -*- */
/*
 * Copyright 2021 Fair.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DIGITIZERS_39_POWER_CALC_IMPL_H
#define INCLUDED_DIGITIZERS_39_POWER_CALC_IMPL_H

#include <digitizers_39/power_calc.h>

namespace gr {
  namespace digitizers_39 {

    class power_calc_impl : public power_calc
    {
     private:
        double d_alpha, d_beta, d_avg_u, d_avg_i;
        float d_phi;

     public:
      power_calc_impl(double alpha = 0.0000001); // 100n
      ~power_calc_impl() override;

      void calc_active_power(float* out, float* in_u, float* in_i, float* phi_out, int noutput_items);
      void calc_reactive_power(float* out, float* in_u, float* in_i, float* phi_out, int noutput_items);
      void calc_apparent_power(float* out, float* in_u, float* in_i, int noutput_items);
      void calc_phi(float* phi_out, const gr_complex* u_in, const gr_complex* i_in, int noutput_items);
      //void calc_rms(float* rms_out, float* mag_in, int noutput_items);
      //void calc_rms(float* output, const gr_complex* input, int noutput_items);
      void calc_rms_u(float* output, const gr_complex* input, int noutput_items);
      void calc_rms_i(float* output, const gr_complex* input, int noutput_items);
      void magnitude_generic(float* mag_in, const gr_complex* val_in, int noutput_items);

      void set_alpha(double alpha) override; //step-length

      // Where all the action really happens
      int work(
              int noutput_items,
              gr_vector_const_void_star &input_items,
              gr_vector_void_star &output_items
      );
    };

  } // namespace digitizers_39
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_39_POWER_CALC_IMPL_H */

