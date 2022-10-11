digitizers_sources += []
digitizers_deps += [gnuradio_gr_dep, volk_dep, fmt_dep, boost_dep, pmtf_dep, python3_embed_dep, json_dep]

block_cpp_args = ['-DHAVE_CPU']

# if IMPLEMENT_CUDA
#     block_cpp_args += '-DHAVE_CUDA'

#     gnuradio_blocklib_digitizers_cu = library('gnuradio-blocklib-digitizers-cu', 
#         digitizers_cu_sources, 
#         include_directories : incdir, 
#         install : true, 
#         dependencies : [cuda_dep])

#     gnuradio_blocklib_digitizers_cu_dep = declare_dependency(include_directories : incdir,
#                         link_with : gnuradio_blocklib_digitizers_cu,
#                         dependencies : cuda_dep)

#     digitizers_deps += [gnuradio_blocklib_digitizers_cu_dep, cuda_dep]

# endif

incdir = include_directories(['../include/gnuradio/digitizers','../include']) # Used by python bindings
gnuradio_blocklib_digitizers_lib = library('gnuradio-blocklib-digitizers',
    digitizers_sources,
    include_directories : ['../include/gnuradio/digitizers','../include', '../lib'], # TODO check where to put "private" headers not to be installed but used by tests
    install : true,
    link_language: 'cpp',
    dependencies : digitizers_deps,
    link_args : ['-lgnuradio-runtime', '-lgnuradio-kernel-fft', '-lgnuradio-kernel-math'],
    cpp_args : block_cpp_args)

gnuradio_blocklib_digitizers_dep = declare_dependency(include_directories : ['../include/gnuradio', '../include', '../lib'], # TODO see above for ../lib
					   link_with : gnuradio_blocklib_digitizers_lib,
                       dependencies : digitizers_deps)
