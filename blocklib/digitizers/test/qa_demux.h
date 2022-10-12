#ifndef _QA_DEMUX_H_
#define _QA_DEMUX_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr::digitizers {

class qa_demux : public CppUnit::TestCase {
public:
    CPPUNIT_TEST_SUITE(qa_demux);
    CPPUNIT_TEST(test_no_trigger);
    CPPUNIT_TEST(test_single_trigger);
    CPPUNIT_TEST(test_multi_trigger);
    CPPUNIT_TEST(test_to_few_post_trigger_samples);
    CPPUNIT_TEST(test_triggers_lost1);
    //      CPPUNIT_TEST(test_triggers_lost2);
    CPPUNIT_TEST(test_hangup);
    CPPUNIT_TEST_SUITE_END();

private:
    void test_no_trigger();
    void test_single_trigger();
    void test_multi_trigger();
    void test_to_few_post_trigger_samples();
    void test_window_overlap();
    void test_triggers_lost1();

    // TODO: No support for overlapping trigger-tags
    //       Though we should test if a warning for skipped triggers is displayed in that case
    //  void test_triggers_lost2();
    void test_hangup();
};
} /* namespace gr::digitizers */

#endif /* _QA_DEMUX_H_ */
