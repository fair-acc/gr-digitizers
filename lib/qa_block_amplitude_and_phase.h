/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef _QA_BLOCK_AMPLITUDE_AND_PHASE_H_
#define _QA_BLOCK_AMPLITUDE_AND_PHASE_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
  namespace digitizers {

    class qa_block_amplitude_and_phase : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_block_amplitude_and_phase);
      CPPUNIT_TEST(find_ampl_phase);
      CPPUNIT_TEST_SUITE_END();

    private:
      void find_ampl_phase();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_BLOCK_AMPLITUDE_AND_PHASE_H_ */

