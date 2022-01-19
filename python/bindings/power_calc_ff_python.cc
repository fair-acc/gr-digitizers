/*
 * Copyright 2022 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

/***********************************************************************************/
/* This file is automatically generated using bindtool and can be manually edited  */
/* The following lines can be configured to regenerate this file during cmake      */
/* If manual edits are made, the following tags should be modified accordingly.    */
/* BINDTOOL_GEN_AUTOMATIC(0)                                                       */
/* BINDTOOL_USE_PYGCCXML(0)                                                        */
/* BINDTOOL_HEADER_FILE(power_calc_ff.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(08b9c710db9e3ed4e11ce16d001b2cdf)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <pulsed_power_daq/power_calc_ff.h>
// pydoc.h is automatically generated in the build directory
#include <power_calc_ff_pydoc.h>

void bind_power_calc_ff(py::module& m)
{

    using power_calc_ff    = ::gr::pulsed_power_daq::power_calc_ff;


    py::class_<power_calc_ff, gr::sync_block, gr::block, gr::basic_block,
        std::shared_ptr<power_calc_ff>>(m, "power_calc_ff", D(power_calc_ff))

        .def(py::init(&power_calc_ff::make),
           py::arg("alpha") = 9.9999999999999995E-8,
           D(power_calc_ff,make)
        )
        




        
        .def("set_alpha",&power_calc_ff::set_alpha,       
            py::arg("alpha"),
            D(power_calc_ff,set_alpha)
        )

        ;




}








