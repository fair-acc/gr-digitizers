###################################################
#    QA
###################################################

testdeps = [cppunit_dep, gnuradio_blocklib_limesdr_dep]
linklibs = ['-lgnuradio-runtime', '-lgnuradio-blocklib-blocks', '-lgnuradio-blocklib-streamops'] # TODO(PORT) only link necessary

qa_limesdr = executable('qa_limesdr', 'qa_limesdr.cc',
                        dependencies: testdeps,
                        link_args: linklibs)

# TODO only run when explicitly request per cmd-line arg
test('LimeSDR', qa_limesdr)
