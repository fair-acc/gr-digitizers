add_executable(test-timing src/test-timing.cpp)
target_link_libraries(test-timing PRIVATE gr-digitizers-options PkgConfig::saftlib imgui implot CLI11::CLI11 gnuradio-algorithm gnuradio-core ps4000a)

# if (ENABLE_GR_DIGITIZERS_TESTING)
#     add_subdirectory(test)
# endif ()
