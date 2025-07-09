add_library(fair-picoscope INTERFACE)
target_include_directories(
  fair-picoscope INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                           $<INSTALL_INTERFACE:include/>)

target_link_libraries(
  fair-picoscope
  INTERFACE gr-digitizers-options
            gnuradio-core
            gnuradio-algorithm
            PicoScope::ps3000a
            PicoScope::ps4000a
            PicoScope::ps5000a
            PicoScope::ps6000)
set_target_properties(
  fair-picoscope
  PROPERTIES
    PUBLIC_HEADER
    "Picoscope.hpp;Picoscope3000a.hpp;Picoscope4000a.hpp;Picoscope5000a.hpp;StatusMessages.hpp"
)

if(ENABLE_GR_DIGITIZERS_TESTING)
  add_subdirectory(test)
endif()
