#ifndef _QA_INTERLOCK_GENERATION_H_
#define _QA_INTERLOCK_GENERATION_H_

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

namespace gr::digitizers {

class qa_interlock_generation : public CppUnit::TestCase
{
public:
    CPPUNIT_TEST_SUITE(qa_interlock_generation);
    CPPUNIT_TEST(interlock_generation_test);
    CPPUNIT_TEST_SUITE_END();

private:
    void interlock_generation_test();
};
} /* namespace gr::digitizers */

#endif /* _QA_INTERLOCK_GENERATION_H_ */
