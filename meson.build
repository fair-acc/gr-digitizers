###############################################################################
# If this is an OOT module, the following top level project things are needed
###############################################################################
project('gr4-digitizers', 'cpp', 
  version : '0.0.0',
  license : 'GPLv3',
  default_options : ['cpp_std=c++20'])

# Import python3 meson module which is used to find the Python dependencies.
# Python3 is required for code generation
py3 = import('python').find_installation('python3')
py3_version = py3.language_version()
if py3_version.version_compare('< 3.6')
  error('Invalid python version!?')
endif

GR_ENABLE_PYTHON = false
if (get_option('enable_python'))
python3_dep = dependency('python3', required : get_option('enable_python'))
python3_embed_dep = dependency('python3-embed', required : get_option('enable_python'))
pybind11_dep = dependency('pybind11', required : get_option('enable_python'))
incdir_numpy = run_command(py3,
  ['-c', 'import os; os.chdir(".."); import numpy; print(numpy.get_include())'],
  check : true
).stdout().strip()

GR_ENABLE_PYTHON = true
endif

py3 = import('python').find_installation('python3')
json_dep = dependency('nlohmann_json')

enable_picoscope = get_option('enable_picoscope')

if (get_option('enable_testing'))
    TEST_ENV = environment()
    TEST_ENV.prepend('LD_LIBRARY_PATH',
      join_paths( meson.build_root(),'blocklib','digitizers',get_option('libdir')),
    )
    TEST_ENV.prepend('PYTHONPATH', join_paths(meson.build_root(),'blocklib/digitizers/python'))

    if (enable_picoscope)
      TEST_ENV.prepend('LD_LIBRARY_PATH',
        join_paths( meson.build_root(),'blocklib','picoscope4000a',get_option('libdir')),
      )
      TEST_ENV.prepend('PYTHONPATH', join_paths(meson.build_root(),'blocklib/picoscope4000a/python'))
    endif
endif

prefix = get_option('prefix')
# Escape spaces
prefix_parts = prefix.split(' ')
prefix = '\ '.join(prefix_parts)

cuda_available = add_languages('cuda', required : false)
IMPLEMENT_CUDA = cuda_available and get_option('enable_cuda')
IMPLEMENT_CPU = true

###############################################################################
# For OOT modules we need script path from gnuradio, so let's look for
# gnuradio-runtime already here
###############################################################################
gnuradio_gr_dep = dependency('gnuradio-runtime')
###############################################################################
# IF Out of Tree Module
# The script dir is part of the gnuradio installation, not of the oot tree,
# so we have to find the install prefix of gnuradio
# Even if we build an In-Tree module, it's the correct location
###############################################################################
share_dir = join_paths(gnuradio_gr_dep.get_pkgconfig_variable('prefix'),'share')
SCRIPTS_DIR = join_paths(share_dir, 'utils','blockbuilder','scripts' )
###############################################################################
# Else In-Tree module
###############################################################################
# share_dir = meson.project_source_root()

###############################################################################
# Dependencies
###############################################################################
volk_dep = dependency('volk', version : '>=2.2')
fmt_dep = dependency('fmt', method: 'cmake', modules: ['fmt::fmt'])
spdlog_dep = dependency('spdlog')
pmtf_dep = dependency('pmtf', version : '>= 0.0.2')
boost_dep = dependency('boost', modules: ['chrono', 'thread']) # TODO(PORT) add all used modules/port away from them
cppunit_dep = dependency('cppunit')

libroot_prefix = get_option('libroot_prefix')
root_dep = dependency('ROOT', method : 'cmake', components : ['MathCore', 'Graf'], modules : ['ROOT::Graf', 'ROOT::MathCore'], cmake_module_path : libroot_prefix + '/cmake')

if enable_picoscope
  libpicoscope_prefix = get_option('libpicoscope_prefix')
  libpicoscope_includedir = join_paths(libpicoscope_prefix, 'include')
  libpicoscope_libdir = join_paths(libpicoscope_prefix, 'lib')

  # TODO(PORT) do at least some basic checks whether these includes/libs exist
  # HACK: ps4000aApi.h includes PicoCallback.h, which is only in libps5000a and libps6000a
  callback_includedir = join_paths(libpicoscope_includedir, 'libps6000a')
  libpicoscope3000a_dep = declare_dependency(include_directories : join_paths(libpicoscope_includedir, 'libps3000a'), link_args : ['-L' + libpicoscope_libdir, '-lps3000a'])
  libpicoscope4000a_dep = declare_dependency(include_directories : [join_paths(libpicoscope_includedir, 'libps4000a'), callback_includedir], link_args : ['-L' + libpicoscope_libdir, '-lps4000a'])
  libpicoscope6000_dep = declare_dependency(include_directories : join_paths(libpicoscope_includedir, 'libps6000'), link_args : ['-L' + libpicoscope_libdir, '-lps6000'])
endif

if GR_ENABLE_PYTHON
run_command('python3', join_paths(SCRIPTS_DIR,'gen_meson.py'), 
  join_paths(meson.project_source_root(),'blocklib'), '--enable_python', check: true)
else
run_command('python3', join_paths(SCRIPTS_DIR,'gen_meson.py'), 
  join_paths(meson.project_source_root(),'blocklib'), check: true)
endif
  
subdir('blocklib/digitizers')

if enable_picoscope
  subdir('blocklib/picoscope3000a')
  subdir('blocklib/picoscope4000a')
  subdir('blocklib/picoscope6000')
endif
