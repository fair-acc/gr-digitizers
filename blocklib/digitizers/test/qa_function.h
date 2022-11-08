#ifndef _QA_FUNCTION_H_
#define _QA_FUNCTION_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr::digitizers {

class qa_function : public CppUnit::TestCase {
public:
    CPPUNIT_TEST_SUITE(qa_function);
    // CPPUNIT_TEST(test_no_timing); FIXME: Currently it is not clear what this block should do
    // CPPUNIT_TEST(test_function);
    CPPUNIT_TEST_SUITE_END();

private:
    void test_no_timing();
    void test_function();
};

} /* namespace gr::digitizers */

#endif /* _QA_FUNCTION_H_ */
