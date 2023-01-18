picoscope4000a_sources += []
picoscope4000a_deps += [gnuradio_blocklib_digitizers_dep, libpicoscope4000a_dep]

block_cpp_args = ['-DHAVE_CPU']


# TODO(PORT) move digitizer_impl etc. to a saner location
digitizer_impl_includedir = '../../digitizers/lib'

incdir = include_directories(['../include/gnuradio/picoscope4000a','../include']) # Used by python bindings
gnuradio_blocklib_picoscope4000a_lib = library('gnuradio-blocklib-picoscope4000a',
    picoscope4000a_sources,
    include_directories : [digitizer_impl_includedir, '../include/gnuradio/picoscope4000a','../include', '../lib'], # TODO check where to put "private" headers not to be installed but used by tests
    install : true,
    link_language: 'cpp',
    dependencies : picoscope4000a_deps,
    cpp_args : block_cpp_args)

gnuradio_blocklib_picoscope4000a_dep = declare_dependency(include_directories : ['../include/gnuradio', '../include', '../lib'], # TODO see above for ../lib
					   link_with : gnuradio_blocklib_picoscope4000a_lib,
                       dependencies : picoscope4000a_deps)
