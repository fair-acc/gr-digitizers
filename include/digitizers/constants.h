/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_CONSTANTS_H
#define INCLUDED_DIGITIZERS_CONSTANTS_H

#include <digitizers/api.h>
#include <string>

namespace digitizers {

  /*!
   * \brief return SYSCONFDIR. Typically ${CMAKE_INSTALL_PREFIX}/etc or /etc
   */
  DIGITIZERS_API const std::string prefix();

  /*!
   * \brief return SYSCONFDIR. Typically ${CMAKE_INSTALL_PREFIX}/etc or /etc
   */
  DIGITIZERS_API const std::string sysconfdir();

  /*!
   * \brief return preferences file directory. Typically ${SYSCONFDIR}/etc/conf.d
   */
  DIGITIZERS_API const std::string prefsdir();

  /*!
   * \brief return date/time of build, as set when 'cmake' is run
   */
  DIGITIZERS_API const std::string build_date();

  /*!
   * \brief return version string defined by cmake (GrVersion.cmake)
   */
  DIGITIZERS_API const std::string version();

  /*!
   * \brief return just the major version defined by cmake
   */
  DIGITIZERS_API const std::string major_version();

  /*!
   * \brief return just the api version defined by cmake
   */
  DIGITIZERS_API const std::string api_version();

  /*!
   * \brief returnjust the minor version defined by cmake
   */
  DIGITIZERS_API const std::string minor_version();

  /*!
   * \brief return C compiler used to build this version of GNU Radio
   */
  DIGITIZERS_API const std::string c_compiler();

  /*!
   * \brief return C++ compiler used to build this version of GNU Radio
   */
  DIGITIZERS_API const std::string cxx_compiler();

  /*!
   * \brief return C and C++ compiler flags used to build this version of GNU Radio
   */
  DIGITIZERS_API const std::string  compiler_flags();

} /* namespace digitizers */

#endif /* INCLUDED_DIGITIZERS_CONSTANTS_H */
