/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef _QA_SIGNAL_AVERAGER_H_
#define _QA_SIGNAL_AVERAGER_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
  namespace digitizers {

    class qa_signal_averager : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_signal_averager);
      CPPUNIT_TEST(single_input_test);
      CPPUNIT_TEST(multiple_input_test);
      CPPUNIT_TEST_SUITE_END();

    private:
      void single_input_test();
      void multiple_input_test();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_SIGNAL_AVERAGER_H_ */

