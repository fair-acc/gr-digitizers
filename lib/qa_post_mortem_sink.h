/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef _QA_POST_MORTEM_SINK_H_
#define _QA_POST_MORTEM_SINK_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
  namespace digitizers {

    class qa_post_mortem_sink : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_post_mortem_sink);
      CPPUNIT_TEST(basics);
      CPPUNIT_TEST(buffer_not_full);
      CPPUNIT_TEST(buffer_full);
      CPPUNIT_TEST(buffer_overflow);
      CPPUNIT_TEST(acq_info);
      CPPUNIT_TEST_SUITE_END();

    private:
      void basics();
      void buffer_not_full();
      void buffer_full();
      void buffer_overflow();
      void acq_info();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_POST_MORTEM_SINK_H_ */

