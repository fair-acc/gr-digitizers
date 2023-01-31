#pragma once

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>
#include <limesdr/limesdr.h>

namespace gr::limesdr {

class qa_limesdr : public CppUnit::TestCase
{
public:
    CPPUNIT_TEST_SUITE(qa_limesdr);
    CPPUNIT_TEST(open_close);
    CPPUNIT_TEST_SUITE_END();

private:
    void open_close();
};

} // namespace gr::limesdr
