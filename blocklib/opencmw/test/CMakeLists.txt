function(add_ut_test_tool TEST_NAME) # for manual usage, not run by ctest
  add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
  target_include_directories(
    ${TEST_NAME}
    PRIVATE ${CMAKE_BINARY_DIR}/include ${CMAKE_CURRENT_BINARY_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(
    ${TEST_NAME}
    PRIVATE gnuradio-core
            gr-testing
            gr-basic
            fair-opencmw
            ut
            gnuradio-blocklib-core
            GrBasicBlocksShared)
endfunction()

function(add_ut_test TEST_NAME)
  add_ut_test_tool(${TEST_NAME})
  add_test(NAME ${TEST_NAME} COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR}
                                     ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME})
endfunction()

add_ut_test(qa_OpenCmwClient)
add_ut_test(qa_OpenCmwSource)

add_custom_target(
  copy-rda3-jar ALL
  COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/DemoRda3Server.jar
    ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(qa_OpenCmwClient copy-rda3-jar)
add_dependencies(qa_OpenCmwSource copy-rda3-jar)
