# TODO do not hardcode
add_library(ps4000a SHARED IMPORTED GLOBAL)
set_property(TARGET ps4000a PROPERTY IMPORTED_LOCATION
                                     ${PICOSCOPE_PREFIX}/lib/libps4000a.so)
target_link_libraries(ps4000a INTERFACE PkgConfig::zlib PkgConfig::libusb)
target_include_directories(
  ps4000a INTERFACE ${PICOSCOPE_PREFIX}/include/libps4000a
                    ${PICOSCOPE_PREFIX}/include/libps5000a) # Hack:
                                                            # PicoCallback.h is
                                                            # missing in
                                                            # libps4000a/

add_library(ps5000a SHARED IMPORTED GLOBAL)
set_property(TARGET ps5000a PROPERTY IMPORTED_LOCATION
                                     ${PICOSCOPE_PREFIX}/lib/libps5000a.so)
target_link_libraries(ps5000a INTERFACE PkgConfig::zlib PkgConfig::libusb)
target_include_directories(ps5000a
                           INTERFACE ${PICOSCOPE_PREFIX}/include/libps5000a)

add_library(fair-picoscope INTERFACE)
target_include_directories(
  fair-picoscope INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                           $<INSTALL_INTERFACE:include/>)

target_link_libraries(
  fair-picoscope INTERFACE ps4000a ps5000a gr-digitizers-options gnuradio-core
                           gnuradio-algorithm fmt)
set_target_properties(
  gr-digitizers
  PROPERTIES PUBLIC_HEADER
             "Picoscope.hpp;Picoscope4000a.hpp;StatusMessages.hpp")

if(ENABLE_GR_DIGITIZERS_TESTING)
  add_subdirectory(test)
endif()
