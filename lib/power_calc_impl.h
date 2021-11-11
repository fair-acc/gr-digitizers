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
        double d_alpha, d_beta, d_avg;
        float d_phi;

     public:
      power_calc_impl(double alpha = 0.0001);
      ~power_calc_impl();

      void calc_active_power(float* p_out, float* rms_u, float* rms_ih, int noutput_items);
      void calc_reactive_power(float* q_out, float* rms_u, float* rms_ih, int noutput_items);
      void calc_apparent_power(float* s_out, float* rms_u, float* rms_ih, int noutput_items);
      void calc_phi(float* u, float* i, int noutput_items);
      void set_alpha(double alpha); //step-length

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

