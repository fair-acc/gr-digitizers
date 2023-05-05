limesdr_sources += []
limesdr_deps += [gnuradio_blocklib_digitizers_dep, limesuite_dep]

block_cpp_args = ['-DHAVE_CPU']


# TODO(PORT) move digitizer_impl etc. to a saner location
digitizer_impl_includedir = '../../digitizers/lib'

incdir = include_directories(['../include/gnuradio/limesdr','../include']) # Used by python bindings
gnuradio_blocklib_limesdr_lib = library('gnuradio-blocklib-limesdr',
    limesdr_sources,
    include_directories : [digitizer_impl_includedir, '../include/gnuradio/limesdr','../include', '../lib'], # TODO check where to put "private" headers not to be installed but used by tests
    install : true,
    link_language: 'cpp',
    dependencies : limesdr_deps,
    link_args : ['-lgnuradio-runtime'],
    cpp_args : block_cpp_args)

gnuradio_blocklib_limesdr_dep = declare_dependency(include_directories : ['../include/gnuradio', '../include', '../lib'], # TODO see above for ../lib
					   link_with : gnuradio_blocklib_limesdr_lib,
                       dependencies : limesdr_deps)
