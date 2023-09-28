cmake_minimum_required(VERSION 3.25)

project(gr-digitizers CXX)
set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)

include(FetchContent)

FetchContent_Declare(
    graph-prototype
    GIT_REPOSITORY https://github.com/fair-acc/graph-prototype.git
    GIT_TAG ed71b4cde6e4116c25da0ff99c4842f1f1fdc349
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

option(ENABLE_TESTING "Enable Test Builds" ON)
if (ENABLE_TESTING AND UNIX AND NOT APPLE)
    list(APPEND CMAKE_CTEST_ARGUMENTS "--output-on-failure")
    enable_testing()
endif ()

if(ENABLE_PICOSCOPE)
    add_subdirectory(gr-picoscope)
endif()

add_subdirectory(blocklib)

