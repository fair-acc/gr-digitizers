#include "qa_block_scaling_offset.h"

#include <gnuradio/attributes.h>
#include <gnuradio/blocks/vector_sink.h>
#include <gnuradio/blocks/vector_source.h>
#include <gnuradio/flowgraph.h>

#include <digitizers/block_scaling_offset.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

namespace gr::digitizers {

void qa_block_scaling_offset::scale_and_offset()
{
    auto fg = gr::flowgraph::make("scale_and_offset");

    double scale = 1.5;
    double offset = 2;
    int n = 30;
    std::vector<float> data;
    for (int i = 0; i < n; i++) {
        data.push_back(i);
    }

    auto src0 = blocks::vector_source_f::make({ data });
    auto src1 = blocks::vector_source_f::make({ data });
    auto snk0 = blocks::vector_sink_f::make({ 1 });
    auto snk1 = blocks::vector_sink_f::make({ 1 });
    auto bso = block_scaling_offset::make({ scale, offset });

    fg->connect(src0, 0, bso, 0);
    fg->connect(bso, 0, snk0, 0);
    fg->connect(src1, 0, bso, 1);
    fg->connect(bso, 1, snk1, 0);

    fg->run();

    auto result0 = snk0->data();
    auto result1 = snk1->data();
    CPPUNIT_ASSERT_EQUAL(data.size(), result0.size());
    CPPUNIT_ASSERT_EQUAL(data.size(), result1.size());

    for (int i = 0; i < n; i++) {
        float should_be0 = ((data.at(i) * scale) - offset);
        float should_be1 = (data.at(i) * scale);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(should_be0, result0.at(i), 0.0001);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(should_be1, result1.at(i), 0.0001);
    }
}
} /* namespace gr::digitizers */

int main(int, char**)
{
    CppUnit::TextTestRunner runner;
    runner.setOutputter(
        CppUnit::CompilerOutputter::defaultOutputter(&runner.result(), std::cerr));
    runner.addTest(gr::digitizers::qa_block_scaling_offset::suite());

    bool was_successful = runner.run("", false);
    return was_successful ? 0 : 1;
}
