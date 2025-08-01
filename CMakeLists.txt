cmake_minimum_required(VERSION 3.25)

project(gr-digitizers CXX)
set(CMAKE_CXX_STANDARD 23)

# Determine if gr-digitizers is built as a subproject (using add_subdirectory)
# or if it is the top-level project.
if(NOT DEFINED GR_DIGITIZERS_TOPLEVEL_PROJECT)
  set(GR_DIGITIZERS_TOPLEVEL_PROJECT OFF)
  if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(GR_DIGITIZERS_TOPLEVEL_PROJECT ON)
    message(STATUS "CMake version: ${CMAKE_VERSION}")
  endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang") # set default C++ STL to Clang's
                                            # libc++ when using Clang
  add_compile_options(-stdlib=libc++ -fcolor-diagnostics)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++")
  set(CLANG true)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-fdiagnostics-color=always)
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)

include(cmake/CMakeRC.cmake)
include(FetchContent)

set(ENABLE_TESTING OFF)
FetchContent_Declare(
  gnuradio4
  GIT_REPOSITORY https://github.com/fair-acc/gnuradio4.git
  GIT_TAG 72997a580d3b62c67e661a6e60705202dfdb6478 # main as of 2025-07-07
  EXCLUDE_FROM_ALL)

FetchContent_Declare(
  ut
  GIT_REPOSITORY https://github.com/boost-ext/ut.git
  GIT_TAG v2.3.1 # latest tag as of 2025-04-02
  EXCLUDE_FROM_ALL)

add_library(gr-digitizers-options INTERFACE)
include(cmake/CompilerWarnings.cmake)
set_project_warnings(gr-digitizers-options)

if(EMSCRIPTEN)
  FetchContent_MakeAvailable(gnuradio4 ut)
  set(CMAKE_EXECUTABLE_SUFFIX ".js")
  target_compile_options(gr-digitizers-options INTERFACE -fwasm-exceptions
                                                         -pthread)
  target_link_options(
    gr-digitizers-options
    INTERFACE
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    -fwasm-exceptions
    -pthread
    "SHELL:-s PTHREAD_POOL_SIZE=30"
    "SHELL:-s FETCH=1"
    "SHELL:-s ASSERTIONS=1")
else()
  if(GR_DIGITIZERS_TOPLEVEL_PROJECT)
    FetchContent_Declare(
      imgui
      GIT_REPOSITORY https://github.com/ocornut/imgui.git
      GIT_TAG v1.91.9b-docking)

    # Enables 32 bit vertex indices for ImGui
    add_compile_definitions("ImDrawIdx=unsigned int")
    FetchContent_Declare(
      implot
      GIT_REPOSITORY https://github.com/epezent/implot.git
      GIT_TAG v0.16)
    FetchContent_MakeAvailable(imgui implot gnuradio4 ut)

    find_package(SDL3 REQUIRED)
    find_package(OpenGL REQUIRED COMPONENTS OpenGL)

    # imgui and implot are not CMake Projects, so we have to define their
    # targets manually here
    add_library(
      imgui OBJECT
      ${imgui_SOURCE_DIR}/imgui_demo.cpp
      ${imgui_SOURCE_DIR}/imgui_draw.cpp
      ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
      ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
      ${imgui_SOURCE_DIR}/imgui_tables.cpp
      ${imgui_SOURCE_DIR}/imgui_widgets.cpp
      ${imgui_SOURCE_DIR}/imgui.cpp
      ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp)
    target_link_libraries(imgui PUBLIC SDL3::SDL3 OpenGL::GL)
    target_compile_options(
      imgui PRIVATE -w) # imgui does lots of oldstyle casts and pointer
                        # arithmethic leading to many warnings
    target_include_directories(imgui BEFORE PUBLIC SYSTEM ${imgui_SOURCE_DIR}
                                                   ${imgui_SOURCE_DIR}/backends)

    add_library(
      implot OBJECT
      ${implot_SOURCE_DIR}/implot_demo.cpp
      ${implot_SOURCE_DIR}/implot_items.cpp ${implot_SOURCE_DIR}/implot.cpp)
    target_compile_options(
      implot PRIVATE -w) # implot does lots of oldstyle casts and pointer
                         # arithmethic leading to many warnings
    target_include_directories(implot BEFORE PUBLIC SYSTEM ${implot_SOURCE_DIR})
    target_link_libraries(implot PUBLIC imgui)
  endif()
endif()

if(CMAKE_CXX_COMPILER MATCHES "/em\\+\\+(-[a-zA-Z0-9.])?$") # if this hasn't
                                                            # been set before
                                                            # via e.g. emcmake
  message(" Transpiling to WASM: using: Emscripten (${CMAKE_CXX_COMPILER})")
  set(EMSCRIPTEN true)
endif()

if(NOT EMSCRIPTEN)
  option(ENABLE_PICOSCOPE "Enable PicoScope support" ON)
  option(ENABLE_TIMING "Enable TimingReceiver support" ON)
  set(PICOSCOPE_PREFIX
      "/opt/picoscope"
      CACHE PATH "Picoscope drivers prefix") # TODO use proper find_package
endif()

find_package(PkgConfig REQUIRED)

if(ENABLE_PICOSCOPE)
  pkg_check_modules(zlib REQUIRED IMPORTED_TARGET zlib)
  pkg_check_modules(libusb REQUIRED IMPORTED_TARGET libusb-1.0)
endif()

if(ENABLE_TIMING AND NOT EMSCRIPTEN)
  pkg_check_modules(saftlib REQUIRED IMPORTED_TARGET saftlib)
  pkg_check_modules(etherbone REQUIRED IMPORTED_TARGET etherbone)
endif()

option(ENABLE_GR_DIGITIZERS_TESTING "Enable gr-digitizers Test Builds"
       ${GR_DIGITIZERS_TOPLEVEL_PROJECT})
if(ENABLE_GR_DIGITIZERS_TESTING
   AND UNIX
   AND NOT APPLE)
  list(APPEND CMAKE_CTEST_ARGUMENTS "--output-on-failure")
  enable_testing()
  if(ENABLE_COVERAGE)
    message("Coverage reporting enabled")
    set(CODE_COVERAGE_VERBOSE ON)
    include(cmake/CodeCoverage.cmake
    )# https://github.com/bilke/cmake-modules/blob/master/CodeCoverage.cmake #
     # (License: BSL-1.0)
    target_compile_options(
      gr-digitizers-options INTERFACE --coverage -O0 -g -U_FORTIFY_SOURCE
                                      -D_FORTIFY_SOURCE=0) # fortify_source is
                                                           # not possible
                                                           # without
                                                           # optimization
    target_link_libraries(gr-digitizers-options INTERFACE --coverage)
    set(GCOVR_ADDITIONAL_ARGS --print-summary --gcov-ignore-errors=all
                              --verbose)
    append_coverage_compiler_flags()
    setup_target_for_coverage_gcovr_xml(
      NAME
      coverage
      EXECUTABLE
      ctest
      DEPENDENCIES
      qa_BlockScalingOffset
      EXCLUDE
      "$CMAKE_BUILD_DIR/*"
      "concepts/.*"
      ".*/test/.*")
    setup_target_for_coverage_gcovr_html(
      NAME
      coverage_html
      EXECUTABLE
      ctest
      DEPENDENCIES
      qa_BlockScalingOffset
      EXCLUDE
      "$CMAKE_BUILD_DIR/*"
      "concepts/.*"
      ".*/test/.*")
  endif()
endif()

add_subdirectory(blocklib)
