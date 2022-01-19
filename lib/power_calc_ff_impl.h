/* -*- c++ -*- */
/*
 * Copyright 2022 fair.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PULSED_POWER_DAQ_POWER_CALC_FF_IMPL_H
#define INCLUDED_PULSED_POWER_DAQ_POWER_CALC_FF_IMPL_H

#include <pulsed_power_daq/power_calc_ff.h>
#include <cstdlib>
#include <gnuradio/math.h>
#include <volk/volk.h>

namespace gr {
namespace pulsed_power_daq {

class power_calc_ff_impl : public power_calc_ff
{
private:
    double d_alpha, d_beta, d_avg_u, d_avg_i, d_avg_phi;


public:
    power_calc_ff_impl(double alpha);
    ~power_calc_ff_impl();

        void calc_active_power(float* out, float* voltage, float* current, float* phi_out, int noutput_items);
        void calc_reactive_power(float* out, float* voltage, float* current, float* phi_out, int noutput_items);
        void calc_apparent_power(float* out, float* voltage, float* current, int noutput_items);
        void calc_phi(float* phi_out, const float* voltage_phi, const float* current_phi, int noutput_items);
        void calc_rms_u(float* output, const float* input, int noutput_items);
        void calc_rms_i(float* output, const float* input, int noutput_items);
        void get_timestamp_ms(float* out);
        // void calc_signed_rms_phase(float* output, float * input, int noutput_items);
        // void calc_phase_correction(float* output, float * input, int noutput_items);

        void set_alpha(double alpha) override; //step-length

        // Where all the action really happens
        int work(
                int noutput_items,
                gr_vector_const_void_star &input_items,
                gr_vector_void_star &output_items
        );
};

} // namespace pulsed_power_daq
} // namespace gr

#endif /* INCLUDED_PULSED_POWER_DAQ_POWER_CALC_FF_IMPL_H */
