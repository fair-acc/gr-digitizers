/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

/*
 * This class gathers together all the test cases for the gr-filter
 * directory into a single test suite.  As you create new test cases,
 * add them here.
 */

#include "qa_digitizers.h"
#include "qa_time_domain_sink.h"
//#include "qa_extractor.h"
#include "qa_picoscope_3000a.h"
#include "qa_picoscope_4000a.h"
#include "qa_picoscope_6000.h"

#include "qa_stft_goertzl_dynamic.h"
#include "qa_time_realignment_ff.h"
#include "qa_interlock_generation_ff.h"
#include "qa_stream_to_vector_overlay_ff.h"
#include "qa_stft_goertzl_dynamic_decimated.h"
#include "qa_function_ff.h"
#include "qa_cascade_sink.h"
#include "qa_demux_ff.h"

#include "qa_block_aggregation.h"
#include "qa_block_amplitude_and_phase.h"
#include "qa_digitizer_block.h"
#include "qa_freq_estimator.h"
#include "qa_chi_square_fit.h"
#include "qa_peak_detector.h"
#include "qa_block_spectral_peaks.h"
#include "qa_median_and_average.h"
#include "qa_post_mortem_sink.h"
#include "qa_block_demux.h"
#include "qa_decimate_and_adjust_timebase.h"
#include "qa_signal_averager.h"
#include "qa_block_scaling_offset.h"
#include "qa_stft_algorithms.h"
#include "qa_edge_trigger_ff.h"
#include "qa_freq_sink_f.h"
#include "qa_picoscope_6000.h"

#include "qa_stft_goertzl_dynamic.h"
#include "qa_time_realignment_ff.h"
#include "qa_interlock_generation_ff.h"
#include "qa_stream_to_vector_overlay_ff.h"
#include "qa_stft_goertzl_dynamic_decimated.h"
#include "qa_function_ff.h"
#include "qa_cascade_sink.h"
#include "qa_demux_ff.h"

CppUnit::TestSuite *
qa_digitizers::suite(bool add_ps3000a_tests, bool add_ps4000a_tests, bool add_ps6000_tests)
{
  CppUnit::TestSuite *s = new CppUnit::TestSuite("digitizers");

  s->addTest(gr::digitizers::qa_time_domain_sink::suite());
  s->addTest(gr::digitizers::qa_block_aggregation::suite());

  if (add_ps3000a_tests) {
    s->addTest(gr::digitizers::qa_picoscope_3000a::suite());
  }
  if (add_ps4000a_tests) {
    s->addTest(gr::digitizers::qa_picoscope_4000a::suite());
  }
  if (add_ps6000_tests) {
    s->addTest(gr::digitizers::qa_picoscope_6000::suite());
  }
  s->addTest(gr::digitizers::qa_block_amplitude_and_phase::suite());
  s->addTest(gr::digitizers::qa_digitizer_block::suite());
  s->addTest(gr::digitizers::qa_freq_estimator::suite());
  s->addTest(gr::digitizers::qa_chi_square_fit::suite());
  s->addTest(gr::digitizers::qa_peak_detector::suite());
  s->addTest(gr::digitizers::qa_block_spectral_peaks::suite());
  s->addTest(gr::digitizers::qa_median_and_average::suite());
  s->addTest(gr::digitizers::qa_post_mortem_sink::suite());
  s->addTest(gr::digitizers::qa_block_demux::suite());
  s->addTest(gr::digitizers::qa_decimate_and_adjust_timebase::suite());
  s->addTest(gr::digitizers::qa_signal_averager::suite());
  s->addTest(gr::digitizers::qa_block_scaling_offset::suite());
  s->addTest(gr::digitizers::qa_stft_algorithms::suite());
  s->addTest(gr::digitizers::qa_edge_trigger_ff::suite());
  s->addTest(gr::digitizers::qa_freq_sink_f::suite());

  s->addTest(gr::digitizers::qa_time_realignment_ff::suite());
  s->addTest(gr::digitizers::qa_stft_goertzl_dynamic::suite());
  s->addTest(gr::digitizers::qa_interlock_generation_ff::suite());
  s->addTest(gr::digitizers::qa_stream_to_vector_overlay_ff::suite());
  s->addTest(gr::digitizers::qa_stft_goertzl_dynamic_decimated::suite());
  s->addTest(gr::digitizers::qa_function_ff::suite());
  s->addTest(gr::digitizers::qa_cascade_sink::suite());
  s->addTest(gr::digitizers::qa_demux_ff::suite());

  return s;
}

