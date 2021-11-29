# Copyright 2020 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Check if there is C++ code at all
########################################################################
if(NOT digitizers_39_sources)
    MESSAGE(STATUS "No C++ sources... skipping python bindings")
    return()
endif(NOT digitizers_39_sources)

########################################################################
# Check for pygccxml
########################################################################
GR_PYTHON_CHECK_MODULE_RAW(
    "pygccxml"
    "import pygccxml"
    PYGCCXML_FOUND
    )

include(GrPybind)

########################################################################
# Python Bindings
########################################################################

list(APPEND digitizers_39_python_files
    picoscope_4000a_python.cc
    power_calc_python.cc
    mains_frequency_calc_python.cc python_bindings.cc)

GR_PYBIND_MAKE_OOT(digitizers_39 
   ../..
   gr::digitizers_39
   "${digitizers_39_python_files}")

install(TARGETS digitizers_39_python DESTINATION ${GR_PYTHON_DIR}/digitizers_39 COMPONENT pythonapi)
