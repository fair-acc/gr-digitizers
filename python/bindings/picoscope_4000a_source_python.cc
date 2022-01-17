/*
 * Copyright 2021 Free Software Foundation, Inc.
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
/* BINDTOOL_HEADER_FILE(picoscope_4000a_source.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(dd0bf866885894d66641305f44353466)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include </opt/picoscope/include/libps4000a/ps4000aApi.h>
#include </opt/picoscope/include/libps4000a/PicoStatus.h>
#include <pulsed_power_daq/picoscope_4000a_source.h>
// pydoc.h is automatically generated in the build directory
#include <picoscope_4000a_source_pydoc.h>

void bind_picoscope_4000a_source(py::module& m)
{

    using picoscope_4000a_source    = gr::pulsed_power_daq::picoscope_4000a_source;


    py::class_<picoscope_4000a_source,
        gr::sync_block,
        gr::block,
        gr::basic_block,
        std::shared_ptr<picoscope_4000a_source>>(m, "picoscope_4000a_source", D(picoscope_4000a_source))

        .def(py::init(&picoscope_4000a_source::make),
           py::arg("serial_number"),
           py::arg("auto_arm"),
           D(picoscope_4000a_source,make)
        )
        

        .def("set_trigger_once", &picoscope_4000a_source::set_trigger_once, py::arg("trigger_once"))
        .def("set_samp_rate", &picoscope_4000a_source::set_samp_rate, py::arg("samp_rate"))
        .def("set_downsampling", &picoscope_4000a_source::set_downsampling, py::arg("downsampling_mode"), py::arg("downsampling_factor"))
        .def("set_aichan_trigger", &picoscope_4000a_source::set_aichan_trigger, py::arg("id"), py::arg("direction"), py::arg("threshold"))
        //.def("set_aichan", &picoscope_4000a_source::set_aichan, py::arg("id"), py::arg("enabled"), py::arg("range"), py::arg("coupling"), py::arg("range_offset"))
        
        .def("set_aichan_a", &picoscope_4000a_source::set_aichan_a, py::arg("enabled"), py::arg("range"), py::arg("coupling"), py::arg("range_offset"))
        .def("set_aichan_b", &picoscope_4000a_source::set_aichan_b, py::arg("enabled"), py::arg("range"), py::arg("coupling"), py::arg("range_offset"))
        .def("set_aichan_c", &picoscope_4000a_source::set_aichan_c, py::arg("enabled"), py::arg("range"), py::arg("coupling"), py::arg("range_offset"))
        .def("set_aichan_d", &picoscope_4000a_source::set_aichan_d, py::arg("enabled"), py::arg("range"), py::arg("coupling"), py::arg("range_offset"))
        .def("set_aichan_e", &picoscope_4000a_source::set_aichan_e, py::arg("enabled"), py::arg("range"), py::arg("coupling"), py::arg("range_offset"))
        .def("set_aichan_f", &picoscope_4000a_source::set_aichan_f, py::arg("enabled"), py::arg("range"), py::arg("coupling"), py::arg("range_offset"))
        .def("set_aichan_g", &picoscope_4000a_source::set_aichan_g, py::arg("enabled"), py::arg("range"), py::arg("coupling"), py::arg("range_offset"))
        .def("set_aichan_h", &picoscope_4000a_source::set_aichan_h, py::arg("enabled"), py::arg("range"), py::arg("coupling"), py::arg("range_offset"))
        
        .def("set_samples", &picoscope_4000a_source::set_samples, py::arg("pre_samples"), py::arg("post_samples"))
        .def("set_rapid_block", &picoscope_4000a_source::set_rapid_block, py::arg("nr_waveforms"))
        
        ;

      
}








