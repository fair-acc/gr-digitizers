#include "qa_timing_receiver.h"

#include <digitizers/enums.h>
#include <digitizers/timing_receiver_simulated.h>

#include <gnuradio/blocks/message_debug.h>
#include <gnuradio/flowgraph.h>
#include <gnuradio/tag.h>

#define ZMQ_BUILD_DRAFT_API
#include <zmq.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

#include <chrono>

namespace gr::digitizers {

void qa_timing_receiver::periodic_interval()
{
    using namespace std::chrono_literals;

    const std::string trigger_name = "TRTEST";
    constexpr int64_t interval = 250; /*ms*/
    constexpr double trigger_offset = 20.;

    auto timing_rec = timing_receiver_simulated::make(
        { .simulation_mode = timing_receiver_simulation_mode_t::PERIODIC_INTERVAL,
          .trigger_name = trigger_name,
          .trigger_offset = trigger_offset,
          .tag_time_interval = interval });
    auto sink = blocks::message_debug::make({});

    auto fg = gr::flowgraph::make("periodic_interval");
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

void qa_timing_receiver::basic_zmq()
{
    using namespace std::chrono_literals;

    const std::string trigger_name = "ZMQ";
    constexpr double trigger_offset = 30.;
    const std::string zmq_endpoint = "udp://127.0.0.1:55555";
    const std::string zmq_group = "test";

    auto timing_rec = timing_receiver_simulated::make(
        { .simulation_mode = timing_receiver_simulation_mode_t::ZEROMQ,
          .trigger_name = trigger_name,
          .trigger_offset = trigger_offset,
          .zmq_endpoint = zmq_endpoint,
          .zmq_group = zmq_group });
    auto sink = blocks::message_debug::make({});

    auto fg = gr::flowgraph::make("basic_zmq");
    fg->connect(timing_rec, "out", sink, "store");
    fg->start();

    // give timing_receiver time to bind the dish socket
    std::this_thread::sleep_for(100ms);

    auto context = zmq_ctx_new();
    auto socket = zmq_socket(context, ZMQ_RADIO);

    CPPUNIT_ASSERT_EQUAL(0, zmq_connect(socket, zmq_endpoint.data()));

    using namespace std::string_literals;

    const std::vector<std::pair<int64_t, std::string>> timestamps = {
        { 239, "\x00\x00\x00\x00\x00\x00\x00\xef"s },
        { 62191, "\x00\x00\x00\x00\x00\x00\xf2\xef"s },
        { 1152921504621196015, "\x10\x00\x00\x00\x00\xda\xf2\xef"s }
    };

    for (auto& [_, timestamp] : timestamps) {
        CPPUNIT_ASSERT_EQUAL(sizeof(int64_t), timestamp.size());
        zmq_msg_t msg;
        CPPUNIT_ASSERT_EQUAL(0, zmq_msg_init_size(&msg, timestamp.size()));
        memcpy(zmq_msg_data(&msg), timestamp.data(), timestamp.size());
        CPPUNIT_ASSERT_EQUAL(0, zmq_msg_set_group(&msg, zmq_group.data()));
        CPPUNIT_ASSERT_EQUAL(static_cast<int>(timestamp.size()),
                             zmq_sendmsg(socket, &msg, 0));
    }

    std::this_thread::sleep_for(
        300ms); // give timing_receiver and GR time to process everything

    fg->stop();

    CPPUNIT_ASSERT_EQUAL(timestamps.size(), sink->num_messages());

    for (std::size_t i = 0; i < timestamps.size(); ++i) {
        const auto message =
            pmtf::get_as<std::map<std::string, pmtf::pmt>>(sink->get_message(i));
        CPPUNIT_ASSERT_EQUAL(
            trigger_name, pmtf::get_as<std::string>(message.at(gr::tag::TRIGGER_NAME)));
        CPPUNIT_ASSERT_EQUAL(trigger_offset,
                             pmtf::get_as<double>(message.at(gr::tag::TRIGGER_OFFSET)));
        CPPUNIT_ASSERT_EQUAL(timestamps[i].first,
                             pmtf::get_as<int64_t>(message.at(gr::tag::TRIGGER_TIME)));
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
