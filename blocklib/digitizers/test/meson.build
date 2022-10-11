###################################################
#    QA
###################################################

testdeps = [cppunit_dep, gnuradio_blocklib_digitizers_dep]
linklibs = ['-lgnuradio-runtime', '-lgnuradio-blocklib-blocks']

qa_block_scaling_offset = executable('qa_block_scaling_offset', 'qa_block_scaling_offset.cc',
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
qa_stft_goertzl_dynamic = executable('qa_stft_goertzl_dynamic', 'qa_stft_goertzl_dynamic.cc',
                                     dependencies: testdeps,
                                     link_args: ['-lgnuradio-runtime', '-lgnuradio-blocklib-blocks', '-lgnuradio-kernel-math'])
qa_time_domain_sink = executable('qa_time_domain_sink', 'qa_time_domain_sink.cc',
                                 dependencies: testdeps,
                                 link_args: linklibs)

test('Block Scaling Offset', qa_block_scaling_offset)
test('Interlock Generation', qa_interlock_generation)
test('Median and Average', qa_median_and_average)
test('STFT Goertzel Dynamic', qa_stft_goertzl_dynamic)
test('Peak Detector', qa_peak_detector)
#test('Time Domain Sink', qa_time_domain_sink)