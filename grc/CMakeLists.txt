# Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
# co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
# You may use, distribute and modify this code under the terms of the GPL v.3  license.

install(FILES
    digitizers_time_domain_sink.xml
    #digitizers_extractor.xml
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
    digitizers_simulation_source.xml
    digitizers_picoscope_6000.xml
    digitizers_time_realignment_ff.xml
    digitizers_interlock_generation_ff.xml
    digitizers_stream_to_vector_overlay_ff.xml
    digitizers_stft_goertzl_dynamic_decimated.xml
    digitizers_function_ff.xml 
    digitizers_block_complex_to_mag_deg.xml
    digitizers_edge_trigger_receiver_f.xml
    digitizers_cascade_sink.xml
    digitizers_wr_receiver_f.xml
    digitizers_demux_ff.xml DESTINATION share/gnuradio/grc/blocks
)
