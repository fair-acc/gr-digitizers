add_library(fair-picoscope INTERFACE)

target_include_directories(fair-picoscope INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/>)

target_link_libraries(fair-picoscope INTERFACE gr-digitizers-options graph-prototype)
set_target_properties(fair-picoscope PROPERTIES PUBLIC_HEADER picoscope.hpp)
