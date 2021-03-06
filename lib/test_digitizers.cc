/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

#include "qa_digitizers.h"
#include <iostream>
#include <fstream>
#include <cppunit/CompilerOutputter.h>

#include <gnuradio/block.h>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int
main (int argc, char **argv)
{
  // Allows the user to run HW related tests
  po::options_description desc("Allowed options");
  desc.add_options()
      ("help", "produce help message")
      ("enable-ps3000a-tests", "run PS3000a tests")
      ("enable-ps4000a-tests", "run PS4000a tests")
      ("enable-ps6000-tests", "run PS6000 tests")
  ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << "\n";
    return 1;
  }

  bool enable_ps3000a_tests = false;
  if (vm.count("enable-ps3000a-tests")) {
    enable_ps3000a_tests = true;
  }

  bool enable_ps4000a_tests = false;
  if (vm.count("enable-ps4000a-tests")) {
    enable_ps4000a_tests = true;
  }

  bool enable_ps6000_tests = false;
  if (vm.count("enable-ps6000-tests")) {
    enable_ps6000_tests = true;
  }

  CppUnit::TextTestRunner runner;
  runner.setOutputter(CppUnit::CompilerOutputter::defaultOutputter(
                           &runner.result(),
                           std::cerr));
  runner.addTest(qa_digitizers::suite(enable_ps3000a_tests, enable_ps4000a_tests, enable_ps6000_tests));

  bool was_successful = runner.run("", false);
  return was_successful ? 0 : 1;
}
