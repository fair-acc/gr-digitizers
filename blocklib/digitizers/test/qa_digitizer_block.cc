#include "qa_digitizer_block.h"
#include "qa_common.h"

#include <digitizers/simulation_source.h>
#include <digitizers/tags.h>

#include <gnuradio/attributes.h>
#include <gnuradio/blocks/vector_sink.h>
#include <gnuradio/flowgraph.h>
#include <gnuradio/streamops/throttle.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

#include <chrono>
#include <thread>

namespace gr::digitizers {

struct simulated_test_flowgraph_t {
    gr::flowgraph::sptr             top;
    simulation_source::sptr         source;
    gr::blocks::vector_sink_f::sptr sink_sig_a;
    gr::blocks::vector_sink_f::sptr sink_err_a;
    gr::blocks::vector_sink_f::sptr sink_sig_b;
    gr::blocks::vector_sink_f::sptr sink_err_b;
    gr::blocks::vector_sink_b::sptr sink_port;
    gr::streamops::throttle::sptr   throttle;
};

static simulation_source::block_args default_args() {
    return {
        .sample_rate = 10000,
        .auto_arm    = true,
    };
}

static simulated_test_flowgraph_t make_test_flowgraph(simulation_source::block_args &args) {
    simulated_test_flowgraph_t fg;

    fg.top        = gr::flowgraph::make("test");
    fg.source     = gr::digitizers::simulation_source::make(args);

    fg.sink_sig_a = blocks::vector_sink_f::make({ 1 });
    fg.sink_err_a = blocks::vector_sink_f::make({ 1 });
    fg.sink_sig_b = blocks::vector_sink_f::make({ 1 });
    fg.sink_err_b = blocks::vector_sink_f::make({ 1 });
    fg.sink_port  = blocks::vector_sink_b::make({ 1 });

    fg.throttle   = streamops::throttle::make({ .samples_per_sec = args.sample_rate,
              .itemsize                                          = sizeof(float) });

    fg.top->connect(fg.source, 0, fg.throttle, 0);
    fg.top->connect(fg.throttle, 0, fg.sink_sig_a, 0);
    fg.top->connect(fg.source, 1, fg.sink_err_a, 0);
    fg.top->connect(fg.source, 2, fg.sink_sig_b, 0);
    fg.top->connect(fg.source, 3, fg.sink_err_b, 0);
    fg.top->connect(fg.source, 4, fg.sink_port, 0);

    return fg;
}

void qa_digitizer_block::fill_data(unsigned samples, unsigned presamples) {
    float add = 0;
    for (unsigned i = 0; i < samples + presamples; i++) {
        if (i == presamples) {
            add = 2.0;
        }
        d_cha_vec.push_back(add + 2.0 * i / 1033.0);
        d_chb_vec.push_back(2.0 * i / 1033.0);
        d_port_vec.push_back(i % 256);
    }
}

void qa_digitizer_block::rapid_block_basics() {
    int samples    = 1000;
    int presamples = 50;
    fill_data(samples, presamples);

    auto args                    = default_args();
    args.trigger_once            = true;
    args.rapid_block_nr_captures = 1;
    args.acquisition_mode        = digitizer_acquisition_mode_t::RAPID_BLOCK;
    args.pre_samples             = presamples;
    args.post_samples            = samples;

    auto fg                      = make_test_flowgraph(args);
    auto source                  = fg.source;

    fg.source->set_data(d_cha_vec, d_chb_vec, d_port_vec);

    fg.top->run();

    auto dataa = fg.sink_sig_a->data();
    CPPUNIT_ASSERT_EQUAL(samples + presamples, (int) dataa.size());
    ASSERT_VECTOR_EQUAL(d_cha_vec.begin(), d_cha_vec.end(), dataa.begin());

    auto datab = fg.sink_sig_b->data();
    CPPUNIT_ASSERT_EQUAL(samples + presamples, (int) datab.size());
    ASSERT_VECTOR_EQUAL(d_chb_vec.begin(), d_chb_vec.end(), datab.begin());

    auto datap = fg.sink_port->data();
    CPPUNIT_ASSERT_EQUAL(samples + presamples, (int) datap.size());
    ASSERT_VECTOR_EQUAL(d_port_vec.begin(), d_port_vec.end(), reinterpret_cast<uint8_t *>(&datap[0]));
}

void qa_digitizer_block::rapid_block_correct_tags() {
    int samples    = 2000;
    int presamples = 200;
    fill_data(samples, presamples);

    auto args                    = default_args();
    args.pre_samples             = presamples;
    args.post_samples            = samples;
    args.trigger_once            = true;
    args.rapid_block_nr_captures = 1;
    args.acquisition_mode        = digitizer_acquisition_mode_t::RAPID_BLOCK;

    auto fg                      = make_test_flowgraph(args);
    auto source                  = fg.source;

    fg.source->set_data(d_cha_vec, d_chb_vec, d_port_vec);

    fg.top->run();

    auto data_tags = fg.sink_sig_a->tags();

    for (auto &tag : data_tags) {
        CPPUNIT_ASSERT_EQUAL(tag.map().size(), std::size_t{ 1 });
        const auto key = tag.map().begin()->first;

        CPPUNIT_ASSERT(key == acq_info_tag_name
                       || key == timebase_info_tag_name
                       || key == trigger_tag_name);

        if (key == trigger_tag_name) {
            auto triggered_data = decode_trigger_tag(tag);
            CPPUNIT_ASSERT_EQUAL(uint32_t{ 0 }, triggered_data.status);
        } else if (key == timebase_info_tag_name) {
            auto timebase = decode_timebase_info_tag(tag);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0 / 10000.0, timebase, 0.0000001);
        } else if (key == acq_info_tag_name) {
            CPPUNIT_ASSERT_EQUAL(static_cast<uint64_t>(presamples), tag.offset());
        } else {
            CPPUNIT_FAIL("unknown tag. key: " + key);
        }
    }
}

void qa_digitizer_block::streaming_basics() {
    int samples     = 2000;
    int presamples  = 200;
    int buffer_size = samples + presamples;

    fill_data(samples, presamples);

    auto args                     = default_args();
    args.acquisition_mode         = digitizer_acquisition_mode_t::STREAMING;
    args.streaming_mode_poll_rate = 0.0001;
    args.buffer_size              = buffer_size;

    auto fg                       = make_test_flowgraph(args);

    fg.source->set_data(d_cha_vec, d_chb_vec, d_port_vec);

    fg.top->start();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fg.top->stop();
    fg.top->wait();

    auto dataa = fg.sink_sig_a->data();
    CPPUNIT_ASSERT(dataa.size() != 0);

    auto size = std::min(dataa.size(), d_cha_vec.size());
    ASSERT_VECTOR_EQUAL(d_cha_vec.begin(), d_cha_vec.begin() + size, dataa.begin());

    auto datab = fg.sink_sig_b->data();
    CPPUNIT_ASSERT(datab.size() != 0);
    size = std::min(datab.size(), d_chb_vec.size());
    ASSERT_VECTOR_EQUAL(d_chb_vec.begin(), d_chb_vec.begin() + size, datab.begin());

    auto datap = fg.sink_port->data();
    CPPUNIT_ASSERT(datap.size() != 0);
    size = std::min(datap.size(), d_port_vec.size());
    ASSERT_VECTOR_EQUAL(d_port_vec.begin(), d_port_vec.begin() + size, datap.begin());
}

void qa_digitizer_block::streaming_correct_tags() {
    int samples     = 2000;
    int presamples  = 200;
    int buffer_size = samples + presamples;

    fill_data(samples, presamples);

    auto args                     = default_args();
    args.buffer_size              = buffer_size;
    args.acquisition_mode         = digitizer_acquisition_mode_t::STREAMING;
    args.streaming_mode_poll_rate = 0.0001;

    auto fg                       = make_test_flowgraph(args);
    auto source                   = fg.source;

    fg.source->set_data(d_cha_vec, d_chb_vec, d_port_vec);

    fg.top->start();
    std::this_thread::sleep_for(std::chrono::microseconds(500));
    fg.top->stop();
    fg.top->wait();

    auto data_tags = fg.sink_sig_a->tags();

    for (auto &tag : data_tags) {
        CPPUNIT_ASSERT_EQUAL(tag.map().size(), std::size_t{ 1 });
        const auto key = tag.map().begin()->first;
        CPPUNIT_ASSERT(key == timebase_info_tag_name || key == acq_info_tag_name);

        if (key == acq_info_tag_name) {
            CPPUNIT_ASSERT_EQUAL(0, static_cast<int>(tag.offset()) % buffer_size);
        } else {
            double timebase = decode_timebase_info_tag(tag);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0 / 10000.0, timebase, 0.0000001);
        }
    }
}

} // namespace gr::digitizers

int main(int, char **) {
    CppUnit::TextTestRunner runner;
    runner.setOutputter(CppUnit::CompilerOutputter::defaultOutputter(
            &runner.result(),
            std::cerr));
    runner.addTest(gr::digitizers::qa_digitizer_block::suite());

    bool was_successful = runner.run("", false);
    return was_successful ? 0 : 1;
}
