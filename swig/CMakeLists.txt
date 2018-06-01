# Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
# co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
# You may use, distribute and modify this code under the terms of the GPL v.3  license.

########################################################################
# Check if there is C++ code at all
########################################################################
if(NOT digitizers_sources)
	MESSAGE(STATUS "No C++ sources... skipping swig/")
	return()
endif(NOT digitizers_sources)

########################################################################
# Include swig generation macros
########################################################################
find_package(SWIG)
find_package(PythonLibs 2)
if(NOT SWIG_FOUND OR NOT PYTHONLIBS_FOUND)
    return()
endif()
include(GrSwig)
include(GrPython)

link_directories(
    /opt/picoscope/lib
)
########################################################################
# Setup swig generation
########################################################################
foreach(incdir ${GNURADIO_RUNTIME_INCLUDE_DIRS})
    list(APPEND GR_SWIG_INCLUDE_DIRS ${incdir}/gnuradio/swig)
endforeach(incdir)

set(GR_SWIG_LIBRARIES gnuradio-digitizers)
set(GR_SWIG_DOC_FILE ${CMAKE_CURRENT_BINARY_DIR}/digitizers_swig_doc.i)
set(GR_SWIG_DOC_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../include)

GR_SWIG_MAKE(digitizers_swig digitizers_swig.i)

########################################################################
# Install the build swig module
########################################################################
GR_SWIG_INSTALL(TARGETS digitizers_swig DESTINATION ${GR_PYTHON_DIR}/digitizers)

########################################################################
# Install swig .i files for development
########################################################################
install(
    FILES
    digitizers_swig.i
    ${CMAKE_CURRENT_BINARY_DIR}/digitizers_swig_doc.i
    DESTINATION ${GR_INCLUDE_DIR}/digitizers/swig
)
