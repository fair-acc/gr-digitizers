if(ENABLE_PICOSCOPE)
  add_subdirectory(picoscope)
endif()

if(ENABLE_TIMING)
  add_subdirectory(timing)
endif()

if(ENABLE_OPENCMW)
  add_subdirectory(opencmw)
endif()
