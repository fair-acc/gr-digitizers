###################################################
#    QA
###################################################

testdeps = [cppunit_dep, gnuradio_blocklib_digitizers_dep]
linklibs = ['-lgnuradio-runtime', '-lgnuradio-blocklib-blocks']

qa_median_and_average = executable('qa_median_and_average', 'qa_median_and_average.cc',
                                   dependencies: testdeps,
                                   link_args: linklibs)
qa_peak_detector = executable('qa_peak_detector', 'qa_peak_detector.cc',
                              dependencies: testdeps,
                              link_args: linklibs)
qa_time_domain_sink = executable('qa_time_domain_sink', 'qa_time_domain_sink.cc',
                                 dependencies: testdeps,
                                 link_args: linklibs)

test('Median and Average', qa_median_and_average)
test('Peak Detector', qa_peak_detector)
#test('Time Domain Sink', qa_time_domain_sink)