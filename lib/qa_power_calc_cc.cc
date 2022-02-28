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

// /*!
//      * \brief sets up all variables necessary for a simple test. 
//      * Will be called upon every time you run a test case
//      */
// class power_calc_cc_fixture
// {
// public:
//     power_calc_cc_impl* calc_block;
//     float* out;
//     float* voltage; 
//     float* current; 
//     float* phi_out;
//     int noutput_items;

    
//     explicit power_calc_cc_fixture()
//     {
//         calc_block = new power_calc_cc_impl(0.0001);
//         float out_float[1] = {0};
//         out = out_float;
//         float float_voltage[1] = {1};
//         voltage = float_voltage;
//         float float_current[1] = {1};
//         current = float_current;
//         float float_phi_out[1] = {0};
//         phi_out = float_phi_out;
//         noutput_items = 1;
//     }
//     ~power_calc_cc_fixture()
//     {
//         delete calc_block;
//         delete out;
//         delete voltage; 
//         delete current; 
//         delete phi_out;
//     }
// };

// BOOST_FIXTURE_TEST_SUITE(power_calc_cc_fixture_testing, power_calc_cc_fixture);
// BOOST_AUTO_TEST_CASE(test_power_calc_cc_Calc_active_power_Basic_input_fixture)
// {//example for fixture testing, which allows for using a preset for each test
// //might have limited use
    
//     power_calc_cc_fixture fixture;
//     fixture.calc_block->calc_active_power(fixture.out, fixture.voltage, 
//                                         fixture.current, fixture.phi_out, 
//                                         fixture.noutput_items);
//     BOOST_CHECK(fixture.out[0] == (float)(1 * 1 * cos(0)));
// }
// BOOST_AUTO_TEST_SUITE_END();

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
