/* -*- c++ -*- */
/*
 * Copyright 2021 fair.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PULSED_POWER_DAQ_POWER_CALC_FF_IMPL_H
#define INCLUDED_PULSED_POWER_DAQ_POWER_CALC_FF_IMPL_H

#include <pulsed_power_daq/power_calc_mul_ph_ff.h>
#include <cstdlib>
#include <gnuradio/math.h>
#include <volk/volk.h>

namespace gr {
  namespace pulsed_power_daq {

    class power_calc_mul_ph_ff_impl : public power_calc_mul_ph_ff
    {
      private:
        double d_alpha, d_beta, d_avg_u, d_avg_i, d_avg_phi, d_last_valid_phi;

      public:
        power_calc_mul_ph_ff_impl(double alpha = 0.0000001); // 100n
        ~power_calc_mul_ph_ff_impl() override;
        void calc_active_power(float* out, float* voltage, float* current, float* phi_out, int noutput_items);
        void calc_reactive_power(float* out, float* voltage, float* current, float* phi_out, int noutput_items);
        void calc_apparent_power(float* out, float* voltage, float* current, int noutput_items);
        void calc_phi_phase_correction(float* phi_out, const float* dalta_phi, int noutput_items);
        void calc_rms_u(float* output, const float* input, int noutput_items);
        void calc_rms_i(float* output, const float* input, int noutput_items);
        void get_timestamp_ms(float* out);
        void calc_acc_val_active_power(float* p_out_acc, float* p_out_1, float* p_out_2, float* p_out_3, int noutput_items);
        void calc_acc_val_reactive_power(float* q_out_acc, float* q_out_1, float* q_out_2, float* q_out_3, int noutput_items);
        void calc_acc_val_apparent_power(float* out, float* acc_active_power, float* acc_reactive_power, int noutput_items);
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

#endif /* INCLUDED_DIGITIZERS_39_POWER_CALC_FF_IMPL_H */
