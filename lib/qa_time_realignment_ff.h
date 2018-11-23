/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef _QA_TIME_REALIGNMENT_FF_H_
#define _QA_TIME_REALIGNMENT_FF_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
  namespace digitizers {

    class qa_time_realignment_ff : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_time_realignment_ff);
      CPPUNIT_TEST(default_case);
      CPPUNIT_TEST(no_timing);
      CPPUNIT_TEST(no_wr_events);
      CPPUNIT_TEST(out_of_tolerance_1);
      CPPUNIT_TEST(out_of_tolerance_2);

      CPPUNIT_TEST_SUITE_END();

    private:
      void default_case();
      void no_timing();
      void no_wr_events();
      void out_of_tolerance_1();
      void out_of_tolerance_2();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_TIME_REALIGNMENT_FF_H_ */

