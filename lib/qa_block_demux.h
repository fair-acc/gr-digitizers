/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef _QA_BLOCK_DEMUX_H_
#define _QA_BLOCK_DEMUX_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
  namespace digitizers {

    class qa_block_demux : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_block_demux);
      //CPPUNIT_TEST(passes_only_desired); FIXME: Need to fix this block before usage
      CPPUNIT_TEST_SUITE_END();

    private:
      void passes_only_desired();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_BLOCK_DEMUX_H_ */

