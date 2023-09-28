cmake_minimum_required(VERSION 3.12)

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

option(ENABLE_PICOSCOPE "Enable PicoScope support" ON)
set(PICOSCOPE_PREFIX "/opt/picoscope" CACHE PATH "Picoscope drivers prefix") # TODO use proper find_package

add_subdirectory(gr-picoscope)
add_subdirectory(blocklib)

option(ENABLE_TESTING "Enable Test Builds" ON)
if (ENABLE_TESTING AND UNIX AND NOT APPLE)
    list(APPEND CMAKE_CTEST_ARGUMENTS "--output-on-failure")
    enable_testing()
endif ()
