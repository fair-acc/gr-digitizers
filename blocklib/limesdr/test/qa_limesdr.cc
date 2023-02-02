
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
