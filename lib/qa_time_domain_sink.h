/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef _QA_TIME_DOMAIN_SINK_H_
#define _QA_TIME_DOMAIN_SINK_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
  namespace digitizers {

    class qa_time_domain_sink : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_time_domain_sink);
      CPPUNIT_TEST(stream_basics);
      CPPUNIT_TEST(stream_values_no_tags);
      CPPUNIT_TEST(stream_values);
      CPPUNIT_TEST(stream_overflow);
      CPPUNIT_TEST(stream_callback);
      CPPUNIT_TEST(stream_acq_info_tag);
      CPPUNIT_TEST(stream_partial_readout);
      CPPUNIT_TEST_SUITE_END();

    private:
      void stream_basics();
      void stream_values_no_tags();
      void stream_values();
      void stream_overflow();
      void stream_callback();
      void stream_acq_info_tag();
      void stream_partial_readout();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_TIME_DOMAIN_SINK_H_ */

