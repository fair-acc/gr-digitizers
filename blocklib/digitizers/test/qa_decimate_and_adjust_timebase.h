#ifndef _QA_DECIMATE_AND_ADJUST_TIMEBASE_H_
#define _QA_DECIMATE_AND_ADJUST_TIMEBASE_H_

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

namespace gr::digitizers {

class qa_decimate_and_adjust_timebase : public CppUnit::TestCase
{
public:
    CPPUNIT_TEST_SUITE(qa_decimate_and_adjust_timebase);
    CPPUNIT_TEST(test_decimation);
    CPPUNIT_TEST(offset_trigger_tag_test);
    CPPUNIT_TEST_SUITE_END();

private:
    void test_decimation();
    void offset_trigger_tag_test();
    void test_single_decim_factor(int n, int d);
};

} /* namespace gr::digitizers */

#endif /* _QA_DECIMATE_AND_ADJUST_TIMEBASE_H_ */
