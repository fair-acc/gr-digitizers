###################################################
#    QA
###################################################

testdeps = [cppunit_dep, gnuradio_blocklib_digitizers_dep]

qa_time_domain_sink = executable('qa_time_domain_sink', 'qa_time_domain_sink.cc',
                                 dependencies: testdeps,
                                 link_args : ['-lgnuradio-runtime', '-lgnuradio-blocklib-blocks'])

test('Time Domain Sink', qa_time_domain_sink)