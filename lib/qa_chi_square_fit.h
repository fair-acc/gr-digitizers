/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef _QA_CHI_SQUARE_FIT_H_
#define _QA_CHI_SQUARE_FIT_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
  namespace digitizers {

    class qa_chi_square_fit : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_chi_square_fit);
      CPPUNIT_TEST(test_chi_square_simple_fitting);
      CPPUNIT_TEST_SUITE_END();

    private:
      void test_chi_square_simple_fitting();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_CHI_SQUARE_FIT_H_ */

