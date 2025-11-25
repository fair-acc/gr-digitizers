  find_package(PkgConfig)

  pkg_check_modules(zlib REQUIRED IMPORTED_TARGET zlib)
  pkg_check_modules(libusb REQUIRED IMPORTED_TARGET libusb-1.0)

  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_PicoScope QUIET PicoScope)
  endif()

  #find_path(PicoScope_INCLUDE_DIR
  #  NAMES foo.h
  #  HINTS ${PC_PicoScope_INCLUDE_DIRS}
  #  PATH_SUFFIXES Foo
  #)
  #find_library(Foo_LIBRARY
  #  NAMES foo
  #  HINTS ${PC_Foo_LIBRARY_DIRS}
  #)
  set(PicoScope_VERSION ${PC_PicoScope_VERSION})

  #include(FindPackageHandleStandardArgs)
  #find_package_handle_standard_args(PicoScope
  #  REQUIRED_VARS
  #    PicoScope_LIBRARY
  #    PicoScope_INCLUDE_DIR
  #  HANDLE_COMPONENTS
  #  VERSION_VAR PicoScope_VERSION
  #)

  #if(PicoScope_FOUND AND NOT TARGET PicoScope::PicoScope)
  #  add_library(PicoScope::PicoScope UNKNOWN IMPORTED)
  #  set_target_properties(PicoScope::PicoScope PROPERTIES
  #    IMPORTED_LOCATION "${PicoScope_LIBRARY}"
  #    INTERFACE_COMPILE_OPTIONS "${PC_PicoScope_CFLAGS_OTHER}"
  #    INTERFACE_INCLUDE_DIRECTORIES "${PicoScope_INCLUDE_DIR}"
  #  )
  #endif()

  #mark_as_advanced(
  #  PicoScope_INCLUDE_DIR
  #  PicoScope_LIBRARY
  #)

  # Legacy ad-hoc discovery from fixed path
  set(PICOSCOPE_PREFIX "/opt/picoscope" CACHE PATH "Picoscope drivers prefix") # TODO use proper find_package

  MESSAGE(INFO "using picoscope library from ${PICOSCOPE_PREFIX}")
  # Add 3000a library
  add_library(PicoScope::ps3000a SHARED IMPORTED GLOBAL)
  set_property(TARGET PicoScope::ps3000a PROPERTY IMPORTED_LOCATION
          ${PICOSCOPE_PREFIX}/lib/libps3000a.so)
  target_link_libraries(PicoScope::ps3000a INTERFACE PkgConfig::zlib PkgConfig::libusb)
  target_include_directories(PicoScope::ps3000a
          INTERFACE ${PICOSCOPE_PREFIX}/include/libps3000a)

  # Add 4000a library
  add_library(PicoScope::ps4000a SHARED IMPORTED GLOBAL)
  set_property(TARGET PicoScope::ps4000a PROPERTY IMPORTED_LOCATION
          ${PICOSCOPE_PREFIX}/lib/libps4000a.so)
  target_link_libraries(PicoScope::ps4000a INTERFACE PkgConfig::zlib PkgConfig::libusb)
  target_include_directories(
          PicoScope::ps4000a INTERFACE ${PICOSCOPE_PREFIX}/include/libps4000a
          ${PICOSCOPE_PREFIX}/include/libps5000a) # Hack: PicoCallback.h is missing in libPicoScope::ps4000a/

  # Add 5000a library
  add_library(PicoScope::ps5000a SHARED IMPORTED GLOBAL)
  set_property(TARGET PicoScope::ps5000a PROPERTY IMPORTED_LOCATION
          ${PICOSCOPE_PREFIX}/lib/libps5000a.so)
  target_link_libraries(PicoScope::ps5000a INTERFACE PkgConfig::zlib PkgConfig::libusb)
  target_include_directories(PicoScope::ps5000a
          INTERFACE ${PICOSCOPE_PREFIX}/include/libps5000a)

  # Add 6000a library
  add_library(PicoScope::ps6000 SHARED IMPORTED GLOBAL)
  set_property(TARGET PicoScope::ps6000 PROPERTY IMPORTED_LOCATION
          ${PICOSCOPE_PREFIX}/lib/libps6000.so)
  target_link_libraries(PicoScope::ps6000 INTERFACE PkgConfig::zlib PkgConfig::libusb)
  target_include_directories(PicoScope::ps6000
          INTERFACE ${PICOSCOPE_PREFIX}/include/libps6000)
