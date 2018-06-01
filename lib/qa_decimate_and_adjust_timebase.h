/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef _QA_DECIMATE_AND_ADJUST_TIMEBASE_H_
#define _QA_DECIMATE_AND_ADJUST_TIMEBASE_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
  namespace digitizers {

    class qa_decimate_and_adjust_timebase : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_decimate_and_adjust_timebase);
      CPPUNIT_TEST(test_decimation);
      CPPUNIT_TEST_SUITE_END();

    private:
      void test_decimation();
      void test_single_decim_factor(int n, int d);
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_DECIMATE_AND_ADJUST_TIMEBASE_H_ */

