set(PICOSCOPE4000A_SOURCES
    picoscope4000a.cc
)

set(PICOSCOPE4000A_HEADERS
    picoscope4000a.hpp
)

# TODO do not hardcode
add_library(ps4000a SHARED IMPORTED)
set_property(TARGET ps4000a PROPERTY
             IMPORTED_LOCATION ${PICOSCOPE_PREFIX}/lib/libps4000a.so)
target_include_directories(ps4000a INTERFACE ${PICOSCOPE_PREFIX}/include/libps4000a ${PICOSCOPE_PREFIX}/include/libps5000a) # Hack: PicoCallback.h is missing in libps4000a/

add_library(gr-picoscope4000a SHARED ${PICOSCOPE4000A_SOURCES})
target_include_directories(gr-picoscope4000a INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/>)

target_link_libraries(gr-picoscope4000a ps4000a gr-picoscope volk refl-cpp fmt)
set_target_properties(gr-digitizers PROPERTIES PUBLIC_HEADER "${PICOSCOPE4000A_HEADERS}")

if (ENABLE_TESTING)
    add_subdirectory(test)
endif ()
