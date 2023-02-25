
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

#if 0
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
#endif

void qa_limesdr::streaming_basics()
{
    auto top = flowgraph::make("streaming_basics");

    auto ls = limesdr::make({
        .enabled_channels = { 0, 1 },
        .timing_trigger_mask = 0b00100000 // 0010 for channel 1
    });

    auto sink0 = blocks::vector_sink<int16_t>::make({ .vlen = 2 });
    auto errsink0 = blocks::null_sink::make({ .itemsize = 2 * sizeof(int16_t) });
    auto sink1 = blocks::vector_sink<int16_t>::make({ .vlen = 2 });
    auto errsink1 = blocks::null_sink::make({ .itemsize = 2 * sizeof(int16_t) });

    // connect and run
    top->connect(ls, 0, sink0, 0);
    top->connect(ls, 1, errsink0, 0);
    top->connect(ls, 2, sink1, 0);
    top->connect(ls, 3, errsink1, 0);

    // Explicitly open unit because it takes quite some time
    // CPPUNIT_ASSERT_NO_THROW(ls->initialize());

    top->start();
    sleep(5);
    top->stop();
    top->wait();

    auto data = sink0->data();
    CPPUNIT_ASSERT(data.size() % 2 == 0);
    const auto sample_count = data.size() / 2;
    std::cout << "Received " << sample_count << std::endl;
    CPPUNIT_ASSERT(sample_count >= 300000 && sample_count <= 400000);
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
