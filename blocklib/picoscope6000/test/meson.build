###################################################
#    QA
###################################################

testdeps = [cppunit_dep, gnuradio_blocklib_picoscope6000_dep, gr_blocklib_blocks, gr_blocklib_streamops]

qa_picoscope_6000 = executable('qa_picoscope_6000', 'qa_picoscope_6000.cc',
                                dependencies: testdeps)

# TODO only run when explicitly request per cmd-line arg
test('Picoscope 6000', qa_picoscope_6000)
