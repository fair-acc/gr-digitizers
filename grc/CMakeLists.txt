# Copyright 2011 Free Software Foundation, Inc.
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
install(FILES
    digitizers_time_domain_sink.xml
    digitizers_extractor.xml
    digitizers_picoscope_3000a.xml
    digitizers_picoscope_4000a.xml
    digitizers_block_custom_filter.xml
    digitizers_block_aggregation.xml
    digitizers_stft_algorithms.xml
    digitizers_block_amplitude_and_phase.xml
    digitizers_freq_estimator.xml
    digitizers_chi_square_fit.xml
    digitizers_block_spectral_peaks.xml
    digitizers_post_mortem_sink.xml
    digitizers_block_demux.xml
    digitizers_decimate_and_adjust_timebase.xml
    digitizers_signal_averager.xml
    digitizers_block_scaling_offset.xml
    digitizers_amplitude_phase_adjuster.xml
    digitizers_edge_trigger_ff.xml
    digitizers_freq_sink_f.xml
    digitizers_picoscope_6000.xml
    digitizers_time_realignment_ff.xml
    digitizers_interlock_generation_ff.xml
    digitizers_stream_to_vector_overlay_ff.xml
    digitizers_stft_goertzl_dynamic_decimated.xml
    digitizers_function_ff.xml DESTINATION share/gnuradio/grc/blocks
)
