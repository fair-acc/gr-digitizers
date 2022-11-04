#ifndef _QA_MEDIAN_AND_AVERAGE_H_
#define _QA_MEDIAN_AND_AVERAGE_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
namespace digitizers {

class qa_median_and_average : public CppUnit::TestCase {
public:
    CPPUNIT_TEST_SUITE(qa_median_and_average);
    CPPUNIT_TEST(basic_median_and_average);
    CPPUNIT_TEST_SUITE_END();

private:
    void basic_median_and_average();
};

}
} // namespace gr::digitizers

#endif /* _QA_MEDIAN_AND_AVERAGE_H_ */
