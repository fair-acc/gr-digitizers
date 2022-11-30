#include "edge_trigger_utils.h"
#include "qa_edge_trigger.h"

#include <gnuradio/attributes.h>
#include <gnuradio/digitizers/edge_trigger.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

namespace gr::digitizers {

void qa_edge_trigger::decode()
{
    std::string payload = "<edgeDetect"
                          " edge=\"rising\""
                          " val=\"0.6545\""
                          " timingEventTimeStamp=\"-1\""
                          " retriggerEventTimeStamp=\"9876543210\""
                          " delaySinceLastTimingEvent=\"123456\""
                          " samplesSinceLastTimingEvent=\"654321\" />";

    edge_detect_t edge;
    auto decoded = decode_edge_detect(payload, edge);
    CPPUNIT_ASSERT_EQUAL(true, decoded);

    CPPUNIT_ASSERT_EQUAL(true, edge.is_raising_edge);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.6545, edge.value, 1E-6);
    CPPUNIT_ASSERT_EQUAL(int64_t{ -1 }, edge.timing_event_timestamp);
    CPPUNIT_ASSERT_EQUAL(int64_t{ 9876543210 }, edge.retrigger_event_timestamp);
    CPPUNIT_ASSERT_EQUAL(int64_t{ 123456 }, edge.delay_since_last_timing_event);
    CPPUNIT_ASSERT_EQUAL(int64_t{ 654321 }, edge.samples_since_last_timing_event);
}

void qa_edge_trigger::encode_decode()
{
    edge_detect_t test_edge;
    test_edge.is_raising_edge = false;
    test_edge.value = -2.33;
    test_edge.retrigger_event_timestamp = 1234567890;
    test_edge.delay_since_last_timing_event = 1000;
    test_edge.samples_since_last_timing_event = 50;

    std::string payload;
    CPPUNIT_ASSERT_NO_THROW(payload = encode_edge_detect(test_edge););

    edge_detect_t edge;
    auto decoded = decode_edge_detect(payload, edge);
    CPPUNIT_ASSERT_EQUAL(true, decoded);

    CPPUNIT_ASSERT_EQUAL(test_edge.is_raising_edge, edge.is_raising_edge);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(test_edge.value, edge.value, 1E-6);
    CPPUNIT_ASSERT_EQUAL(test_edge.timing_event_timestamp, edge.timing_event_timestamp);
    CPPUNIT_ASSERT_EQUAL(test_edge.retrigger_event_timestamp,
                         edge.retrigger_event_timestamp);
    CPPUNIT_ASSERT_EQUAL(test_edge.delay_since_last_timing_event,
                         edge.delay_since_last_timing_event);
    CPPUNIT_ASSERT_EQUAL(test_edge.samples_since_last_timing_event,
                         edge.samples_since_last_timing_event);
}

} /* namespace gr::digitizers */

int main(int, char**)
{
    CppUnit::TextTestRunner runner;
    runner.setOutputter(
        CppUnit::CompilerOutputter::defaultOutputter(&runner.result(), std::cerr));
    runner.addTest(gr::digitizers::qa_edge_trigger::suite());

    bool was_successful = runner.run("", false);
    return was_successful ? 0 : 1;
}
