/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef _QA_TIME_DOMAIN_SINK_H_
#define _QA_TIME_DOMAIN_SINK_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>
#include <digitizers/tags.h>

namespace gr {
  namespace digitizers {

    class qa_time_domain_sink : public CppUnit::TestCase
    {
    public:

        class Test
        {
        public:

            Test(std::size_t size);

            ~Test();

            void callback(const float           *values,
                         std::size_t             values_size,
                         const float            *errors,
                         std::size_t             errors_size,
                         std::vector<gr::tag_t>& tags);

            void
            check_errors_zero();

            void
            check_values_equal(std::vector<float>& values);

            void
            check_errors_equal(std::vector<float>& errors);

            float *values_;
            float *errors_;

            float *p_values_;
            float *p_errors_;

            std::size_t values_size_;
            std::size_t errors_size_;

            std::size_t size_max_;

            int callback_calls_;
            std::vector< std::vector <acq_info_t> > tags_per_call_;
        };
      CPPUNIT_TEST_SUITE(qa_time_domain_sink);
      CPPUNIT_TEST(stream_basics);
      CPPUNIT_TEST(stream_values_no_tags);
      CPPUNIT_TEST(stream_values);
      CPPUNIT_TEST(stream_no_callback);
      CPPUNIT_TEST(stream_acq_info_tag);
      CPPUNIT_TEST_SUITE_END();

    private:
      void stream_basics();
      void stream_values_no_tags();
      void stream_values();
      void stream_no_callback();
      void stream_acq_info_tag();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_TIME_DOMAIN_SINK_H_ */

