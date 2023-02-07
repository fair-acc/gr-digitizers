#ifndef LIB_QA_DIGITIZER_BLOCK_H_
#define LIB_QA_DIGITIZER_BLOCK_H_

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cstdint>
#include <vector>

namespace gr::digitizers {

class qa_digitizer_block : public CppUnit::TestCase
{
    std::vector<float> d_cha_vec, d_chb_vec;
    std::vector<uint8_t> d_port_vec;

public:
    CPPUNIT_TEST_SUITE(qa_digitizer_block);
    CPPUNIT_TEST(rapid_block_basics);
    CPPUNIT_TEST(rapid_block_channel_b_only);
    CPPUNIT_TEST(rapid_block_correct_tags);
    CPPUNIT_TEST(streaming_basics);
    CPPUNIT_TEST(streaming_correct_tags);
    CPPUNIT_TEST(streaming_timing_no_signal);
    CPPUNIT_TEST(streaming_timing_analog_input);
    CPPUNIT_TEST(streaming_timing_digital_input);
    CPPUNIT_TEST_SUITE_END();

private:
    void fill_data(unsigned samples, unsigned presamples);
    void rapid_block_basics();
    void rapid_block_channel_b_only();
    void rapid_block_correct_tags();
    void streaming_basics();
    void streaming_correct_tags();
    void streaming_timing_no_signal();
    void streaming_timing_analog_input();
    void streaming_timing_digital_input();
};

} /* namespace gr::digitizers */

#endif /* LIB_QA_DIGITIZER_BLOCK_H_ */
