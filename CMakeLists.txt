cmake_minimum_required(VERSION 3.12)

project(gr-digitizers CXX)
set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(
    graph-prototype
    GIT_REPOSITORY https://github.com/fair-acc/graph-prototype.git
    GIT_TAG 39c680c37d1a7a79bde8a5098dc42fb5efe03c02
)

FetchContent_Declare(
    zeromq
    GIT_REPOSITORY https://github.com/zeromq/libzmq.git
    GIT_TAG v4.3.4 # latest v4.3.4
)

FetchContent_Declare(
    ut
    GIT_REPOSITORY https://github.com/boost-ext/ut.git
    GIT_TAG bf8388f61103571dee3061a4ef23292a320d9dbf # head as of 2023-08-21
)

FetchContent_MakeAvailable(graph-prototype zeromq ut)

add_subdirectory(blocklib)

option(ENABLE_TESTING "Enable Test Builds" ON)
if (ENABLE_TESTING AND UNIX AND NOT APPLE)
    list(APPEND CMAKE_CTEST_ARGUMENTS "--output-on-failure")
    enable_testing()
endif ()
