cmake_minimum_required(VERSION 3.25)

project(gr-digitizers CXX)
set(CMAKE_CXX_STANDARD 23)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)

include(cmake/CMakeRC.cmake)
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

if (EMSCRIPTEN)
    FetchContent_MakeAvailable(graph-prototype ut)
else ()
    FetchContent_Declare(
            imgui
            GIT_REPOSITORY  https://github.com/ocornut/imgui.git
            GIT_TAG         v1.90
    )

    # Enables 32 bit vertex indices for ImGui
    add_compile_definitions("ImDrawIdx=unsigned int")
    FetchContent_Declare(
            implot
            GIT_REPOSITORY  https://github.com/epezent/implot.git
            GIT_TAG         v0.16
    )
    FetchContent_MakeAvailable(imgui implot graph-prototype ut)

    find_package(SDL2 REQUIRED)
    find_package(OpenGL REQUIRED COMPONENTS OpenGL)

    # imgui and implot are not CMake Projects, so we have to define their targets manually here
    add_library(imgui
        OBJECT
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    )
    target_link_libraries(imgui PUBLIC SDL2::SDL2 OpenGL::GL)
    target_include_directories(imgui BEFORE
        PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
    )

    add_library(implot
        OBJECT ${implot_SOURCE_DIR}/implot_demo.cpp ${implot_SOURCE_DIR}/implot_items.cpp ${implot_SOURCE_DIR}/implot.cpp
    )
    target_include_directories(implot BEFORE
        PUBLIC
            ${implot_SOURCE_DIR}
    )
    target_link_libraries(implot PUBLIC imgui)
endif ()

add_library(gr-digitizers-options INTERFACE)
include(cmake/CompilerWarnings.cmake)
set_project_warnings(gr-digitizers-options)

if(CMAKE_CXX_COMPILER MATCHES "/em\\+\\+(-[a-zA-Z0-9.])?$") # if this hasn't been set before via e.g. emcmake
    message(" Transpiling to WASM: using: Emscripten (${CMAKE_CXX_COMPILER})")
    set(EMSCRIPTEN true)
endif()

if(NOT EMSCRIPTEN)
    option(ENABLE_PICOSCOPE "Enable PicoScope support" ON)
    option(ENABLE_TIMING "Enable TimingReceiver support" ON)
    set(PICOSCOPE_PREFIX "/opt/picoscope" CACHE PATH "Picoscope drivers prefix") # TODO use proper find_package
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
