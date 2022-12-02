#ifndef _QA_TIME_REALIGNMENT_H_
#define _QA_TIME_REALIGNMENT_H_

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

namespace gr::digitizers {

class qa_time_realignment : public CppUnit::TestCase
{
public:
    CPPUNIT_TEST_SUITE(qa_time_realignment);

    // Currently time-realligment doe nothing
    // Possibly we "entomb" it at some later point
    //      CPPUNIT_TEST(default_case);
    //      CPPUNIT_TEST(no_timing);
    //      CPPUNIT_TEST(no_wr_events);
    //      CPPUNIT_TEST(out_of_tolerance_1);
    //      CPPUNIT_TEST(out_of_tolerance_2);

    CPPUNIT_TEST_SUITE_END();

private:
    void default_case();
    void no_timing();
    void no_wr_events();
    void out_of_tolerance_1();
    void out_of_tolerance_2();
};

} // namespace gr::digitizers

#endif /* _QA_TIME_REALIGNMENT_H_ */
