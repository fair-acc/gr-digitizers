#ifndef _QA_SIGNAL_AVERAGER_H_
#define _QA_SIGNAL_AVERAGER_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr::digitizers {

class qa_signal_averager : public CppUnit::TestCase {
public:
    CPPUNIT_TEST_SUITE(qa_signal_averager);
    CPPUNIT_TEST(single_input_test);
    CPPUNIT_TEST(multiple_input_test);
    CPPUNIT_TEST(offset_trigger_tag_test);
    CPPUNIT_TEST_SUITE_END();

private:
    void single_input_test();
    void multiple_input_test();
    void offset_trigger_tag_test();
};

} /* namespace gr::digitizers */

#endif /* _QA_SIGNAL_AVERAGER_H_ */
