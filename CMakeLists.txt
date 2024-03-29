# Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
# co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
# You may use, distribute and modify this code under the terms of the GPL v.3  license.

########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 2.6)
project(gr-digitizers CXX C)
enable_testing()

if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
endif()


#install to PyBOMBS target prefix if defined
if(DEFINED ENV{PYBOMBS_PREFIX})
    set(CMAKE_INSTALL_PREFIX $ENV{PYBOMBS_PREFIX})
    message(STATUS "PyBOMBS installed GNU Radio. Setting CMAKE_INSTALL_PREFIX to $ENV{PYBOMBS_PREFIX}")
endif()

#select the release build type by default to get optimization flags
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
#set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

#make sure our local CMake Modules path comes first
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake/Modules)

# Set the version information here
# All of them need to be filled, otherwise the version will be named e.g. "libname-1.2.3..so"
set(VERSION_INFO_MAJOR_VERSION 5)
set(VERSION_INFO_API_COMPAT    1)
set(VERSION_INFO_MINOR_VERSION 4)
set(VERSION_INFO_MAINT_VERSION 0)


# Set cmake policies.
# This will suppress developer warnings during the cmake process that can occur
# if a newer cmake version than the minimum is used.

if(POLICY CMP0026)
    cmake_policy(SET CMP0026 OLD)
endif()
if(POLICY CMP0043)
    cmake_policy(SET CMP0043 OLD)
endif()
if(POLICY CMP0045)
    cmake_policy(SET CMP0045 OLD)
endif()
if(POLICY CMP0046)
    cmake_policy(SET CMP0046 OLD)
endif()

########################################################################
# Compiler specific setup
########################################################################
if(CMAKE_COMPILER_IS_GNUCXX AND NOT WIN32)
    #http://gcc.gnu.org/wiki/Visibility
    add_definitions(-fvisibility=hidden)
endif()

########################################################################
# Find boost
########################################################################
if(UNIX AND EXISTS "/usr/lib64")
    list(APPEND BOOST_LIBRARYDIR "/usr/lib64") #fedora 64-bit fix
endif(UNIX AND EXISTS "/usr/lib64")
set(Boost_ADDITIONAL_VERSIONS
    "1.35.0" "1.35" "1.36.0" "1.36" "1.37.0" "1.37" "1.38.0" "1.38" "1.39.0" "1.39"
    "1.40.0" "1.40" "1.41.0" "1.41" "1.42.0" "1.42" "1.43.0" "1.43" "1.44.0" "1.44"
    "1.45.0" "1.45" "1.46.0" "1.46" "1.47.0" "1.47" "1.48.0" "1.48" "1.49.0" "1.49"
    "1.50.0" "1.50" "1.51.0" "1.51" "1.52.0" "1.52" "1.53.0" "1.53" "1.54.0" "1.54"
    "1.55.0" "1.55" "1.56.0" "1.56" "1.57.0" "1.57" "1.58.0" "1.58" "1.59.0" "1.59"
    "1.60.0" "1.60" "1.61.0" "1.61" "1.62.0" "1.62" "1.63.0" "1.63" "1.64.0" "1.64"
    "1.65.0" "1.65" "1.66.0" "1.66" "1.67.0" "1.67" "1.68.0" "1.68" "1.69.0" "1.69"
)
find_package(Boost "1.35" COMPONENTS filesystem system program_options thread chrono)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost required to compile digitizers")
endif()

########################################################################
# Find root
########################################################################

# You need to tell CMake where to find the ROOT installation. This can be done in a number of ways:
#   - ROOT built with classic configure/make use the provided $ROOTSYS/etc/cmake/FindROOT.cmake
#   - ROOT built with CMake. Add in CMAKE_PREFIX_PATH the installation prefix for ROOT
IF(NOT DEFINED ENV{ROOTSYS})
    # add default path of gsi-installation
	set(ENV{ROOTSYS} "/opt/cern/root")
ENDIF(NOT DEFINED ENV{ROOTSYS})

list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})

#---Locate the ROOT package and defines a number of variables (e.g. ROOT_INCLUDE_DIRS)
# TODO: the following does not work correctly, for some reason all the libraries gets pulled in
#       and not just MathCore and Graf as specified.
find_package(ROOT REQUIRED COMPONENTS MathCore Graf)

#---Define useful ROOT functions and macros (e.g. ROOT_GENERATE_DICTIONARY)
# include(${ROOT_USE_FILE})


########################################################################
# Install directories
########################################################################
include(GrPlatform) #define LIB_SUFFIX
set(GR_RUNTIME_DIR      bin)
set(GR_LIBRARY_DIR      lib${LIB_SUFFIX})
set(GR_INCLUDE_DIR      include/digitizers)
set(GR_DATA_DIR         share)
set(GR_PKG_DATA_DIR     ${GR_DATA_DIR}/${CMAKE_PROJECT_NAME})
set(GR_DOC_DIR          ${GR_DATA_DIR}/doc)
set(GR_PKG_DOC_DIR      ${GR_DOC_DIR}/${CMAKE_PROJECT_NAME})
set(GR_CONF_DIR         etc)
set(GR_PKG_CONF_DIR     ${GR_CONF_DIR}/${CMAKE_PROJECT_NAME}/conf.d)
set(GR_LIBEXEC_DIR      libexec)
set(GR_PKG_LIBEXEC_DIR  ${GR_LIBEXEC_DIR}/${CMAKE_PROJECT_NAME})
set(GRC_BLOCKS_DIR      ${GR_PKG_DATA_DIR}/grc/blocks)
set(ROOT_DIR            $ENV{ROOTSYS})

set(PICOSCOPE_INCLUDE_DIR   /usr/include/picoscope)
set(GR_DIGITIZERS_EXAMPLES_DIR      ${GR_PKG_DATA_DIR}/examples)


########################################################################
# On Apple only, set install name and use rpath correctly, if not already set
########################################################################
if(APPLE)
    if(NOT CMAKE_INSTALL_NAME_DIR)
        set(CMAKE_INSTALL_NAME_DIR
            ${CMAKE_INSTALL_PREFIX}/${GR_LIBRARY_DIR} CACHE
            PATH "Library Install Name Destination Directory" FORCE)
    endif(NOT CMAKE_INSTALL_NAME_DIR)
    if(NOT CMAKE_INSTALL_RPATH)
        set(CMAKE_INSTALL_RPATH
            ${CMAKE_INSTALL_PREFIX}/${GR_LIBRARY_DIR} CACHE
            PATH "Library Install RPath" FORCE)
    endif(NOT CMAKE_INSTALL_RPATH)
    if(NOT CMAKE_BUILD_WITH_INSTALL_RPATH)
        set(CMAKE_BUILD_WITH_INSTALL_RPATH ON CACHE
            BOOL "Do Build Using Library Install RPath" FORCE)
    endif(NOT CMAKE_BUILD_WITH_INSTALL_RPATH)
endif(APPLE)

########################################################################
# Find gnuradio build dependencies
########################################################################
find_package(CppUnit)
find_package(Doxygen)

# Search for GNU Radio and its components and versions. Add any
# components required to the list of GR_REQUIRED_COMPONENTS (in all
# caps such as FILTER or FFT) and change the version to the minimum
# API compatible version required.
set(GR_REQUIRED_COMPONENTS RUNTIME BLOCKS PMT FILTER)
find_package(Gnuradio "3.7.2" REQUIRED)
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake/Modules)
include(GrVersion)

if(NOT CPPUNIT_FOUND)
    message(FATAL_ERROR "CppUnit required to compile digitizers")
endif()

if(NOT EXISTS ${PICOSCOPE_INCLUDE_DIR}/ps3000aApi.h)
  message(SEND_ERROR "Can't find ps3000aApi.h in ${PICOSCOPE_INCLUDE_DIR}")
endif()
if(NOT EXISTS ${PICOSCOPE_INCLUDE_DIR}/ps4000aApi.h)
  message(SEND_ERROR "Can't find ps4000aApi.h in ${PICOSCOPE_INCLUDE_DIR}")
endif()
if(NOT EXISTS ${PICOSCOPE_INCLUDE_DIR}/ps6000Api.h)
  message(SEND_ERROR "Can't find ps6000Api.h in ${PICOSCOPE_INCLUDE_DIR}")
endif()
include(GrMiscUtils)

# Handle gr_log enable/disable
GR_LOGGING()

########################################################################
# Setup doxygen option
########################################################################
if(DOXYGEN_FOUND)
	option(ENABLE_DOXYGEN "Build docs using Doxygen" ON)
else(DOXYGEN_FOUND)
	option(ENABLE_DOXYGEN "Build docs using Doxygen" OFF)
endif(DOXYGEN_FOUND)

########################################################################
# Setup the include and linker paths
########################################################################
include_directories(
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/lib
    ${CMAKE_BINARY_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${CPPUNIT_INCLUDE_DIRS}
    ${GNURADIO_ALL_INCLUDE_DIRS}
    ${PICOSCOPE_INCLUDE_DIR}
    ${ROOT_INCLUDE_DIR}
)

link_directories(
    ${Boost_LIBRARY_DIRS}
    ${CPPUNIT_LIBRARY_DIRS}
    ${GNURADIO_RUNTIME_LIBRARY_DIRS}
    ${ROOT_LIBRARY_DIRS}
)

# Set component parameters
set(GR_DIGITIZERS_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE INTERNAL "" FORCE)
set(GR_DIGITIZERS_SWIG_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/swig CACHE INTERNAL "" FORCE)

########################################################################
# Create uninstall target
########################################################################
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
@ONLY)

add_custom_target(uninstall
    ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)

########################################################################
# Add subdirectories
########################################################################
add_subdirectory(include/digitizers)
add_subdirectory(lib)
add_subdirectory(swig)
add_subdirectory(python)
add_subdirectory(grc)
add_subdirectory(apps)
add_subdirectory(docs)
add_subdirectory(examples/cpp)

########################################################################
# Install cmake search helper for this library
########################################################################
if(NOT CMAKE_MODULES_DIR)
  set(CMAKE_MODULES_DIR lib${LIB_SUFFIX}/cmake)
endif(NOT CMAKE_MODULES_DIR)

install(FILES cmake/Modules/digitizersConfig.cmake
    DESTINATION ${CMAKE_MODULES_DIR}/digitizers
)
