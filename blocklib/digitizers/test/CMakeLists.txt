function(add_ut_test TEST_NAME)
  add_executable(${TEST_NAME} ${TEST_NAME}.cc)
  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")) # needed for clang15 (false
                                                # positives, fixed in clang16)
    target_compile_options(${TEST_NAME} PRIVATE -Wall)
    target_link_options(${TEST_NAME} PRIVATE -Wall)
  else()
    target_compile_options(${TEST_NAME} PRIVATE -fsanitize=address -Wall)
    target_link_options(${TEST_NAME} PRIVATE -fsanitize=address -Wall)
  endif()
  target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_BINARY_DIR}/include
                                                  ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(${TEST_NAME} PRIVATE gnuradio-core gr-digitizers fmt ut)
  add_test(NAME ${TEST_NAME} COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR}
                                     ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME})
endfunction()

add_ut_test(qa_BlockScalingOffset)
