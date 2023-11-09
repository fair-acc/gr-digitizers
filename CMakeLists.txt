cmake_minimum_required(VERSION 3.25)

project(gr-digitizers CXX)
set(CMAKE_CXX_STANDARD 23)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)

include(FetchContent)

set(ENABLE_TESTING OFF)
FetchContent_Declare(
    graph-prototype
    GIT_REPOSITORY https://github.com/fair-acc/graph-prototype.git
    GIT_TAG 9f24bac891cef9e667a0f5719c666765e2a922d5 # main as of 2023-11-09
)

FetchContent_Declare(
    ut
    GIT_REPOSITORY https://github.com/boost-ext/ut.git
    GIT_TAG bf8388f61103571dee3061a4ef23292a320d9dbf # head as of 2023-08-21
)

FetchContent_MakeAvailable(graph-prototype ut)

add_library(gr-digitizers-options INTERFACE)
include(cmake/CompilerWarnings.cmake)
set_project_warnings(gr-digitizers-options)

if(CMAKE_CXX_COMPILER MATCHES "/em\\+\\+(-[a-zA-Z0-9.])?$") # if this hasn't been set before via e.g. emcmake
    message(" Transpiling to WASM: using: Emscripten (${CMAKE_CXX_COMPILER})")
    set(EMSCRIPTEN true)
endif()

if(NOT EMSCRIPTEN)
    option(ENABLE_PICOSCOPE "Enable PicoScope support" ON)
    set(PICOSCOPE_PREFIX "/opt/picoscope" CACHE PATH "Picoscope drivers prefix") # TODO use proper find_package
endif()

if(ENABLE_PICOSCOPE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(zlib REQUIRED IMPORTED_TARGET zlib)
    pkg_check_modules(libusb REQUIRED IMPORTED_TARGET libusb-1.0)
endif()

option(ENABLE_GR_DIGITIZERS_TESTING "Enable gr-digitizers Test Builds" ON)
if (ENABLE_GR_DIGITIZERS_TESTING AND UNIX AND NOT APPLE)
    list(APPEND CMAKE_CTEST_ARGUMENTS "--output-on-failure")
    enable_testing()
    if (ENABLE_COVERAGE)
        message("Coverage reporting enabled")
        include(cmake/CodeCoverage.cmake) # https://github.com/bilke/cmake-modules/blob/master/CodeCoverage.cmake # (License: BSL-1.0)
        target_compile_options(gr-digitizers-options INTERFACE --coverage -O0 -g -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0) # fortify_source is not possible without optimization
        target_link_libraries(gr-digitizers-options INTERFACE --coverage)
        append_coverage_compiler_flags()
        setup_target_for_coverage_gcovr_xml(
                NAME coverage
                EXECUTABLE ctest
                DEPENDENCIES qa_BlockScalingOffset
                EXCLUDE "$CMAKE_BUILD_DIR/*" "concepts/.*" ".*/test/.*")
        setup_target_for_coverage_gcovr_html(
                NAME coverage_html
                EXECUTABLE ctest
                DEPENDENCIES qa_BlockScalingOffset
                EXCLUDE "$CMAKE_BUILD_DIR/*" "concepts/.*" ".*/test/.*")
    endif ()
endif ()

add_subdirectory(blocklib)

