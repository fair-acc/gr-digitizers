# Copyright 2011,2012 Free Software Foundation, Inc.
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
# Install public header files
########################################################################
install(FILES
    api.h
    constants.h
    range.h
    status.h
    sink_common.h
    tags.h
    digitizer_block.h
    simulation_source.h
    time_domain_sink.h
    #extractor.h
    picoscope_3000a.h
    picoscope_4000a.h
    block_custom_filter.h
    block_aggregation.h
    aggregation_helper.h
    stft_algorithms.h
    block_amplitude_and_phase.h
    amplitude_and_phase_helper.h
    freq_estimator.h
    chi_square_fit.h
    peak_detector.h
    block_spectral_peaks.h
    median_and_average.h
    post_mortem_sink.h
    block_demux.h
    decimate_and_adjust_timebase.h
    signal_averager.h
    block_scaling_offset.h
    amplitude_phase_adjuster.h
    edge_trigger_ff.h
    edge_trigger_utils.h
    freq_sink_f.h
    picoscope_6000.h
    stft_goertzl_dynamic.h
    time_realignment_ff.h
    interlock_generation_ff.h
    stream_to_vector_overlay_ff.h
    stft_goertzl_dynamic_decimated.h
    function_ff.h 
    block_complex_to_mag_deg.h
    edge_trigger_receiver_f.h
    cascade_sink.h
    wr_receiver_f.h
    demux_ff.h DESTINATION include/digitizers
)
