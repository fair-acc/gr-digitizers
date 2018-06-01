/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef _QA_FREQ_SINK_F_H_
#define _QA_FREQ_SINK_F_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
  namespace digitizers {

    class qa_freq_sink_f : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_freq_sink_f);
      CPPUNIT_TEST(test_metadata);
      CPPUNIT_TEST(test_sink_no_tags);
      CPPUNIT_TEST(test_sink_tags);
      CPPUNIT_TEST(test_sink_callback);
      CPPUNIT_TEST_SUITE_END();

    private:
      void test_metadata();
      void test_sink_no_tags();
      void test_sink_tags();
      void test_sink_callback();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_FREQ_SINK_F_H_ */

