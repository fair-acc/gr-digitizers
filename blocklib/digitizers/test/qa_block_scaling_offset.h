/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef _QA_BLOCK_SCALING_OFFSET_H_
#define _QA_BLOCK_SCALING_OFFSET_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr::digitizers {

class qa_block_scaling_offset : public CppUnit::TestCase {
public:
    CPPUNIT_TEST_SUITE(qa_block_scaling_offset);
    CPPUNIT_TEST(scale_and_offset);
    CPPUNIT_TEST_SUITE_END();

private:
    void scale_and_offset();
};

} /* namespace gr::digitizers */

#endif /* _QA_BLOCK_SCALING_OFFSET_H_ */
