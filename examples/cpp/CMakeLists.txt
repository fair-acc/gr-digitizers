
#add_compile_options(-std=c++11 -fsanitize=address -O1 -fno-omit-frame-pointer -g)

add_compile_options(-std=c++17 -g)

include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${GR_ANALOG_INCLUDE_DIRS}
  ${GNURADIO_RUNTIME_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
)

link_directories(
    ${GR_ANALOG_LIBRARY_DIRS}
    ${GNURADIO_RUNTIME_LIBRARY_DIRS}
    ${Boost_LIBRARY_DIRS}
    /opt/picoscope/lib
)

# add_executable(test_tags test_tags.cc)

# target_link_libraries(test_tags
# 	${GNURADIO_RUNTIME_LIBRARIES}
# 	${GNURADIO_ANALOG_LIBRARIES}
# 	${GNURADIO_BLOCKS_LIBRARIES}
# 	${GNURADIO_PMT_LIBRARIES}
#     ${Boost_LIBRARIES}
# 	gnuradio-digitizers_39
# #	asan
# )

# INSTALL(TARGETS
#   test_tags
#   DESTINATION ${GR_DIGITIZERS_EXAMPLES_DIR}/cpp
#   COMPONENT "digitizers_examples"
# )

find_package(Gnuradio "3.9" COMPONENTS
		blocks
		fec
		fft
		filter
		analog
		digital
		dtv
		audio
		channels
		qtgui
		trellis
		uhd
		video-sdl
		vocoder
		wavelet
		zeromq
		network
		soapy
		)

# add_executable(ps4000a_test ps4000a_test.cc)
add_executable(power_calc_test power_calc_test.cc)
add_executable(mains_frequency_calc_test mains_frequency_calc_test.cc)

add_executable(ps4000a_plus_power_calc ps4000a_plus_power_calc.cc)
add_executable(ps4000a_plus_power_calc_plus_raw ps4000a_plus_power_calc_plus_raw.cc)
add_executable(ps4000a_plus_power_calc_plus_raw_two_zmq ps4000a_plus_power_calc_plus_raw_two_zmq.cc)
add_executable(ps4000a_plus_power_calc_plus_raw_three_zmq ps4000a_plus_power_calc_plus_raw_three_zmq.cc)
add_executable(ps4000a_plus_power_calc_plus_raw_three_files ps4000a_plus_power_calc_plus_raw_three_files.cc)




# target_link_libraries(ps4000a_test
# 	power_calc_test
# 	${GNURADIO_RUNTIME_LIBRARIES}
# 	${GNURADIO_ANALOG_LIBRARIES}
# 	${GNURADIO_BLOCKS_LIBRARIES}
# 	${GNURADIO_PMT_LIBRARIES}
#     ${Boost_LIBRARIES}
# 	gnuradio-digitizers_39
#     gnuradio-blocks
#     gnuradio::gnuradio-zeromq
# #	asan
# )

target_link_libraries(power_calc_test
	${GNURADIO_RUNTIME_LIBRARIES}
	${GNURADIO_ANALOG_LIBRARIES}
	${GNURADIO_BLOCKS_LIBRARIES}
	${GNURADIO_PMT_LIBRARIES}
    ${Boost_LIBRARIES}
	gnuradio-digitizers_39
	gnuradio::gnuradio-blocks
    gnuradio::gnuradio-qtgui
	gnuradio::gnuradio-filter
    gnuradio::gnuradio-fft
    gnuradio::gnuradio-analog
    gnuradio::gnuradio-zeromq
)

target_link_libraries(mains_frequency_calc_test
	${GNURADIO_RUNTIME_LIBRARIES}
	${GNURADIO_ANALOG_LIBRARIES}
	${GNURADIO_BLOCKS_LIBRARIES}
	${GNURADIO_PMT_LIBRARIES}
    ${Boost_LIBRARIES}
	gnuradio-digitizers_39
	gnuradio::gnuradio-blocks
    gnuradio::gnuradio-qtgui
    gnuradio::gnuradio-analog
    gnuradio::gnuradio-zeromq
)

target_link_libraries(ps4000a_plus_power_calc
	${GNURADIO_RUNTIME_LIBRARIES}
	${GNURADIO_ANALOG_LIBRARIES}
	${GNURADIO_BLOCKS_LIBRARIES}
	${GNURADIO_PMT_LIBRARIES}
    ${Boost_LIBRARIES}
	gnuradio-digitizers_39
	gnuradio::gnuradio-blocks
	gnuradio::gnuradio-filter
    gnuradio::gnuradio-qtgui
    gnuradio::gnuradio-fft
    gnuradio::gnuradio-analog
    gnuradio::gnuradio-zeromq
)

target_link_libraries(ps4000a_plus_power_calc_plus_raw
	${GNURADIO_RUNTIME_LIBRARIES}
	${GNURADIO_ANALOG_LIBRARIES}
	${GNURADIO_BLOCKS_LIBRARIES}
	${GNURADIO_PMT_LIBRARIES}
    ${Boost_LIBRARIES}
	gnuradio-digitizers_39
	gnuradio::gnuradio-blocks
	gnuradio::gnuradio-filter
    gnuradio::gnuradio-qtgui
    gnuradio::gnuradio-fft
    gnuradio::gnuradio-analog
    gnuradio::gnuradio-zeromq
)

target_link_libraries(ps4000a_plus_power_calc_plus_raw_two_zmq
	${GNURADIO_RUNTIME_LIBRARIES}
	${GNURADIO_ANALOG_LIBRARIES}
	${GNURADIO_BLOCKS_LIBRARIES}
	${GNURADIO_PMT_LIBRARIES}
    ${Boost_LIBRARIES}
	gnuradio-digitizers_39
	gnuradio::gnuradio-blocks
	gnuradio::gnuradio-filter
    gnuradio::gnuradio-qtgui
    gnuradio::gnuradio-fft
    gnuradio::gnuradio-analog
    gnuradio::gnuradio-zeromq
)

target_link_libraries(ps4000a_plus_power_calc_plus_raw_three_zmq
	${GNURADIO_RUNTIME_LIBRARIES}
	${GNURADIO_ANALOG_LIBRARIES}
	${GNURADIO_BLOCKS_LIBRARIES}
	${GNURADIO_PMT_LIBRARIES}
    ${Boost_LIBRARIES}
	gnuradio-digitizers_39
	gnuradio::gnuradio-blocks
	gnuradio::gnuradio-filter
    gnuradio::gnuradio-qtgui
    gnuradio::gnuradio-fft
    gnuradio::gnuradio-analog
    gnuradio::gnuradio-zeromq
)

target_link_libraries(ps4000a_plus_power_calc_plus_raw_three_files
	${GNURADIO_RUNTIME_LIBRARIES}
	${GNURADIO_ANALOG_LIBRARIES}
	${GNURADIO_BLOCKS_LIBRARIES}
	${GNURADIO_PMT_LIBRARIES}
    ${Boost_LIBRARIES}
	gnuradio-digitizers_39
	gnuradio::gnuradio-blocks
	gnuradio::gnuradio-filter
    gnuradio::gnuradio-qtgui
    gnuradio::gnuradio-fft
    gnuradio::gnuradio-analog
    gnuradio::gnuradio-zeromq
)

INSTALL(TARGETS
	ps4000a_plus_power_calc_plus_raw_three_files
	ps4000a_plus_power_calc_plus_raw_three_zmq
	ps4000a_plus_power_calc_plus_raw_two_zmq
	ps4000a_plus_power_calc_plus_raw
	ps4000a_plus_power_calc
	power_calc_test
	mains_frequency_calc_test
	DESTINATION ${GR_DIGITIZERS_EXAMPLES_DIR}/cpp
	COMPONENT "digitizers_examples"
)

# INSTALL(TARGETS
#   ps4000a_test
#   DESTINATION ${GR_DIGITIZERS_EXAMPLES_DIR}/cpp
#   COMPONENT "digitizers_examples"
# )


#add_executable(edge_trigger_cli edge_trigger_cli.cc)

#target_link_libraries(edge_trigger_cli
#    ${Boost_LIBRARIES}
#	gnuradio-digitizers
##	asan
#)

#INSTALL(TARGETS
#  edge_trigger_cli
#  DESTINATION ${GR_DIGITIZERS_EXAMPLES_DIR}/cpp
#  COMPONENT "digitizers_examples"
#)


