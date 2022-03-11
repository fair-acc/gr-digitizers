/* -*- c++ -*- */
/*
 * Copyright 2022 me.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/attributes.h>
#include <pulsed_power_daq/power_calc_ff.h>
#include "power_calc_ff_impl.h"
#include <boost/test/unit_test.hpp>

namespace gr {
namespace pulsed_power_daq {

BOOST_AUTO_TEST_SUITE(power_calc_ff_testing);

bool isSimilar(float first, float second, float decimals){
        float difference = 1/decimals;
        return abs(first-second) < difference;
}


BOOST_AUTO_TEST_CASE(test_power_calc_ff_Calc_active_and_reactive_power)
{
        power_calc_ff_impl* calc_block = new power_calc_ff_impl(0.0001);
        float out_active_float[1] = {0};
        float* out_active = out_active_float;
        float out_reactive_float[1] = {0};
        float* out_reactive = out_reactive_float;
        float float_voltage[1] = {1};
        float* voltage = float_voltage;
        float float_current[1] = {1};
        float* current = float_current;
        float float_phi_out[1] = {0};
        float* phi_out = float_phi_out;
        int noutput_items = 1;
        calc_block->calc_active_power (out_active, voltage, current, phi_out, noutput_items);
        calc_block->calc_reactive_power (out_reactive, voltage, current, phi_out, noutput_items);
        BOOST_CHECK (out_active[0] == (float)(1 * 1 * cos(0)));
        BOOST_CHECK (out_reactive[0] == (float)(1 * 1 * sin(0)));


        //TODO: what to expect from NaN?
        //right now we expect NaN

        /**
         * checking for one input being NaN
         */
        voltage[0] = 1;
        current[0] = 1;
        voltage[0] = nanf("");
        calc_block->calc_active_power (out_active, voltage, current, phi_out, noutput_items);
        calc_block->calc_reactive_power (out_reactive, voltage, current, phi_out, noutput_items);
        BOOST_CHECK (isnan(out_active[0]));
        BOOST_CHECK (isnan(out_reactive[0]));

        voltage[0] = 1;
        current[0] = nanf("");
        phi_out[0] = 0;
        calc_block->calc_active_power (out_active, voltage, current, phi_out, noutput_items);
        calc_block->calc_reactive_power (out_reactive, voltage, current, phi_out, noutput_items);
        BOOST_CHECK (isnan(out_active[0]));
        BOOST_CHECK (isnan(out_reactive[0]));

        voltage[0] = 1;
        current[0] = 1;
        phi_out[0] = nanf("");
        calc_block->calc_active_power (out_active, voltage, current, phi_out, noutput_items);
        calc_block->calc_reactive_power (out_reactive, voltage, current, phi_out, noutput_items);
        BOOST_CHECK (isnan(out_active[0]));
        BOOST_CHECK (isnan(out_reactive[0]));

        /**
         * checking for all inputs being NaN
         */
        voltage[0] = nanf("");
        current[0] = nanf("");
        phi_out[0] = nanf("");
        calc_block->calc_active_power (out_active, voltage, current, phi_out, noutput_items);
        calc_block->calc_reactive_power (out_reactive, voltage, current, phi_out, noutput_items);
        BOOST_CHECK (isnan(out_active[0]));
        BOOST_CHECK (isnan(out_reactive[0]));
}

BOOST_AUTO_TEST_CASE(test_power_calc_ff_Calc_apparent_power)
{
        power_calc_ff_impl* calc_block = new power_calc_ff_impl(0.0001);
        float out_float[1] = {0};
        float* out_apparent = out_float;
        float float_voltage[1] = {2};
        float* voltage = float_voltage;
        float float_current[1] = {2};
        float* current = float_current;
        int noutput_items = 1;
        calc_block->calc_apparent_power(out_apparent, voltage, current, noutput_items);
        BOOST_CHECK(out_apparent[0] == 4.f);

        voltage[0] = nanf("");
        current[0] = 2;
        calc_block->calc_apparent_power(out_apparent, voltage, current, noutput_items);
        BOOST_CHECK(isnan(out_apparent[0]));

        voltage[0] = 2;
        current[0] = nanf("");
        calc_block->calc_apparent_power(out_apparent, voltage, current, noutput_items);
        BOOST_CHECK(isnan(out_apparent[0]));

        voltage[0] = nanf("");
        current[0] = nanf("");
        calc_block->calc_apparent_power(out_apparent, voltage, current, noutput_items);
        BOOST_CHECK(isnan(out_apparent[0]));
}

BOOST_AUTO_TEST_CASE(test_power_calc_ff_Calc_rms)
{
        float alpha = 0.0001;
        power_calc_ff_impl* calc_block = new power_calc_ff_impl(alpha);
        float out_u_float[1] = {0};
        float* out_rms_u = out_u_float;
        float out_i_float[1] = {0};
        float* out_rms_i = out_i_float;
        float float_value[1] = {0};
        float* value = float_value;
        int noutput_items = 1;

        //test 0
        calc_block->calc_rms_u(out_rms_u, value, noutput_items);
        calc_block->calc_rms_i(out_rms_i, value, noutput_items);
        BOOST_CHECK(out_rms_u[0] == 0.f);
        BOOST_CHECK(out_rms_i[0] == 0.f);
        calc_block->calc_rms_u(out_rms_u, value, noutput_items);
        calc_block->calc_rms_i(out_rms_i, value, noutput_items);
        BOOST_CHECK(out_rms_u[0] == 0.f);
        BOOST_CHECK(out_rms_i[0] == 0.f);

        //test 100
        float curValue = 100;
        value[0] = curValue;
        calc_block->calc_rms_u(out_rms_u, value, noutput_items);
        calc_block->calc_rms_i(out_rms_i, value, noutput_items);
        BOOST_CHECK(out_rms_u[0] == sqrt(((1.f-alpha)*0 + alpha*curValue*curValue)));
        BOOST_CHECK(out_rms_i[0] == sqrt(((1.f-alpha)*0 + alpha*curValue*curValue)));

        float avg_rms_u = ((1.f-alpha)*0 + alpha*curValue*curValue);
        float avg_rms_i = ((1.f-alpha)*0 + alpha*curValue*curValue);
        for (int i = 0; i < 100; i++)
        {
                value[0] = curValue;
                calc_block->calc_rms_u(out_rms_u, value, noutput_items);
                calc_block->calc_rms_i(out_rms_i, value, noutput_items);
                BOOST_CHECK(isSimilar(out_rms_u[0], sqrt(((1.f-alpha)*avg_rms_u + alpha*curValue*curValue)), 7));
                BOOST_CHECK(isSimilar(out_rms_i[0], sqrt(((1.f-alpha)*avg_rms_i + alpha*curValue*curValue)), 7));
                avg_rms_u = ((1-alpha)*avg_rms_u + alpha*curValue*curValue);
                avg_rms_i = ((1-alpha)*avg_rms_i + alpha*curValue*curValue);
        }
}
//TODO: test phi phase correction
BOOST_AUTO_TEST_SUITE_END();
    } /* namespace pulsed_power_daq */
} /* namespace gr */
