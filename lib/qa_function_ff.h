/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef _QA_FUNCTION_FF_H_
#define _QA_FUNCTION_FF_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
  namespace digitizers {

    class qa_function_ff : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_function_ff);
      CPPUNIT_TEST(test_no_timing);
      CPPUNIT_TEST(test_function);
      CPPUNIT_TEST_SUITE_END();

    private:
      void test_no_timing();
      void test_function();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_FUNCTION_FF_H_ */

