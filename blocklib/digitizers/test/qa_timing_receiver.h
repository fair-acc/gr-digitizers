#pragma once

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

namespace gr::digitizers {

class qa_timing_receiver : public CppUnit::TestCase
{
public:
    CPPUNIT_TEST_SUITE(qa_timing_receiver);
    CPPUNIT_TEST(periodic_interval);
    CPPUNIT_TEST(basic_zmq);
    CPPUNIT_TEST_SUITE_END();

private:
    void periodic_interval();
    void basic_zmq();
};

} /* namespace gr::digitizers */
