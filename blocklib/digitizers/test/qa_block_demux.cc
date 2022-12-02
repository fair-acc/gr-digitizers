#include <gnuradio/attributes.h>
#include <gnuradio/flowgraph.h>

#include "qa_block_demux.h"

#include <gnuradio/blocks/vector_sink.h>
#include <gnuradio/blocks/vector_source.h>
#include <digitizers/block_demux.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

namespace gr::digitizers {

void qa_block_demux::passes_only_desired()
{
    std::vector<unsigned char> vals = { 0, 1, 2, 3, 4, 5, 6, 7 };
    auto fg = gr::flowgraph::make("basic_connection");
    auto demux = digitizers::block_demux::make({ 0 });
    auto src = gr::blocks::vector_source_b::make({ vals });
    auto snk = gr::blocks::vector_sink_f::make({ 1 });

    fg->connect(src, 0, demux, 0);
    fg->connect(demux, 0, snk, 0);

    fg->run();

    auto data = snk->data();

    CPPUNIT_ASSERT(data.size() != 0);
    for (std::size_t i = 0; i < data.size(); i++) {
        if (i % 2 == 0) {
            CPPUNIT_ASSERT(data[i] == 0.0);
        }
        else {
            CPPUNIT_ASSERT(data[i] == 1.0);
        }
    }
}

} // namespace gr::digitizers

int main(int, char**)
{
    CppUnit::TextTestRunner runner;
    runner.setOutputter(
        CppUnit::CompilerOutputter::defaultOutputter(&runner.result(), std::cerr));
    runner.addTest(gr::digitizers::qa_block_demux::suite());

    bool was_successful = runner.run("", false);
    return was_successful ? 0 : 1;
}