#ifndef _QA_STFT_GOERTZL_DYNAMIC_H_
#define _QA_STFT_GOERTZL_DYNAMIC_H_

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

namespace gr::digitizers {

class qa_stft_goertzl_dynamic : public CppUnit::TestCase
{
public:
    CPPUNIT_TEST_SUITE(qa_stft_goertzl_dynamic);
    CPPUNIT_TEST(basic_test);
    CPPUNIT_TEST_SUITE_END();

private:
    void basic_test();
};

} /* namespace gr::digitizers */

#endif /* _QA_STFT_GOERTZL_DYNAMIC_H_ */
