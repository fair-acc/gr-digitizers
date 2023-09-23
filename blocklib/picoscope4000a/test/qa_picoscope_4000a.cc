#include <boost/ut.hpp>

#include <helper_blocks.h>
#include <picoscope4000a.h>

#include <scheduler.hpp>

namespace gr::picoscope4000a::test {

const boost::ut::suite Picoscope4000aTests = [] {
    using namespace boost::ut;
    using namespace fair::graph;
    using namespace gr::helpers;
    using namespace gr::picoscope;
    using namespace gr::picoscope4000a;

    "open and close"_test = [] {
        graph flow_graph;
        auto& ps = flow_graph.make_node<Picoscope4000a>();

        // this takes time, so we do it a few times only
        for (auto i = 0; i < 3; i++) {
            expect(nothrow([&ps] { ps.initialize(); }));
            expect(neq(ps.driver_version(), std::string()));
            expect(neq(ps.hardware_version(), std::string()));
            expect(nothrow([&ps] { ps.close(); })) << "doesn't throw";
        }
    };

    "streaming basics"_test = [] {
        graph flow_graph;
        auto& ps = flow_graph.make_node<Picoscope4000a>(Settings{
            .enabled_channels = { { "A",
                                    { .range = 5., .coupling = coupling_t::AC_1M } } },
            .sample_rate = 10000.,
            .driver_buffer_size = 50000,
            .acquisition_mode = acquisition_mode_t::STREAMING,
            .streaming_mode_poll_rate = 0.00001,
            .auto_arm = true });

        auto& sink = flow_graph.make_node<count_sink<float>>();
        auto& errsink = flow_graph.make_node<count_sink<float>>();

        expect(eq(connection_result_t::SUCCESS,
                  flow_graph.connect<"values0">(ps).template to<"in">(sink)));
        expect(eq(connection_result_t::SUCCESS,
                  flow_graph.connect<"errors0">(ps).template to<"in">(errsink)));

        // Explicitly open unit because it takes quite some time
        expect(nothrow([&ps] { ps.initialize(); }));

        ps.start(); // TODO should be done by scheduler

        // TODO tried multi_threaded scheduler with start(); sleep; stop(), something goes
        // wrong there (scheduler doesn't shovel data reliably)
        scheduler::simple sched{ std::move(flow_graph) };
        auto quitter = std::async([&ps] {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            ps.force_quit();
        });

        sched.run_and_wait();

        expect(eq(sink.samples_seen, errsink.samples_seen));
        expect(ge(sink.samples_seen, std::size_t{ 5000 }));
        expect(le(sink.samples_seen, std::size_t{ 20000 }));
    };

    "rapid block basics"_test = [] {
#if 0
        graph flow_graph;
        auto& ps = flow_graph.make_node<Picoscope4000a>({
  //          .enabled_channels = {{"A", {.range = 5., .offset = 0.f, .coupling = coupling_t::AC_1M}}},
            .sample_rate = 10000.,
            .pre_samples = 33,
            .post_samples = 1000,
            .acquisition_mode = digitizer_acquisition_mode_t::RapidBlock,
            .rapid_block_nr_captures = 1,
            .auto_arm = true,
            .trigger_once = true });

        auto &sink = flow_graph.make_node<vector_sink<float>>();
        auto &sink1 = flow_graph.make_node<null_sink<float>>();
        auto &sink2 = flow_graph.make_node<null_sink<float>>();

        expect(eq(connection_result_t::SUCCESS,
                  flow_graph.connect<"out0">(ps).template to<"in">(sink)));
        expect(eq(connection_result_t::SUCCESS,
                  flow_graph.connect<"out1">(ps).template to<"in">(sink1)));
        expect(eq(connection_result_t::SUCCESS,
                  flow_graph.connect<"out2">(ps).template to<"in">(sink2)));

        scheduler::simple sched{ std::move(flow_graph) };
        sched.run_and_wait();

        expect(eq(sink.data.size(), std::size_t{1033}));

#ifdef PORT_DISABLED // TODO(PORT) these are currently not settable after creation, is
                        // this needed?
        // run once more, note configuration is applied at start
        sink->reset();
        ps->set_samples(200, 800);
        top->run();

        data = sink->data();
        CPPUNIT_ASSERT_EQUAL(size_t{ 1000 }, data.size());
#endif

#if 0
        // check actual sample rate
        auto actual_samp_rate = ps.actual_sample_rate();
        CPPUNIT_ASSERT_DOUBLES_EQUAL(10000.0, actual_samp_rate, 0.0001);
#endif
#endif
    };
};

} // namespace gr::picoscope4000a::test

int main()
{ /* tests are statically executed */
}

#if 0

#include "qa_picoscope_4000a.h"

#include "utils.h"
#include <digitizers/tags.h>
#include <picoscope4000a/picoscope4000a.h>

#include <gnuradio/attributes.h>
#include <gnuradio/blocks/null_sink.h>
#include <gnuradio/blocks/vector_sink.h>
#include <gnuradio/flowgraph.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

#include <chrono>
#include <cstdlib>
#include <thread>

using namespace std::chrono;

using gr::digitizers::coupling_t;
using gr::digitizers::trigger_direction_t;

namespace gr::picoscope4000a {

void connect_remaining_outputs_to_null_sinks(flowgraph_sptr fg,
                                             block_sptr ps,
                                             std::size_t first)
{
    for (auto i = first; i <= 15u; ++i) {
        auto ns = blocks::null_sink::make({ .itemsize = sizeof(float) });
        fg->connect(ps, i, ns, 0);
    }
}

void qa_picoscope_4000a::open_close()
{
    auto ps = picoscope4000a::make({});

    // this takes time, so we do it a few times only
    for (auto i = 0; i < 3; i++) {
        CPPUNIT_ASSERT_NO_THROW(ps->initialize(););

        auto driver_version = ps->driver_version();
        CPPUNIT_ASSERT(!driver_version.empty());

        auto hw_version = ps->hardware_version();
        CPPUNIT_ASSERT(!hw_version.empty());

        CPPUNIT_ASSERT_NO_THROW(ps->close(););
    }
}

void qa_picoscope_4000a::rapid_block_basics()
{
    auto top = gr::flowgraph::make("basics");

    auto ps = picoscope4000a::make(
        { .sample_rate = 10000.,
          .pre_samples = 33,
          .post_samples = 1000,
          .acquisition_mode = digitizer_acquisition_mode_t::RAPID_BLOCK,
          .rapid_block_nr_captures = 1,
          .auto_arm = true,
          .trigger_once = true });

    ps->set_aichan("A",
                   true,
                   5.0,
                   coupling_t::AC_1M,
                   0); // TODO(PORT) remove last arg (double_range) when default values
                       // work in the code generation;

    auto sink = blocks::vector_sink_f::make({ 1 });
    auto errsink = blocks::null_sink::make({ .itemsize = sizeof(float) });

    // connect and run
    top->connect(ps, 0, sink, 0);
    top->connect(ps, 1, errsink, 0);
    connect_remaining_outputs_to_null_sinks(top, ps, 2);
    top->run();

    auto data = sink->data();
    CPPUNIT_ASSERT_EQUAL(size_t{ 1033 }, data.size());

#ifdef PORT_DISABLED // TODO(PORT) these are currently not settable after creation, is
                     // this needed?
    // run once more, note configuration is applied at start
    sink->reset();
    ps->set_samples(200, 800);
    top->run();

    data = sink->data();
    CPPUNIT_ASSERT_EQUAL(size_t{ 1000 }, data.size());
#endif

    // check actual sample rate
    auto actual_samp_rate = ps->actual_sample_rate();
    CPPUNIT_ASSERT_DOUBLES_EQUAL(10000.0, actual_samp_rate, 0.0001);
}

void qa_picoscope_4000a::rapid_block_channels()
{
    auto top = gr::flowgraph::make("channels");

    auto ps = picoscope4000a::make(
        { .sample_rate = 10000.,
          .pre_samples = 50,
          .post_samples = 1000,
          .acquisition_mode = digitizer_acquisition_mode_t::RAPID_BLOCK,
          .rapid_block_nr_captures = 1,
          .auto_arm = true,
          .trigger_once = true });

    ps->set_aichan("A",
                   true,
                   5.0,
                   coupling_t::AC_1M,
                   0); // TODO(PORT) remove last arg (double_range) when default values
                       // work in the code generation;
    ps->set_aichan("B",
                   true,
                   5.0,
                   coupling_t::AC_1M,
                   0); // TODO(PORT) remove last arg (double_range) when default values
                       // work in the code generation;
    ps->set_aichan("C",
                   true,
                   5.0,
                   coupling_t::AC_1M,
                   0); // TODO(PORT) remove last arg (double_range) when default values
                       // work in the code generation;
    ps->set_aichan("D",
                   true,
                   5.0,
                   coupling_t::AC_1M,
                   0); // TODO(PORT) remove last arg (double_range) when default values
                       // work in the code generation;

    auto sinkA = blocks::vector_sink_f::make({ 1 });
    auto sinkB = blocks::vector_sink_f::make({ 1 });
    auto sinkC = blocks::vector_sink_f::make({ 1 });
    auto sinkD = blocks::vector_sink_f::make({ 1 });
    auto errsinkA = blocks::null_sink::make({ .itemsize = sizeof(float) });
    auto errsinkB = blocks::null_sink::make({ .itemsize = sizeof(float) });
    auto errsinkC = blocks::null_sink::make({ .itemsize = sizeof(float) });
    auto errsinkD = blocks::null_sink::make({ .itemsize = sizeof(float) });

    // connect and run
    top->connect(ps, 0, sinkA, 0);
    top->connect(ps, 1, errsinkA, 0);
    top->connect(ps, 2, sinkB, 0);
    top->connect(ps, 3, errsinkB, 0);
    top->connect(ps, 4, sinkC, 0);
    top->connect(ps, 5, errsinkC, 0);
    top->connect(ps, 6, sinkD, 0);
    top->connect(ps, 7, errsinkD, 0);
    connect_remaining_outputs_to_null_sinks(top, ps, 8);
    top->run();

    CPPUNIT_ASSERT_EQUAL(1050, (int)sinkA->data().size());
    CPPUNIT_ASSERT_EQUAL(1050, (int)sinkB->data().size());
    CPPUNIT_ASSERT_EQUAL(1050, (int)sinkC->data().size());
    CPPUNIT_ASSERT_EQUAL(1050, (int)sinkD->data().size());
}

void qa_picoscope_4000a::rapid_block_continuous()
{
    auto top = gr::flowgraph::make("continuous");

    auto ps = picoscope4000a::make(
        { .sample_rate = 10000,
          .buffer_size = 1000,
          .post_samples = 1000,
          .acquisition_mode =
              digitizer_acquisition_mode_t::RAPID_BLOCK, // TODO(PORT) buffer_size was set
                                                         // explicitly before when setting
                                                         // pre/post samples
          .rapid_block_nr_captures = 1 });

    ps->set_aichan("A",
                   true,
                   5.0,
                   coupling_t::AC_1M,
                   0); // TODO(PORT) remove last arg (double_range) when default values
                       // work in the code generation;

    auto sink = blocks::vector_sink_f::make({ 1 });
    auto errsink = blocks::null_sink::make({ .itemsize = sizeof(float) });

    // connect and run
    top->connect(ps, 0, sink, 0);
    top->connect(ps, 1, errsink, 0);
    connect_remaining_outputs_to_null_sinks(top, ps, 2);

    // We explicitly open unit because it takes quite some time
    // and we don't want to time this part
    CPPUNIT_ASSERT_NO_THROW(ps->initialize());

    top->start();
    sleep(1);
    top->stop();
    top->wait();

    // We should be able to acquire around 10k samples now but since
    // sleep can be imprecise and we want stable tests we just check if
    // we've got a good number of samples
    auto samples = sink->data().size();
    CPPUNIT_ASSERT_MESSAGE("actual samples: " + std::to_string(samples),
                           samples > 3000 && samples < 15000);
}

void qa_picoscope_4000a::rapid_block_downsampling()
{
    run_rapid_block_downsampling(digitizer_downsampling_mode_t::AVERAGE);
    run_rapid_block_downsampling(digitizer_downsampling_mode_t::MIN_MAX_AGG);
    run_rapid_block_downsampling(digitizer_downsampling_mode_t::DECIMATE);
}

void qa_picoscope_4000a::rapid_block_tags()
{
    using digitizers::acq_info_tag_name;
    using digitizers::decode_timebase_info_tag;
    using digitizers::timebase_info_tag_name;

    auto top = gr::flowgraph::make("tags");

    auto samp_rate = 10000.0;

    auto ps = picoscope4000a::make(
        { .sample_rate = samp_rate,
          .pre_samples = 200,
          .post_samples = 1000,
          .acquisition_mode = digitizer_acquisition_mode_t::RAPID_BLOCK,
          .rapid_block_nr_captures = 1,
          .auto_arm = true,
          .trigger_once = true });

    ps->set_aichan("A",
                   true,
                   5.0,
                   coupling_t::AC_1M,
                   0); // TODO(PORT) remove last arg (double_range) when default values
                       // work in the code generation;

    auto sink = blocks::vector_sink_f::make({ 1 });
    auto errsink = blocks::vector_sink_f::make({ 1 });

    // connect and run
    top->connect(ps, 0, sink, 0);
    top->connect(ps, 1, errsink, 0);
    connect_remaining_outputs_to_null_sinks(top, ps, 2);
    top->run();

    auto data_tags = sink->tags();
    CPPUNIT_ASSERT_EQUAL(1, (int)data_tags.size());

    for (auto& tag : data_tags) {
        CPPUNIT_ASSERT_EQUAL(false, tag.map().empty());
        const auto key = tag.map().begin()->first;
        CPPUNIT_ASSERT(key == acq_info_tag_name || key == timebase_info_tag_name);

        if (key == timebase_info_tag_name) {
            auto timebase = decode_timebase_info_tag(tag);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0 / samp_rate, timebase, 0.0000001);
        }
        else {
            CPPUNIT_ASSERT_EQUAL(static_cast<uint64_t>(200), tag.offset());
        }
    }
}

} // namespace gr::picoscope4000a

int main(int, char**)
{
    const auto var = std::getenv("PICOSCOPE_RUN_TESTS");

    if (!var || std::string_view(var).find("4000a") == std::string_view::npos) {
        std::cout
            << "'4000a' not in PICOSCOPE_RUN_TESTS environment variable, do not run test"
            << std::endl;
        return 0;
    }

    CppUnit::TextTestRunner runner;
    runner.setOutputter(
        CppUnit::CompilerOutputter::defaultOutputter(&runner.result(), std::cerr));
    runner.addTest(gr::picoscope4000a::qa_picoscope_4000a::suite());

    bool was_successful = runner.run("", false);
    return was_successful ? 0 : 1;
}

#endif