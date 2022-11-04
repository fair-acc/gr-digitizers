picoscope3000a_sources += []
picoscope3000a_deps += [gnuradio_blocklib_digitizers_dep, libpicoscope3000a_dep]

block_cpp_args = ['-DHAVE_CPU']


# TODO(PORT) move digitizer_impl etc. to a saner location
digitizer_impl_includedir = '../../digitizers/lib'

incdir = include_directories(['../include/gnuradio/picoscope3000a','../include']) # Used by python bindings
gnuradio_blocklib_picoscope3000a_lib = library('gnuradio-blocklib-picoscope3000a',
    picoscope3000a_sources,
    include_directories : [digitizer_impl_includedir, '../include/gnuradio/picoscope3000a','../include', '../lib'], # TODO check where to put "private" headers not to be installed but used by tests
    install : true,
    link_language: 'cpp',
    dependencies : picoscope3000a_deps,
    link_args : ['-lgnuradio-runtime'],
    cpp_args : block_cpp_args)

gnuradio_blocklib_picoscope3000a_dep = declare_dependency(include_directories : ['../include/gnuradio', '../include', '../lib'], # TODO see above for ../lib
					   link_with : gnuradio_blocklib_picoscope3000a_lib,
                       dependencies : picoscope3000a_deps)
