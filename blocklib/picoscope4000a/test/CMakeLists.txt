# for manual usage, not run by ctest
function(add_ut_test_tool TEST_NAME)
    add_executable(${TEST_NAME} ${TEST_NAME}.cc)
    target_compile_options(${TEST_NAME} PRIVATE -Wall)
    target_link_options(${TEST_NAME} PRIVATE -Wall)
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_BINARY_DIR}/include ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${TEST_NAME} PRIVATE graph-prototype gr-picoscope4000a gr-helpers ut)
endfunction()

add_ut_test_tool(qa_picoscope_4000a)
