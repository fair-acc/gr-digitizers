# Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
# co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
# You may use, distribute and modify this code under the terms of the GPL v.3  license.

########################################################################
# Handle the generated constants
########################################################################
execute_process(COMMAND ${PYTHON_EXECUTABLE} -c
    "import time;print time.strftime('%a, %d %b %Y %H:%M:%S', time.gmtime())"
    OUTPUT_VARIABLE BUILD_DATE OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "Loading build date ${BUILD_DATE} into constants...")
message(STATUS "Loading version ${VERSION} into constants...")

#double escape for windows backslash path separators
string(REPLACE "\\" "\\\\" prefix "${prefix}")
string(REPLACE "\\" "\\\\" SYSCONFDIR "${SYSCONFDIR}")
string(REPLACE "\\" "\\\\" GR_PREFSDIR "${GR_PREFSDIR}")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/constants.cc.in
    ${CMAKE_CURRENT_BINARY_DIR}/constants.cc
    ESCAPE_QUOTES
@ONLY)

list(APPEND digitizers_sources ${CMAKE_CURRENT_BINARY_DIR}/constants.cc
    edge_trigger_receiver_f_impl.cc
    cascade_sink_impl.cc
    wr_receiver_f_impl.cc
    demux_ff_impl.cc)

########################################################################
# Setup library
########################################################################
include(GrPlatform) #define LIB_SUFFIX


include_directories(${Boost_INCLUDE_DIR}
/opt/picoscope/include
${ROOT_INCLUDE_DIRS}
${GR_INCLUDE_DIR}
)
link_directories(${Boost_LIBRARY_DIRS}
/opt/picoscope/lib
${ROOT_LIBRARY_DIRS}
)
list(APPEND digitizers_sources
    simulation_source_impl.cc
    time_domain_sink_impl.cc
    #extractor_impl.cc
    digitizer_block_impl.cc
    picoscope_impl.cc
    picoscope_3000a_impl.cc
    picoscope_4000a_impl.cc
    block_custom_filter_impl.cc
    block_aggregation_impl.cc
    aggregation_helper_impl.cc
    stft_algorithms_impl.cc
    block_amplitude_and_phase_impl.cc
    amplitude_and_phase_helper_impl.cc
    freq_estimator_impl.cc
    chi_square_fit_impl.cc
    peak_detector_impl.cc
    block_spectral_peaks_impl.cc
    median_and_average_impl.cc
    post_mortem_sink_impl.cc
    block_demux_impl.cc
    decimate_and_adjust_timebase_impl.cc
    signal_averager_impl.cc
    block_scaling_offset_impl.cc
    amplitude_phase_adjuster_impl.cc
    edge_trigger_ff_impl.cc
    freq_sink_f_impl.cc
    picoscope_6000_impl.cc
    stft_goertzl_dynamic_impl.cc
    time_realignment_ff_impl.cc
    interlock_generation_ff_impl.cc
    stream_to_vector_overlay_ff_impl.cc
    stft_goertzl_dynamic_decimated_impl.cc
    function_ff_impl.cc
    block_complex_to_mag_deg_impl.cc )

set(digitizers_sources "${digitizers_sources}" PARENT_SCOPE)
if(NOT digitizers_sources)
	MESSAGE(STATUS "No C++ sources... skipping lib/")
	return()
endif(NOT digitizers_sources)

add_library(gnuradio-digitizers SHARED ${digitizers_sources})
target_link_libraries(gnuradio-digitizers 
    Boost::chrono
    gnuradio::gnuradio-fft
    gnuradio::gnuradio-filter
    gnuradio::gnuradio-runtime
    ps3000a
    ps4000a
    ps6000
    # ROOT's cmake does not work correctly on some platforms, if you want
    # to link against libs that are really needed uncomment the following two lines:
    #/usr/lib64/root/libMathCore.so
    #/usr/lib64/root/libGraf.so
    ${ROOT_LIBRARIES}
)
set_target_properties(gnuradio-digitizers PROPERTIES DEFINE_SYMBOL "gnuradio_digitizers_EXPORTS")

if(APPLE)
    set_target_properties(gnuradio-digitizers PROPERTIES
        INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
    )
endif(APPLE)


if(ENABLE_STATIC_LIBS)
  add_library(gnuradio-digitizers_static STATIC ${digitizers_sources})

  if(NOT WIN32)
    set_target_properties(gnuradio-digitizers_static
      PROPERTIES OUTPUT_NAME gnuradio-digitizers)
  endif(NOT WIN32)

  install(TARGETS gnuradio-digitizers_static
    ARCHIVE DESTINATION lib${LIB_SUFFIX} COMPONENT "digitizers_devel"   # .lib file
    )
endif(ENABLE_STATIC_LIBS)


########################################################################
# Install built library files
########################################################################
include(GrMiscUtils)
GR_LIBRARY_FOO(gnuradio-digitizers RUNTIME_COMPONENT "digitizers_runtime" DEVEL_COMPONENT "digitizers_devel")

########################################################################
# Build and register unit test
########################################################################
include(GrTest)

include_directories(${CPPUNIT_INCLUDE_DIRS})
list(APPEND test_digitizers_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/test_digitizers.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_digitizers.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_demux_ff.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_cascade_sink.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_function_ff.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_stft_goertzl_dynamic_decimated.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_stream_to_vector_overlay_ff.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_interlock_generation_ff.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_time_realignment_ff.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_stft_goertzl_dynamic.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_picoscope_6000.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_freq_sink_f.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_edge_trigger_ff.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_block_scaling_offset.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_signal_averager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_decimate_and_adjust_timebase.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_block_demux.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_median_and_average.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_block_spectral_peaks.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_peak_detector.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_chi_square_fit.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_freq_estimator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_block_amplitude_and_phase.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_block_aggregation.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_picoscope_3000a.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_picoscope_4000a.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_time_domain_sink.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_digitizer_block.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_post_mortem_sink.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_stft_algorithms.cc
)

add_executable(test-digitizers ${test_digitizers_sources})

target_link_libraries(
  test-digitizers
  gnuradio::gnuradio-blocks
  gnuradio::gnuradio-fft
  gnuradio::gnuradio-filter
  gnuradio::gnuradio-runtime
  Boost::chrono
  cppunit
  gnuradio-digitizers
)

GR_ADD_TEST(test_digitizers test-digitizers)

########################################################################
# Print summary
########################################################################
message(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Building for version: ${VERSION} / ${LIBVER}")

