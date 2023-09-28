# setup header only library
add_library(gr-digitizers INTERFACE)

set(DIGITIZERS_HEADERS
    block_scaling_offset.hpp
)

target_include_directories(gr-digitizers INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/>)

target_link_libraries(gr-digitizers INTERFACE gr-digitizers-options gr-helpers graph-prototype)
set_target_properties(gr-digitizers PROPERTIES PUBLIC_HEADER "${DIGITIZERS_HEADERS}")

if (ENABLE_TESTING)
    add_subdirectory(test)
endif ()
