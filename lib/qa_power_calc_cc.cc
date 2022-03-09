/* -*- c++ -*- */
/*
 * Copyright 2022 me.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/attributes.h>
#include <pulsed_power_daq/power_calc_cc.h>
#include "power_calc_cc_impl.h"
#include <boost/test/unit_test.hpp>

namespace gr {
namespace pulsed_power_daq {

BOOST_AUTO_TEST_SUITE(power_calc_cc_testing);
BOOST_AUTO_TEST_CASE(test_power_calc_cc_Calc_active_power_Basic_input)
{
        power_calc_cc_impl* calc_block = new power_calc_cc_impl(0.0001);
        float out_float[1] = {0};
        float* out = out_float;
        float float_voltage[1] = {1};
        float*voltage = float_voltage;
        float float_current[1] = {1};
        float*current = float_current;
        float float_phi_out[1] = {0};
        float*phi_out = float_phi_out;
        int noutput_items = 1;
        calc_block->calc_active_power(out, voltage, current, phi_out, noutput_items);
        BOOST_CHECK(out[0] == (float)(1 * 1 * cos(0)));

}
BOOST_AUTO_TEST_SUITE_END();
    } /* namespace pulsed_power_daq */
} /* namespace gr */
