#ifndef _QA_BLOCK_SPECTRAL_PEAKS_H_
#define _QA_BLOCK_SPECTRAL_PEAKS_H_

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

namespace gr::digitizers {

class qa_block_spectral_peaks : public CppUnit::TestCase
{
public:
    CPPUNIT_TEST_SUITE(qa_block_spectral_peaks);
    CPPUNIT_TEST(test_spectral_peaks);
    CPPUNIT_TEST_SUITE_END();

private:
    void test_spectral_peaks();
};

} /* namespace gr::digitizers */

#endif /* _QA_BLOCK_SPECTRAL_PEAKS_H_ */
