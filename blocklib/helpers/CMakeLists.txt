# setup header only library
add_library(fair-helpers INTERFACE)

target_include_directories(fair-helpers INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/>)

target_link_libraries(fair-helpers INTERFACE gnuradio-core)
set_target_properties(fair-helpers PROPERTIES PUBLIC_HEADER helper_blocks.hpp)
