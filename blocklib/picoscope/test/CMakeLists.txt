# for manual usage, not run by ctest
function(add_ut_test_tool TEST_NAME)
  add_executable(${TEST_NAME} ${TEST_NAME}.cc)
  target_include_directories(
    ${TEST_NAME}
    PRIVATE ${CMAKE_BINARY_DIR}/include ${CMAKE_CURRENT_BINARY_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(
    ${TEST_NAME}
    PRIVATE gnuradio-core
            gr-testing
            gr-basic
            fair-picoscope
            ut
            gnuradio-blocklib-core
            GrBasicBlocksShared)
endfunction()

function(add_ut_test TEST_NAME)
  add_ut_test_tool(${TEST_NAME})
  add_test(NAME ${TEST_NAME} COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR}
                                     ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME})
endfunction()

add_ut_test_tool(qa_Picoscope)
add_ut_test_tool(qa_PicoscopeAPI)
add_ut_test_tool(qa_PicoscopePerformanceMonitor)
add_ut_test(qa_TimingMatcher)

if(NOT EMSCRIPTEN AND NOT CLANG)
  add_ut_test_tool(qa_PicoscopeTiming3000A)
  target_link_libraries(qa_PicoscopeTiming3000A PRIVATE timing)
  add_ut_test_tool(qa_PicoscopeTiming4000A)
  target_link_libraries(qa_PicoscopeTiming4000A PRIVATE timing)
  add_ut_test_tool(qa_PicoscopeTiming5000A)
  target_link_libraries(qa_PicoscopeTiming5000A PRIVATE timing)
  add_ut_test_tool(qa_PicoscopeTiming6000)
  target_link_libraries(qa_PicoscopeTiming6000 PRIVATE timing)
endif()
