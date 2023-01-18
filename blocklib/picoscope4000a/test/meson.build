###################################################
#    QA
###################################################

testdeps = [cppunit_dep, gnuradio_blocklib_picoscope4000a_dep]
linklibs = ['-lgnuradio-runtime', '-lgnuradio-blocklib-blocks', '-lgnuradio-blocklib-streamops'] # TODO(PORT) only link necessary

qa_picoscope_4000a = executable('qa_picoscope_4000a', 'qa_picoscope_4000a.cc',
                                dependencies: testdeps,
                                link_args: linklibs)

# TODO only run when explicitly request per cmd-line arg
test('Picoscope 4000a', qa_picoscope_4000a)
