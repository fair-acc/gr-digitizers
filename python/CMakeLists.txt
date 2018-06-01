# Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
# co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
# You may use, distribute and modify this code under the terms of the GPL v.3  license.

########################################################################
# Include python install macros
########################################################################
include(GrPython)
if(NOT PYTHONINTERP_FOUND)
    return()
endif()

########################################################################
# Install python sources
########################################################################
GR_PYTHON_INSTALL(
    FILES
    __init__.py
    DESTINATION ${GR_PYTHON_DIR}/digitizers
)

########################################################################
# Handle the unit tests
########################################################################
include(GrTest)

set(GR_TEST_TARGET_DEPS gnuradio-digitizers)
set(GR_TEST_PYTHON_DIRS ${CMAKE_BINARY_DIR}/swig)
