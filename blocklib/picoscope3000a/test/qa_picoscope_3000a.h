#pragma once

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>
#include <picoscope3000a/picoscope3000a.h>

namespace gr::picoscope3000a {

class qa_picoscope_3000a : public CppUnit::TestCase
{
public:
    CPPUNIT_TEST_SUITE(qa_picoscope_3000a);
    CPPUNIT_TEST(open_close);
    CPPUNIT_TEST(rapid_block_basics);
    CPPUNIT_TEST(rapid_block_channels);
    CPPUNIT_TEST(rapid_block_continuous);
    CPPUNIT_TEST(rapid_block_downsampling_basics);
    CPPUNIT_TEST(rapid_block_downsampling);
    CPPUNIT_TEST(rapid_block_tags);
    CPPUNIT_TEST_SUITE_END();

private:
    void run_rapid_block_downsampling(digitizer_downsampling_mode_t mode);

    void setUpDevice();
    void open_close();
    void rapid_block_basics();
    void rapid_block_channels();
    void rapid_block_continuous();
    void rapid_block_downsampling_basics();
    void rapid_block_downsampling();
    void rapid_block_tags();
    void rapid_block_trigger();

    void streaming_basics();
};
} // namespace gr::picoscope3000a
