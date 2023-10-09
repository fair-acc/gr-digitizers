# setup header only library
add_library(gr-digitizers INTERFACE)

target_include_directories(gr-digitizers INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/>)

target_link_libraries(gr-digitizers INTERFACE gr-digitizers-options fair-helpers gnuradio-core)
set_target_properties(gr-digitizers PROPERTIES PUBLIC_HEADER block_scaling_offset.hpp)

if (ENABLE_GR_DIGITIZERS_TESTING)
    add_subdirectory(test)
endif ()
