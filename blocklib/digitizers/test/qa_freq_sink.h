#ifndef _QA_FREQ_SINK_F_H_
#define _QA_FREQ_SINK_F_H_

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

namespace gr::digitizers {

class qa_freq_sink : public CppUnit::TestCase
{
public:
    CPPUNIT_TEST_SUITE(qa_freq_sink);
    CPPUNIT_TEST(test_metadata);
    CPPUNIT_TEST(test_sink_no_tags);
    CPPUNIT_TEST(test_sink_tags);
    CPPUNIT_TEST(test_sink_callback);
    CPPUNIT_TEST_SUITE_END();

private:
    void test_metadata();
    void test_sink_no_tags();
    void test_sink_tags();
    void test_sink_callback();
};

} /* namespace gr::digitizers */

#endif /* _QA_FREQ_SINK_F_H_ */
