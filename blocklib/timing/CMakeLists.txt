add_executable(saftbusexample src/saftbusexample.cpp)
target_link_libraries(saftbusexample PRIVATE gr-digitizers-options PkgConfig::saftlib CLI11::CLI11)

# if (ENABLE_GR_DIGITIZERS_TESTING)
#     add_subdirectory(test)
# endif ()
