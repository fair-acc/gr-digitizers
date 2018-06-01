/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef _QA_BLOCK_AGGREGATION_H_
#define _QA_BLOCK_AGGREGATION_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
  namespace digitizers {

    class qa_block_aggregation : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_block_aggregation);
      CPPUNIT_TEST(basic_connection);
      CPPUNIT_TEST(test_tags);
      CPPUNIT_TEST_SUITE_END();

    private:
      void basic_connection();
      void test_tags();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_BLOCK_AGGREGATION_H_ */

