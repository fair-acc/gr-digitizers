/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef _QA_PICOSCOPE_3000A_H_
#define _QA_PICOSCOPE_3000A_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>
#include <digitizers/picoscope_3000a.h>

namespace gr {
  namespace digitizers {

    class qa_picoscope_3000a : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_picoscope_3000a);
      CPPUNIT_TEST(open_close);
      CPPUNIT_TEST(rapid_block_basics);
      CPPUNIT_TEST(rapid_block_channels);
      CPPUNIT_TEST(rapid_block_continuous);
      CPPUNIT_TEST(rapid_block_downsampling_basics);
      CPPUNIT_TEST(rapid_block_downsampling);
      CPPUNIT_TEST(rapid_block_tags);
      CPPUNIT_TEST_SUITE_END();

    private:
      void run_rapid_block_downsampling(downsampling_mode_t mode);

      picoscope_3000a::sptr createAndInitRapidBlock();
      picoscope_3000a::sptr createAndInitStream();
      void setUpDevice();
      void open_close();
      void rapid_block_basics();
      void rapid_block_channels();
      void rapid_block_continuous();
      void rapid_block_downsampling_basics();
      void rapid_block_downsampling();
      void rapid_block_tags();
      void rapid_block_trigger();

      void streaming_basics();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_PICOSCOPE_3000A_H_ */

