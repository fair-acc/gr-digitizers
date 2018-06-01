/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef _QA_STFT_GOERTZL_DYNAMIC_H_
#define _QA_STFT_GOERTZL_DYNAMIC_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
  namespace digitizers {

    class qa_stft_goertzl_dynamic : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_stft_goertzl_dynamic);
      CPPUNIT_TEST(basic_test);
      CPPUNIT_TEST_SUITE_END();

    private:
      void basic_test();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_STFT_GOERTZL_DYNAMIC_H_ */

