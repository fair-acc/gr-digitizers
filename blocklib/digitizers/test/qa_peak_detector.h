#ifndef _QA_PEAK_DETECTOR_H_
#define _QA_PEAK_DETECTOR_H_

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

namespace gr::digitizers {

class qa_peak_detector : public CppUnit::TestCase
{
public:
    CPPUNIT_TEST_SUITE(qa_peak_detector);
    CPPUNIT_TEST(basic_peak_find);
    CPPUNIT_TEST_SUITE_END();

private:
    void basic_peak_find();
};

} /* namespace gr::digitizers */

#endif /* _QA_PEAK_DETECTOR_H_ */
