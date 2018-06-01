/*
 * qa_digitizer_block.h
 *
 *  Created on: Jan 5, 2018
 *      Author: osboxes
 */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef LIB_QA_DIGITIZER_BLOCK_H_
#define LIB_QA_DIGITIZER_BLOCK_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>
#include <vector>
#include <cstdint>

namespace gr {
  namespace digitizers {

    class qa_digitizer_block : public CppUnit::TestCase
    {
      std::vector<float> d_cha_vec, d_chb_vec;
      std::vector<uint8_t> d_port_vec;
    public:
      CPPUNIT_TEST_SUITE(qa_digitizer_block);
      CPPUNIT_TEST(rapid_block_basics);
      CPPUNIT_TEST(rapid_block_correct_tags);
      CPPUNIT_TEST(streaming_basics);
      CPPUNIT_TEST(streaming_correct_tags);
      CPPUNIT_TEST_SUITE_END();

    private:
      void fill_data(unsigned samples, unsigned presamples);
      void rapid_block_basics();
      void rapid_block_correct_tags();
      void streaming_basics();
      void streaming_correct_tags();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* LIB_QA_DIGITIZER_BLOCK_H_ */
