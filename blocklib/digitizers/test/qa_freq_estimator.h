#ifndef _QA_FREQ_ESTIMATOR_H_
#define _QA_FREQ_ESTIMATOR_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr::digitizers {

class qa_freq_estimator : public CppUnit::TestCase {
public:
    CPPUNIT_TEST_SUITE(qa_freq_estimator);
    CPPUNIT_TEST(basic_frequency_estimation);
    CPPUNIT_TEST_SUITE_END();

private:
    void basic_frequency_estimation();
};

} /* namespace gr::digitizers */

#endif /* _QA_FREQ_ESTIMATOR_H_ */
