/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef _QA_DIGITIZERS_H_
#define _QA_DIGITIZERS_H_

#include <gnuradio/attributes.h>
#include <cppunit/TestSuite.h>

//! collect all the tests for the gr-filter directory

class __GR_ATTR_EXPORT qa_digitizers
{
 public:
  //! return suite of tests for all of gr-filter directory
  static CppUnit::TestSuite *suite(bool add_ps3000a_tests, bool add_ps4000a_tests, bool add_ps6000_tests);
};

#endif /* _QA_DIGITIZERS_H_ */
