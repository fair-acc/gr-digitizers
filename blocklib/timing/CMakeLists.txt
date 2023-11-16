cmrc_add_resource_library(
        ui_assets
        NAMESPACE
        ui_assets
        WHENCE
        ${imgui_SOURCE_DIR}/misc/fonts
        ${imgui_SOURCE_DIR}/misc/fonts/Roboto-Medium.ttf)

add_executable(test-timing src/test-timing.cpp)
target_link_libraries(test-timing PRIVATE gr-digitizers-options PkgConfig::saftlib PkgConfig::etherbone imgui implot CLI11::CLI11 gnuradio-algorithm gnuradio-core ps4000a ui_assets)

# if (ENABLE_GR_DIGITIZERS_TESTING)
#     add_subdirectory(test)
# endif ()
