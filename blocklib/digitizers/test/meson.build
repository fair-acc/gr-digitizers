###################################################
#    QA
###################################################

testdeps = [cppunit_dep, gnuradio_blocklib_digitizers_dep, gr_blocklib_blocks, gr_blocklib_math, gr_blocklib_streamops, gr_kernel_math]
linklibs = []

qa_block_demux = executable('qa_block_demux', 'qa_block_demux.cc',
                            dependencies: testdeps,
                            link_args: linklibs)
qa_block_scaling_offset = executable('qa_block_scaling_offset', 'qa_block_scaling_offset.cc',
                                     dependencies: testdeps,
                                     link_args: linklibs)
qa_block_spectral_peaks = executable('qa_block_spectral_peaks', 'qa_block_spectral_peaks.cc',
                                     dependencies: testdeps,
                                     link_args: linklibs)
qa_chi_square_fit = executable('qa_chi_square_fit', 'qa_chi_square_fit.cc',
                               dependencies: testdeps,
                               link_args: linklibs)
qa_decimate_and_adjust_timebase = executable('qa_decimate_and_adjust_timebase', 'qa_decimate_and_adjust_timebase.cc',
                                             dependencies: testdeps,
                                             link_args: linklibs)
qa_demux = executable('qa_demux', 'qa_demux.cc',
                      dependencies: testdeps,
                      link_args: linklibs)
qa_digitizer_block = executable('qa_digitizer_block', 'qa_digitizer_block.cc',
                                dependencies: testdeps,
                                link_args: linklibs)
qa_edge_trigger = executable('qa_edge_trigger', 'qa_edge_trigger.cc',
                             dependencies: testdeps,
                             link_args: linklibs)
qa_freq_estimator = executable('qa_freq_estimator', 'qa_freq_estimator.cc',
                               dependencies: testdeps,
                               link_args: linklibs)
qa_freq_sink = executable('qa_freq_sink', 'qa_freq_sink.cc',
                          dependencies: testdeps,
                          link_args: linklibs)
qa_function = executable('qa_function', 'qa_function.cc',
                         dependencies: testdeps,
                         link_args: linklibs)
qa_interlock_generation = executable('qa_interlock_generation', 'qa_interlock_generation.cc',
                                     dependencies: testdeps,
                                     link_args: linklibs)
qa_median_and_average = executable('qa_median_and_average', 'qa_median_and_average.cc',
                                   dependencies: testdeps,
                                   link_args: linklibs)
qa_peak_detector = executable('qa_peak_detector', 'qa_peak_detector.cc',
                              dependencies: testdeps,
                              link_args: linklibs)
qa_post_mortem_sink = executable('qa_post_mortem_sink', 'qa_post_mortem_sink.cc',
                                 dependencies: testdeps,
                                 link_args: linklibs)
qa_signal_averager = executable('qa_signal_averager', 'qa_signal_averager.cc', dependencies: testdeps, link_args: linklibs)
qa_stft_goertzl_dynamic = executable('qa_stft_goertzl_dynamic', 'qa_stft_goertzl_dynamic.cc',
                                     dependencies: testdeps,
                                     link_args: [])
qa_stft_goertzl_dynamic_decimated = executable('qa_stft_goertzl_dynamic_decimated', 'qa_stft_goertzl_dynamic_decimated.cc',
                                               dependencies: testdeps,
                                               link_args: [])
qa_stream_to_vector_overlay = executable('qa_stream_to_vector_overlay', 'qa_stream_to_vector_overlay.cc',
                                         dependencies: testdeps,
                                         link_args: linklibs)
qa_time_domain_sink = executable('qa_time_domain_sink', 'qa_time_domain_sink.cc',
                                 dependencies: testdeps,
                                 link_args: linklibs)
qa_timing_receiver = executable('qa_timing_receiver', 'qa_timing_receiver.cc',
                                dependencies: testdeps,
                                link_args: linklibs)
qa_wr_receiver = executable('qa_wr_receiver', 'qa_wr_receiver.cc',
                            dependencies: testdeps,
                            link_args: [])

test('Block Demux', qa_block_demux)
test('Block Scaling Offset', qa_block_scaling_offset)
test('Block Spectral Peaks', qa_block_spectral_peaks)
#test('Chi Square Fit', qa_chi_square_fit) # TODO(port) reenable (currently fails sometimes with empty sink)
test('Decimate and Adjust Timebase', qa_decimate_and_adjust_timebase)
#test('Demux', qa_demux) #TODO (port) reenable (currently hangs)
test('Digitizer Block (Simulation Source)', qa_digitizer_block)
test('Edge Trigger', qa_edge_trigger)
test('Frequency Estimator', qa_freq_estimator)
test('Frequency Sink', qa_freq_sink)
test('Function', qa_function)
test('Interlock Generation', qa_interlock_generation)
test('Median and Average', qa_median_and_average)
test('Peak Detector', qa_peak_detector)
test('Post Mortem Sink', qa_post_mortem_sink)
test('Signal Averager', qa_signal_averager)
test('STFT Goertzel Dynamic', qa_stft_goertzl_dynamic)
test('STFT Goertzel Dynamic_Decimated', qa_stft_goertzl_dynamic_decimated)
test('Stream to Vector Overlay', qa_stream_to_vector_overlay)
test('Time Domain Sink', qa_time_domain_sink)
test('Timing Receiver', qa_timing_receiver)
test('WR Receiver', qa_wr_receiver)
