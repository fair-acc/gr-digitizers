# setup header only library
add_library(gr-helpers INTERFACE)

target_include_directories(gr-helpers INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/>)

target_link_libraries(gr-helpers INTERFACE graph-prototype)
set_target_properties(gr-helpers PROPERTIES PUBLIC_HEADER helper_blocks.hpp)
