# Copyright 2016 Free Software Foundation, Inc.
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

#add_compile_options(-std=c++11 -fsanitize=address -O1 -fno-omit-frame-pointer -g)

add_compile_options(-std=c++11 -g)

include_directories(
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

add_executable(test_tags test_tags.cc)

target_link_libraries(test_tags
	${GNURADIO_RUNTIME_LIBRARIES}
	${GNURADIO_ANALOG_LIBRARIES}
	${GNURADIO_BLOCKS_LIBRARIES}
	${GNURADIO_PMT_LIBRARIES}
    ${Boost_LIBRARIES}
	gnuradio-digitizers
#	asan
)

INSTALL(TARGETS
  test_tags
  DESTINATION ${GR_DIGITIZERS_EXAMPLES_DIR}/cpp
  COMPONENT "digitizers_examples"
)

add_executable(ps3000a_test ps3000a_test.cc)

target_link_libraries(ps3000a_test
	${GNURADIO_RUNTIME_LIBRARIES}
	${GNURADIO_ANALOG_LIBRARIES}
	${GNURADIO_BLOCKS_LIBRARIES}
	${GNURADIO_PMT_LIBRARIES}
    ${Boost_LIBRARIES}
	gnuradio-digitizers
#	asan
)

INSTALL(TARGETS
  ps3000a_test
  DESTINATION ${GR_DIGITIZERS_EXAMPLES_DIR}/cpp
  COMPONENT "digitizers_examples"
)

add_executable(edge_trigger_cli edge_trigger_cli.cc)

target_link_libraries(edge_trigger_cli
    ${Boost_LIBRARIES}
	gnuradio-digitizers
#	asan
)

INSTALL(TARGETS
  edge_trigger_cli
  DESTINATION ${GR_DIGITIZERS_EXAMPLES_DIR}/cpp
  COMPONENT "digitizers_examples"
)


