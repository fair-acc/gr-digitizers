/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef _QA_WR_RECEIVER_F_H_
#define _QA_WR_RECEIVER_F_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
  namespace digitizers {

    class qa_wr_receiver_f : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_wr_receiver_f);
      CPPUNIT_TEST(test);
      CPPUNIT_TEST_SUITE_END();

    private:
      void test();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_WR_RECEIVER_F_H_ */

