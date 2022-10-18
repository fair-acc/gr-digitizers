#ifndef _QA_POST_MORTEM_SINK_H_
#define _QA_POST_MORTEM_SINK_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr::digitizers {

class qa_post_mortem_sink : public CppUnit::TestCase {
public:
    CPPUNIT_TEST_SUITE(qa_post_mortem_sink);
    CPPUNIT_TEST(basics);
    CPPUNIT_TEST(buffer_not_full);
    CPPUNIT_TEST(buffer_full);
    CPPUNIT_TEST(buffer_overflow);
    CPPUNIT_TEST(acq_info);
    CPPUNIT_TEST_SUITE_END();

private:
    void basics();
    void buffer_not_full();
    void buffer_full();
    void buffer_overflow();
    void acq_info();
};

} /* namespace gr::digitizers */

#endif /* _QA_POST_MORTEM_SINK_H_ */
