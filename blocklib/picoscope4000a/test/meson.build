###################################################
#    QA
###################################################

testdeps = [cppunit_dep, gnuradio_blocklib_picoscope4000a_dep, gr_blocklib_blocks, gr_blocklib_streamops]

qa_picoscope_4000a = executable('qa_picoscope_4000a', 'qa_picoscope_4000a.cc',
                                dependencies: testdeps)

# TODO only run when explicitly request per cmd-line arg
test('Picoscope 4000a', qa_picoscope_4000a)
