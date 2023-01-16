digitizers_sources += ['digitizer_block_impl.cc', 'picoscope_impl.cc']
digitizers_deps += [gnuradio_gr_dep, volk_dep, fmt_dep, spdlog_dep, boost_dep, pmt_dep, python3_embed_dep, json_dep, root_dep]

block_cpp_args = ['-DHAVE_CPU']

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

pkg = import('pkgconfig')
libs = [gnuradio_blocklib_digitizers_lib]     # the library/libraries users need to link against
h = ['.'] # subdirectories of ${prefix}/${includedir} to add to header path
pkg.generate(libraries : libs,
             subdirs : h,
             version : meson.project_version(),
             name : 'libgnuradio-blocklib-digitizers',
             filebase : 'gnuradio-blocklib-digitizers',
             description : 'GNU Radio Digitizers component')