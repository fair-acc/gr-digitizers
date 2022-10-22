/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#include "qa_wr_receiver.h"
#include "qa_common.h"

#include <digitizers/tags.h>
#include <digitizers/wr_receiver.h>

#include <gnuradio/attributes.h>
#include <gnuradio/blocks/vector_sink.h>
#include <gnuradio/blocks/vector_source.h>
#include <gnuradio/flowgraph.h>
#include <gnuradio/math/add.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

namespace gr::digitizers {

void qa_wr_receiver::test() {
    size_t             data_size = 16000;
    std::vector<float> expected_data(data_size); // zeroes

    auto               fg = gr::flowgraph::make("WR");

    // vector source is used to limit the data (flowgraph stops after data size samples is generated)
    auto vector_source = gr::blocks::vector_source_f::make({ expected_data });
    auto wr_source     = gr::digitizers::wr_receiver::make({});
    auto adder         = gr::math::add_ff::make({});
    auto vector_sink   = gr::blocks::vector_sink_f::make({});

    fg->connect(vector_source, 0, adder, 0);
    fg->connect(wr_source, 0, adder, 1);
    fg->connect(adder, 0, vector_sink, 0);

    // inject made up event
    wr_source->add_timing_event("made up event", 987654321, 123456789);

    fg->run();

    auto data = vector_sink->data();
    CPPUNIT_ASSERT_EQUAL(data_size, data.size());
    ASSERT_VECTOR_EQUAL(expected_data.begin(), expected_data.end(), data.begin());

    auto tags = vector_sink->tags();
    CPPUNIT_ASSERT_EQUAL(size_t{ 1 }, tags.size());

    auto event = decode_wr_event_tag(tags.at(0));
    CPPUNIT_ASSERT_EQUAL(std::string{ "made up event" }, event.event_id);
    CPPUNIT_ASSERT_EQUAL(int64_t{ 987654321 }, event.wr_trigger_stamp);
    CPPUNIT_ASSERT_EQUAL(int64_t{ 123456789 }, event.wr_trigger_stamp_utc); // TODO(PORT) not in baseline
#ifdef PORT_DISABLED                                                        // TODO(PORT) this wasn't compiled in the baseline, check if it's good for anything
    CPPUNIT_ASSERT_EQUAL(int64_t{ 123456789 }, event.last_beam_in_timestamp);
    CPPUNIT_ASSERT_EQUAL(true, event.time_sync_only);
    CPPUNIT_ASSERT_EQUAL(false, event.realignment_required);
    CPPUNIT_ASSERT_EQUAL(uint64_t{ 0 }, event.offset);
#endif
}

} // namespace gr::digitizers

int main(int, char **) {
    CppUnit::TextTestRunner runner;
    runner.setOutputter(CppUnit::CompilerOutputter::defaultOutputter(
            &runner.result(),
            std::cerr));
    runner.addTest(gr::digitizers::qa_wr_receiver::suite());

    bool was_successful = runner.run("", false);
    return was_successful ? 0 : 1;
}
