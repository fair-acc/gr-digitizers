/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef _QA_DEMUX_FF_H_
#define _QA_DEMUX_FF_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
  namespace digitizers {

    class qa_demux_ff : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_demux_ff);
      CPPUNIT_TEST(test_no_trigger);
      CPPUNIT_TEST(test_single_trigger);
      CPPUNIT_TEST(test_multi_trigger);
      CPPUNIT_TEST(test_to_few_post_trigger_samples);
      CPPUNIT_TEST_SUITE_END();

    private:
      void test_no_trigger();
      void test_single_trigger();
      void test_multi_trigger();
      void test_to_few_post_trigger_samples();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_DEMUX_FF_H_ */

