add_library(gr-picoscope INTERFACE)

set(GR_PICOSCOPE_HEADERS
    picoscope.hpp
)

target_include_directories(gr-picoscope INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/>)

target_link_libraries(gr-picoscope INTERFACE gr-digitizers-options graph-prototype)
set_target_properties(gr-picoscope PROPERTIES PUBLIC_HEADER "${GR_PICOSCOPE_HEADERS}")
