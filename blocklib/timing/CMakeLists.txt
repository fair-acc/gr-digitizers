if (NOT EMSCRIPTEN)
    cmrc_add_resource_library(
            ui_assets
            NAMESPACE
            ui_assets
            WHENCE
            ${imgui_SOURCE_DIR}/misc/fonts
            ${imgui_SOURCE_DIR}/misc/fonts/Roboto-Medium.ttf)

    add_library(timing INTERFACE)
    target_include_directories(timing INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/)

    add_executable(test-timing src/test-timing.cpp)
    target_link_libraries(test-timing PRIVATE gr-digitizers-options timing PkgConfig::saftlib PkgConfig::etherbone imgui implot gnuradio-algorithm gnuradio-core ui_assets)
endif ()

if (ENABLE_GR_DIGITIZERS_TESTING)
    add_subdirectory(test)
endif ()
