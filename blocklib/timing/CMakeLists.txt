if(NOT EMSCRIPTEN AND NOT CLANG)
  add_library(timing INTERFACE include/fair/timing/timing.hpp
              include/fair/timing/TimingSource.hpp)
  target_include_directories(timing
                             INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/)
  target_link_libraries(timing INTERFACE PkgConfig::saftlib
                                         PkgConfig::etherbone)

  if(GR_DIGITIZERS_TOPLEVEL_PROJECT)
    cmrc_add_resource_library(
      ui_assets NAMESPACE ui_assets WHENCE ${imgui_SOURCE_DIR}/misc/fonts
      ${imgui_SOURCE_DIR}/misc/fonts/Roboto-Medium.ttf)

    add_executable(test-timing src/test-timing.cpp)
    target_link_libraries(
      test-timing
      PRIVATE gr-digitizers-options
              timing
              imgui
              implot
              gnuradio-algorithm
              gnuradio-core
              ui_assets)
  endif()
endif()

if(ENABLE_GR_DIGITIZERS_TESTING)
  add_subdirectory(test)
endif()
