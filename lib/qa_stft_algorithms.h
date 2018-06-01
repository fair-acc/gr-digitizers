/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef LIB_QA_STFT_ALGORITHMS_H_
#define LIB_QA_STFT_ALGORITHMS_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
  namespace digitizers {

    class qa_stft_algorithms : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_stft_algorithms);
      CPPUNIT_TEST(test_stft_fft);
      CPPUNIT_TEST_SUITE_END();

    private:
      void test_stft_fft();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* LIB_QA_STFT_ALGORITHMS_H_ */
