/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef _QA_EDGE_TRIGGER_FF_H_
#define _QA_EDGE_TRIGGER_FF_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
  namespace digitizers {

    class qa_edge_trigger_ff : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_edge_trigger_ff);
      CPPUNIT_TEST(decode);
      CPPUNIT_TEST(encode_decode);
      CPPUNIT_TEST_SUITE_END();

    private:
      void decode();
      void encode_decode();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_EDGE_TRIGGER_FF_H_ */

