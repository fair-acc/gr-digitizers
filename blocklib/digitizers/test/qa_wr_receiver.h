#ifndef _QA_WR_RECEIVER_F_H_
#define _QA_WR_RECEIVER_F_H_

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

namespace gr::digitizers {

class qa_wr_receiver : public CppUnit::TestCase
{
public:
    CPPUNIT_TEST_SUITE(qa_wr_receiver);
    CPPUNIT_TEST(test);
    CPPUNIT_TEST_SUITE_END();

private:
    void test();
};

} /* namespace gr::digitizers */

#endif /* _QA_WR_RECEIVER_F_H_ */
