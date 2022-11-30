#ifndef _QA_STFT_GOERTZL_DYNAMIC_DECIMATED_H_
#define _QA_STFT_GOERTZL_DYNAMIC_DECIMATED_H_

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

namespace gr::digitizers {

class qa_stft_goertzl_dynamic_decimated : public CppUnit::TestCase
{
public:
    CPPUNIT_TEST_SUITE(qa_stft_goertzl_dynamic_decimated);
    CPPUNIT_TEST(t1);
    CPPUNIT_TEST_SUITE_END();

private:
    void t1();
};

} /* namespace gr::digitizers */

#endif /* _QA_STFT_GOERTZL_DYNAMIC_DECIMATED_H_ */
