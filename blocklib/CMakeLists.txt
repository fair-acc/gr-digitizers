add_subdirectory(helpers)
add_subdirectory(digitizers)

if (ENABLE_PICOSCOPE)
    add_subdirectory(picoscope)
endif ()
