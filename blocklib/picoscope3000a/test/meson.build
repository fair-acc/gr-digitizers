###################################################
#    QA
###################################################

testdeps = [cppunit_dep, gnuradio_blocklib_picoscope3000a_dep, gr_blocklib_blocks, gr_blocklib_streamops]

qa_picoscope_3000a = executable('qa_picoscope_3000a', 'qa_picoscope_3000a.cc',
                                dependencies: testdeps,
                                link_args: linklibs)

# TODO only run when explicitly request per cmd-line arg
test('Picoscope 3000a', qa_picoscope_3000a)
