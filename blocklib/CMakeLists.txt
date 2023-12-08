add_subdirectory(helpers)
add_subdirectory(digitizers)

if (ENABLE_PICOSCOPE)
    add_subdirectory(picoscope)
endif ()

if (ENABLE_TIMING)
    add_subdirectory(timing)
endif ()
