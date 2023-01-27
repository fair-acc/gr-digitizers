
#include "qa_picoscope_3000a.h"
#include "utils.h"

#include <digitizers/tags.h>

#include <gnuradio/attributes.h>
#include <gnuradio/blocks/null_sink.h>
#include <gnuradio/blocks/vector_sink.h>
#include <gnuradio/flowgraph.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

#include <string_view>

using gr::digitizers::coupling_t;
using gr::digitizers::trigger_direction_t;

namespace gr::picoscope3000a {

void connect_remaining_outputs_to_null_sinks(flowgraph_sptr fg,
                                             block_sptr ps,
                                             std::size_t first_analog,
                                             std::size_t first_digital = 0)
{
    for (auto i = first_analog; i <= 7u; ++i) {
        auto ns = blocks::null_sink::make({ .itemsize = sizeof(float) });
        fg->connect(ps, i, ns, 0);
    }

    for (auto i = first_digital; i <= 1u; ++i) {
        auto ns = blocks::null_sink::make({ .itemsize = sizeof(uint8_t) });
        fg->connect(ps, 8 + i, ns, 0);
    }
}

void qa_picoscope_3000a::open_close()
{
    auto ps = picoscope3000a::make({});

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

void qa_picoscope_3000a::rapid_block_basics()
{
    auto top = gr::flowgraph::make("basics");

    auto ps = picoscope3000a::make(
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

#ifdef PORT_DISABLED // TODO(PORT) sink->reset() does not exist in GR4
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

void qa_picoscope_3000a::rapid_block_channels()
{
    auto top = gr::flowgraph::make("channels");

    auto ps = picoscope3000a::make(
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
    ps->set_diport("port0", true, 3.0);
    ps->set_diport("port1", true, 3.0);

    auto sinkA = blocks::vector_sink_f::make({ 1 });
    auto sinkB = blocks::vector_sink_f::make({ 1 });
    auto sinkC = blocks::vector_sink_f::make({ 1 });
    auto sinkD = blocks::vector_sink_f::make({ 1 });
    auto errsinkA = blocks::null_sink::make({ .itemsize = sizeof(float) });
    auto errsinkB = blocks::null_sink::make({ .itemsize = sizeof(float) });
    auto errsinkC = blocks::null_sink::make({ .itemsize = sizeof(float) });
    auto errsinkD = blocks::null_sink::make({ .itemsize = sizeof(float) });

    auto sink0 = blocks::vector_sink_b::make({ 1 });
    auto sink1 = blocks::vector_sink_b::make({ 1 });

    // connect and run
    top->connect(ps, 0, sinkA, 0);
    top->connect(ps, 1, errsinkA, 0);
    top->connect(ps, 2, sinkB, 0);
    top->connect(ps, 3, errsinkB, 0);
    top->connect(ps, 4, sinkC, 0);
    top->connect(ps, 5, errsinkC, 0);
    top->connect(ps, 6, sinkD, 0);
    top->connect(ps, 7, errsinkD, 0);

    top->connect(ps, 8, sink0, 0);
    top->connect(ps, 9, sink1, 0);

    top->run();

    CPPUNIT_ASSERT_EQUAL(1050, (int)sinkA->data().size());
    CPPUNIT_ASSERT_EQUAL(1050, (int)sinkB->data().size());
    CPPUNIT_ASSERT_EQUAL(1050, (int)sinkC->data().size());
    CPPUNIT_ASSERT_EQUAL(1050, (int)sinkD->data().size());
    CPPUNIT_ASSERT_EQUAL(1050, (int)sink0->data().size());
    CPPUNIT_ASSERT_EQUAL(1050, (int)sink1->data().size());
}

void qa_picoscope_3000a::rapid_block_continuous()
{
    auto top = gr::flowgraph::make("continuous");

    auto ps = picoscope3000a::make(
        { .sample_rate = 10000.0,
          .pre_samples = 0,
          .post_samples = 1000,
          .acquisition_mode = digitizer_acquisition_mode_t::RAPID_BLOCK,
          .rapid_block_nr_captures = 1,
          .auto_arm = true });

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

void qa_picoscope_3000a::rapid_block_downsampling_basics()
{
    auto top = gr::flowgraph::make("downsampling");

    auto ps = picoscope3000a::make(
        { .sample_rate = 10000.,
          .pre_samples = 200,
          .post_samples = 1000,
          .acquisition_mode = digitizer_acquisition_mode_t::RAPID_BLOCK,
          .rapid_block_nr_captures = 1,
          .downsampling_mode = digitizer_downsampling_mode_t::DECIMATE,
          .downsampling_factor = 4,
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
    CPPUNIT_ASSERT_EQUAL(300, (int)data.size());
}

void qa_picoscope_3000a::run_rapid_block_downsampling(digitizer_downsampling_mode_t mode)
{
    auto top = gr::flowgraph::make("channels");

    auto ps = picoscope3000a::make(
        { .sample_rate = 10000.,
          .pre_samples = 1000,
          .post_samples = 10000,
          .acquisition_mode = digitizer_acquisition_mode_t::RAPID_BLOCK,
          .rapid_block_nr_captures = 1,
          .downsampling_mode = mode,
          .downsampling_factor = 10,
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
    ps->set_diport("port0", true, 3.0);
    ps->set_diport("port1", true, 3.0);

    auto sinkA = blocks::vector_sink_f::make({ 1 });
    auto sinkB = blocks::vector_sink_f::make({ 1 });
    auto sinkC = blocks::vector_sink_f::make({ 1 });
    auto sinkD = blocks::vector_sink_f::make({ 1 });
    auto errsinkA = blocks::vector_sink_f::make({ 1 });
    auto errsinkB = blocks::vector_sink_f::make({ 1 });
    auto errsinkC = blocks::vector_sink_f::make({ 1 });
    auto errsinkD = blocks::vector_sink_f::make({ 1 });

    auto sink0 = blocks::vector_sink_b::make({ 1 });
    auto sink1 = blocks::vector_sink_b::make({ 1 });

    // connect and run
    top->connect(ps, 0, sinkA, 0);
    top->connect(ps, 1, errsinkA, 0);
    top->connect(ps, 2, sinkB, 0);
    top->connect(ps, 3, errsinkB, 0);
    top->connect(ps, 4, sinkC, 0);
    top->connect(ps, 5, errsinkC, 0);
    top->connect(ps, 6, sinkD, 0);
    top->connect(ps, 7, errsinkD, 0);

    top->connect(ps, 8, sink0, 0);
    top->connect(ps, 9, sink1, 0);

    top->run();

    CPPUNIT_ASSERT_EQUAL(1100, (int)sinkA->data().size());
    CPPUNIT_ASSERT_EQUAL(1100, (int)sinkB->data().size());
    CPPUNIT_ASSERT_EQUAL(1100, (int)sinkC->data().size());
    CPPUNIT_ASSERT_EQUAL(1100, (int)sinkD->data().size());
    CPPUNIT_ASSERT_EQUAL(1100, (int)sink0->data().size());
    CPPUNIT_ASSERT_EQUAL(1100, (int)sink1->data().size());

    CPPUNIT_ASSERT_EQUAL(1100, (int)errsinkA->data().size());
    CPPUNIT_ASSERT_EQUAL(1100, (int)errsinkB->data().size());
    CPPUNIT_ASSERT_EQUAL(1100, (int)errsinkC->data().size());
    CPPUNIT_ASSERT_EQUAL(1100, (int)errsinkD->data().size());
}

void qa_picoscope_3000a::rapid_block_downsampling()
{
    run_rapid_block_downsampling(digitizer_downsampling_mode_t::AVERAGE);
    run_rapid_block_downsampling(digitizer_downsampling_mode_t::MIN_MAX_AGG);
    run_rapid_block_downsampling(digitizer_downsampling_mode_t::DECIMATE);
}

void qa_picoscope_3000a::rapid_block_tags()
{
    auto top = gr::flowgraph::make("tags");

    auto samp_rate = 10000.0;

    auto ps = picoscope3000a::make(
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

        CPPUNIT_ASSERT(key == digitizers::acq_info_tag_name ||
                       key == digitizers::timebase_info_tag_name);

        if (key == digitizers::timebase_info_tag_name) {
            double timebase = digitizers::decode_timebase_info_tag(tag);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0 / samp_rate, timebase, 0.0000001);
        }
        else {
            CPPUNIT_ASSERT_EQUAL(static_cast<uint64_t>(200), tag.offset());
        }
    }
}

void qa_picoscope_3000a::rapid_block_trigger()
{
    auto top = gr::flowgraph::make("tags");

    auto samp_rate = 10000.0;

    auto ps = picoscope3000a::make(
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
    ps->set_diport("port0", true, 1.5);
    ps->set_aichan_trigger("A", trigger_direction_t::RISING, 1.5);

    auto sink = blocks::vector_sink_f::make({ 1 });
    auto errsink = blocks::vector_sink_f::make({ 1 });

    auto bsink = blocks::null_sink::make({ .itemsize = sizeof(float) });
    auto csink = blocks::null_sink::make({ .itemsize = sizeof(float) });
    auto dsink = blocks::null_sink::make({ .itemsize = sizeof(float) });
    auto berrsink = blocks::null_sink::make({ .itemsize = sizeof(float) });
    auto cerrsink = blocks::null_sink::make({ .itemsize = sizeof(float) });
    auto derrsink = blocks::null_sink::make({ .itemsize = sizeof(float) });

    auto port0 = blocks::vector_sink_b::make({ 1 });

    // connect and run
    top->connect(ps, 0, sink, 0);
    top->connect(ps, 1, errsink, 0);
    top->connect(ps, 2, bsink, 0);
    top->connect(ps, 3, berrsink, 0);
    top->connect(ps, 4, csink, 0);
    top->connect(ps, 5, cerrsink, 0);
    top->connect(ps, 6, dsink, 0);
    top->connect(ps, 7, derrsink, 0);
    top->connect(ps, 8, port0, 0);
    connect_remaining_outputs_to_null_sinks(top, ps, 8 /*none*/, 1);
    top->run();

    auto data = sink->data();
    CPPUNIT_ASSERT_EQUAL(1200, (int)data.size());
}

void qa_picoscope_3000a::streaming_basics()
{
    auto top = gr::flowgraph::make("streaming_basic");

    auto ps = picoscope3000a::make(
        { .sample_rate = 1000.,
          .buffer_size = 100000,
          .acquisition_mode = digitizer_acquisition_mode_t::STREAMING,
          .streaming_mode_poll_rate = 0.00001,
          .auto_arm = true });

    auto sink = blocks::vector_sink_f::make({ 1 });
    auto errsink = blocks::null_sink::make({ .itemsize = sizeof(float) });

    // connect and run
    top->connect(ps, 0, sink, 0);
    top->connect(ps, 1, errsink, 0);
    connect_remaining_outputs_to_null_sinks(top, ps, 2);

    // Explicitly open unit because it takes quite some time
    CPPUNIT_ASSERT_NO_THROW(ps->initialize());

    top->start();
    sleep(2);
    top->stop();
    top->wait();

    auto data = sink->data();
    CPPUNIT_ASSERT(data.size() <= 20000 && data.size() >= 5000);

#ifdef PORT_DISABLED // TODO(PORT) sink->reset() does not exist in GR4
    // ps->initialize();

    sink->reset();
    top->start();
    sleep(2);
    top->stop();
    top->wait();

    data = sink->data();
    CPPUNIT_ASSERT(data.size() <= 20000 && data.size() >= 5000);
#endif
}

} // namespace gr::picoscope3000a

int main(int, char**)
{
    const auto var = std::getenv("PICOSCOPE_RUN_TESTS");

    if (!var || std::string_view(var).find("3000a") == std::string_view::npos) {
        std::cout
            << "'3000a' not in PICOSCOPE_RUN_TESTS environment variable, do not run test"
            << std::endl;
        return 0;
    }

    CppUnit::TextTestRunner runner;
    runner.setOutputter(
        CppUnit::CompilerOutputter::defaultOutputter(&runner.result(), std::cerr));
    runner.addTest(gr::picoscope3000a::qa_picoscope_3000a::suite());

    bool was_successful = runner.run("", false);
    return was_successful ? 0 : 1;
}