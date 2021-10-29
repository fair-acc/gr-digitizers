find_package(PkgConfig)

PKG_CHECK_MODULES(PC_DIGITIZERS_39 digitizers_39)

FIND_PATH(
    DIGITIZERS_39_INCLUDE_DIRS
    NAMES digitizers_39/api.h
    HINTS $ENV{DIGITIZERS_39_DIR}/include
        ${PC_DIGITIZERS_39_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    DIGITIZERS_39_LIBRARIES
    NAMES gnuradio-digitizers_39
    HINTS $ENV{DIGITIZERS_39_DIR}/lib
        ${PC_DIGITIZERS_39_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/digitizers_39Target.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DIGITIZERS_39 DEFAULT_MSG DIGITIZERS_39_LIBRARIES DIGITIZERS_39_INCLUDE_DIRS)
MARK_AS_ADVANCED(DIGITIZERS_39_LIBRARIES DIGITIZERS_39_INCLUDE_DIRS)
