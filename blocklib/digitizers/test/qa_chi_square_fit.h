#ifndef _QA_CHI_SQUARE_FIT_H_
#define _QA_CHI_SQUARE_FIT_H_

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

namespace gr::digitizers {

class qa_chi_square_fit : public CppUnit::TestCase
{
public:
    CPPUNIT_TEST_SUITE(qa_chi_square_fit);
    CPPUNIT_TEST(test_chi_square_simple_fitting);
    CPPUNIT_TEST_SUITE_END();

private:
    void test_chi_square_simple_fitting();
};

} /* namespace gr::digitizers */

#endif /* _QA_CHI_SQUARE_FIT_H_ */
