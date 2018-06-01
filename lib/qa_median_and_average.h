/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef _QA_MEDIAN_AND_AVERAGE_H_
#define _QA_MEDIAN_AND_AVERAGE_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
  namespace digitizers {

    class qa_median_and_average : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_median_and_average);
      CPPUNIT_TEST(basic_median_and_average);
      CPPUNIT_TEST_SUITE_END();

    private:
      void basic_median_and_average();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_MEDIAN_AND_AVERAGE_H_ */

