#include "qa_timing_receiver.h"

#include <digitizers/timing_receiver_simulated.h>

#include <gnuradio/blocks/message_debug.h>
#include <gnuradio/flowgraph.h>
#include <gnuradio/tag.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

#include <chrono>

namespace gr::digitizers {

void qa_timing_receiver::basic_simulation()
{
    using namespace std::chrono_literals;

    const std::string trigger_name = "TRTEST";
    constexpr int64_t interval = 250; /*ms*/
    constexpr double trigger_offset = 20.;

    auto fg = gr::flowgraph::make("basic_simulation");
    auto timing_rec = timing_receiver_simulated::make({ .trigger_name = trigger_name,
                                                        .trigger_offset = trigger_offset,
                                                        .tag_time_interval = interval });
    auto sink = blocks::message_debug::make({});

    fg->connect(timing_rec, "out", sink, "store");

    fg->start();

    const auto start = std::chrono::system_clock::now();
    const auto start_ns_since_epoch =
        std::chrono::duration_cast<std::chrono::nanoseconds>(start.time_since_epoch())
            .count();

    std::this_thread::sleep_for(1900ms);
    fg->stop();

    constexpr std::size_t num_messages_expected = 8;

    // expecting messages roughly at 0ms, 250ms, 500ms, 750ms, 1000ms, 1250ms, 1500ms,
    // 1750ms
    CPPUNIT_ASSERT_EQUAL(num_messages_expected, sink->num_messages());

    for (std::size_t i = 0; i < num_messages_expected; ++i) {
        const auto message =
            pmtf::get_as<std::map<std::string, pmtf::pmt>>(sink->get_message(i));
        CPPUNIT_ASSERT_EQUAL(
            trigger_name, pmtf::get_as<std::string>(message.at(gr::tag::TRIGGER_NAME)));
        CPPUNIT_ASSERT_EQUAL(trigger_offset,
                             pmtf::get_as<double>(message.at(gr::tag::TRIGGER_OFFSET)));

        const auto expected_timestamp_ns =
            static_cast<int64_t>(start_ns_since_epoch + i * interval * 1000000);
        const auto actual_timestamp_ns =
            pmtf::get_as<int64_t>(message.at(gr::tag::TRIGGER_TIME));
        // allow < 15 ms inaccuracy
        CPPUNIT_ASSERT(std::abs(expected_timestamp_ns - actual_timestamp_ns) <
                       15 * 1000000);
    }
}

} /* namespace gr::digitizers */

int main(int, char**)
{
    CppUnit::TextTestRunner runner;
    runner.setOutputter(
        CppUnit::CompilerOutputter::defaultOutputter(&runner.result(), std::cerr));
    runner.addTest(gr::digitizers::qa_timing_receiver::suite());

    bool was_successful = runner.run("", false);
    return was_successful ? 0 : 1;
}
