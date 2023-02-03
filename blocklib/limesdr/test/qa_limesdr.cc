
#include "qa_limesdr.h"
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

namespace gr::limesdr {

static void connect_remaining_outputs_to_null_sinks(flowgraph_sptr fg,
                                                    block_sptr ls,
                                                    std::size_t first_analog,
                                                    std::size_t first_digital = 0)
{
    for (auto i = first_analog; i <= 1u; ++i) {
        auto ns = blocks::null_sink::make({ .itemsize = sizeof(float) });
        fg->connect(ls, i, ns, 0);
    }

    for (auto i = first_digital; i <= 1u; ++i) {
        auto ns = blocks::null_sink::make({ .itemsize = sizeof(uint8_t) });
        fg->connect(ls, 2 + i, ns, 0);
    }
}

void qa_limesdr::open_close()
{
    auto ps = limesdr::make({});

    // this takes time, so we do it a few times only
    for (auto i = 0; i < 3; i++) {
        CPPUNIT_ASSERT_NO_THROW(ps->initialize(););

        const auto driver_version = ps->driver_version();
        CPPUNIT_ASSERT(!driver_version.empty());

        const auto hw_version = ps->hardware_version();
        CPPUNIT_ASSERT(!hw_version.empty());

        CPPUNIT_ASSERT_NO_THROW(ps->close(););
    }
}

void qa_limesdr::streaming_basics()
{
    auto top = flowgraph::make("streaming_basics");

    auto ls = limesdr::make(
        { .sample_rate = 100000.,
          .buffer_size = 100000,
          .acquisition_mode = digitizer_acquisition_mode_t::STREAMING,
          .streaming_mode_poll_rate = 0.00001,
          .auto_arm = true });

    ls->set_aichan("A",
                   true,
                   5.0,
                   coupling_t::AC_1M,
                   0); // TODO(PORT) remove last arg (double_range) when default values
                       // work in the code generation;

    auto sink = blocks::vector_sink_f::make({ 1 });
    auto errsink = blocks::null_sink::make({ .itemsize = sizeof(float) });

    // connect and run
    top->connect(ls, 0, sink, 0);
    top->connect(ls, 1, errsink, 0);
    connect_remaining_outputs_to_null_sinks(top, ls, 2);

    // Explicitly open unit because it takes quite some time
    CPPUNIT_ASSERT_NO_THROW(ls->initialize());

    top->start();
    sleep(2);
    top->stop();
    top->wait();

    auto data = sink->data();
    CPPUNIT_ASSERT(data.size() <= 200000 && data.size() >= 150000);
}

} // namespace gr::limesdr

int main(int, char**)
{
    const auto var = std::getenv("PICOSCOPE_RUN_TESTS"); // TODO generalize name

    if (!var || std::string_view(var).find("lime") == std::string_view::npos) {
        std::cout
            << "'lime' not in PICOSCOPE_RUN_TESTS environment variable, do not run test"
            << std::endl;
        return 0;
    }

    CppUnit::TextTestRunner runner;
    runner.setOutputter(
        CppUnit::CompilerOutputter::defaultOutputter(&runner.result(), std::cerr));
    runner.addTest(gr::limesdr::qa_limesdr::suite());

    bool was_successful = runner.run("", false);
    return was_successful ? 0 : 1;
}
