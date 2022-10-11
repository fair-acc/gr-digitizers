/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef _QA_PEAK_DETECTOR_H_
#define _QA_PEAK_DETECTOR_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr::digitizers {

class qa_peak_detector : public CppUnit::TestCase {
public:
    CPPUNIT_TEST_SUITE(qa_peak_detector);
    CPPUNIT_TEST(basic_peak_find);
    CPPUNIT_TEST_SUITE_END();

private:
    void basic_peak_find();
};

} /* namespace gr::digitizers */

#endif /* _QA_PEAK_DETECTOR_H_ */
