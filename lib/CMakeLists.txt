# Copyright 2011,2012,2016 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
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
    extractor_impl.cc
    digitizer_block_impl.cc
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
    function_ff_impl.cc )

set(digitizers_sources "${digitizers_sources}" PARENT_SCOPE)
if(NOT digitizers_sources)
	MESSAGE(STATUS "No C++ sources... skipping lib/")
	return()
endif(NOT digitizers_sources)

add_library(gnuradio-digitizers SHARED ${digitizers_sources})
target_link_libraries(gnuradio-digitizers 
	${Boost_LIBRARIES} 
	${GNURADIO_ALL_LIBRARIES} 
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

  add_dependencies(gnuradio-digitizers_static
    digitizers_generated_includes)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_extractor.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_time_domain_sink.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_digitizer_block.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_post_mortem_sink.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_stft_algorithms.cc
)

add_executable(test-digitizers ${test_digitizers_sources})

target_link_libraries(
  test-digitizers
  ${GNURADIO_RUNTIME_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CPPUNIT_LIBRARIES}
  gnuradio-digitizers
)

GR_ADD_TEST(test_digitizers test-digitizers)

########################################################################
# Print summary
########################################################################
message(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Building for version: ${VERSION} / ${LIBVER}")

