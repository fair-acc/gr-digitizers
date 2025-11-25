add_library(fair-picoscope INTERFACE)
target_include_directories(
  fair-picoscope
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
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
    "include/fair/picoscope/Picoscope.hpp;include/fair/picoscope/Picoscope3000a.hpp;Picoscope4000a.hpp;include/fair/picoscope/Picoscope5000a.hpp;include/fair/picoscope/Picoscope6000.hpp;include/fair/picoscope/StatusMessages.hpp"
)

add_executable(picoscope-cli src/picoscope-cli.cpp)
target_link_libraries(picoscope-cli PRIVATE fair-picoscope gnuradio-algorithm
                                            gnuradio-core)

if(ENABLE_GR_DIGITIZERS_TESTING)
  add_subdirectory(test)
endif()
