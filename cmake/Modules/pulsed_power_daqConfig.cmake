find_package(PkgConfig)

PKG_CHECK_MODULES(PC_PULSED_POWER_DAQ pulsed_power_daq)

FIND_PATH(
    PULSED_POWER_DAQ_INCLUDE_DIRS
    NAMES pulsed_power_daq/api.h
    HINTS $ENV{PULSED_POWER_DAQ_DIR}/include
        ${PC_PULSED_POWER_DAQ_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    PULSED_POWER_DAQ_LIBRARIES
    NAMES gnuradio-pulsed_power_daq
    HINTS $ENV{PULSED_POWER_DAQ_DIR}/lib
        ${PC_PULSED_POWER_DAQ_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/pulsed_power_daqTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PULSED_POWER_DAQ DEFAULT_MSG PULSED_POWER_DAQ_LIBRARIES PULSED_POWER_DAQ_INCLUDE_DIRS)
MARK_AS_ADVANCED(PULSED_POWER_DAQ_LIBRARIES PULSED_POWER_DAQ_INCLUDE_DIRS)
