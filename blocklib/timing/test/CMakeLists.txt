function(add_ut_test TEST_NAME)
  add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang"))
    # needed for clang15 (false positives, fixed in clang16)
    target_compile_options(${TEST_NAME} PRIVATE -Wall)
    target_link_options(${TEST_NAME} PRIVATE -Wall)
  else()
    target_compile_options(${TEST_NAME} PRIVATE -fsanitize=address -Wall)
    target_link_options(${TEST_NAME} PRIVATE -fsanitize=address -Wall)
  endif()
  target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_BINARY_DIR}/include
                                                  ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(${TEST_NAME} PRIVATE gr-digitizers ut
                                             gnuradio-blocklib-core)
  add_test(NAME ${TEST_NAME} COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR}
                                     ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME})
endfunction()

# clang/libc++ does not support the required <chrono> features to convert from
# TAI to UTC and 'std::views::enumerate'
if(NOT EMSCRIPTEN AND NOT CLANG)
  add_ut_test(qa_timing)
  target_link_libraries(qa_timing PRIVATE timing)

  add_ut_test(qa_timingSource)
  target_link_libraries(
    qa_timingSource PRIVATE gnuradio-algorithm gnuradio-core gr-basic
                            gr-testing timing gnuradio-blocklib-core)
endif()
