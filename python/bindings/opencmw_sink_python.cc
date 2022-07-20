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
/* BINDTOOL_HEADER_FILE(opencmw_sink.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(bb05434d3a98d926847f9d4d265dcb9a)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <pulsed_power_daq/opencmw_sink.h>
// pydoc.h is automatically generated in the build directory
#include <opencmw_sink_pydoc.h>

void bind_opencmw_sink(py::module& m)
{

    using opencmw_sink = ::gr::pulsed_power_daq::opencmw_sink;


    py::class_<opencmw_sink,
               gr::sync_block,
               gr::block,
               gr::basic_block,
               std::shared_ptr<opencmw_sink>>(m, "opencmw_sink", D(opencmw_sink))

        .def(
            py::init(&opencmw_sink::make), py::arg("nworkers") = 1, D(opencmw_sink, make))


        ;
}
