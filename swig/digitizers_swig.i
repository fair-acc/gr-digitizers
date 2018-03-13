/* -*- c++ -*- */

#define DIGITIZERS_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "digitizers_swig_doc.i"

%{
#include "digitizers/simulation_source.h"
#include "digitizers/time_domain_sink.h"
#include "digitizers/extractor.h"
#include "digitizers/picoscope_3000a.h"
#include "digitizers/picoscope_4000a.h"
#include "digitizers/block_custom_filter.h"
#include "digitizers/block_aggregation.h"
#include "digitizers/aggregation_helper.h"
#include "digitizers/stft_algorithms.h"
#include "digitizers/block_amplitude_and_phase.h"
#include "digitizers/amplitude_and_phase_helper.h"
#include "digitizers/freq_estimator.h"
#include "digitizers/chi_square_fit.h"
#include "digitizers/peak_detector.h"
#include "digitizers/block_spectral_peaks.h"
#include "digitizers/median_and_average.h"
#include "digitizers/post_mortem_sink.h"
#include "digitizers/block_demux.h"
#include "digitizers/decimate_and_adjust_timebase.h"
#include "digitizers/signal_averager.h"
#include "digitizers/block_scaling_offset.h"
#include "digitizers/amplitude_phase_adjuster.h"
#include "digitizers/edge_trigger_ff.h"
#include "digitizers/freq_sink_f.h"
#include "digitizers/picoscope_6000.h"
#include "digitizers/stft_goertzl_dynamic.h"
#include "digitizers/time_realignment_ff.h"
#include "digitizers/interlock_generation_ff.h"
#include "digitizers/stream_to_vector_overlay_ff.h"
#include "digitizers/stft_goertzl_dynamic_decimated.h"
#include "digitizers/function_ff.h"
%}

%include "digitizers/range.h"
%include "digitizers/status.h"
%include "digitizers/sink_common.h"
%include "digitizers/digitizer_block.h"

%include "digitizers/simulation_source.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, simulation_source);
%include "digitizers/time_domain_sink.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, time_domain_sink);
%include "digitizers/extractor.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, extractor);
%include "digitizers/picoscope_3000a.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, picoscope_3000a);
%include "digitizers/picoscope_4000a.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, picoscope_4000a);
%include "digitizers/block_custom_filter.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, block_custom_filter);
%include "digitizers/block_aggregation.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, block_aggregation);
%include "digitizers/aggregation_helper.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, aggregation_helper);
%include "digitizers/stft_algorithms.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, stft_algorithms);
%include "digitizers/block_amplitude_and_phase.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, block_amplitude_and_phase);
%include "digitizers/amplitude_and_phase_helper.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, amplitude_and_phase_helper);

%include "digitizers/freq_estimator.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, freq_estimator);
%include "digitizers/chi_square_fit.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, chi_square_fit);
%include "digitizers/peak_detector.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, peak_detector);
%include "digitizers/block_spectral_peaks.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, block_spectral_peaks);
%include "digitizers/median_and_average.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, median_and_average);
%include "digitizers/post_mortem_sink.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, post_mortem_sink);
%include "digitizers/block_demux.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, block_demux);
%include "digitizers/decimate_and_adjust_timebase.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, decimate_and_adjust_timebase);
%include "digitizers/signal_averager.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, signal_averager);
%include "digitizers/block_scaling_offset.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, block_scaling_offset);
%include "digitizers/amplitude_phase_adjuster.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, amplitude_phase_adjuster);

%include "digitizers/edge_trigger_ff.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, edge_trigger_ff);
%include "digitizers/freq_sink_f.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, freq_sink_f);
%include "digitizers/picoscope_6000.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, picoscope_6000);

%include "digitizers/stft_goertzl_dynamic.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, stft_goertzl_dynamic);
%include "digitizers/time_realignment_ff.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, time_realignment_ff);

%include "digitizers/interlock_generation_ff.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, interlock_generation_ff);
%include "digitizers/stream_to_vector_overlay_ff.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, stream_to_vector_overlay_ff);

%include "digitizers/stft_goertzl_dynamic_decimated.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, stft_goertzl_dynamic_decimated);
%include "digitizers/function_ff.h"
GR_SWIG_BLOCK_MAGIC2(digitizers, function_ff);
