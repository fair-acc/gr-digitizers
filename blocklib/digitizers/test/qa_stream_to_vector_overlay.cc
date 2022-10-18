#include "qa_stream_to_vector_overlay.h"

#include <digitizers/stream_to_vector_overlay.h>

#include <gnuradio/attributes.h>
#include <gnuradio/blocks/vector_sink.h>
#include <gnuradio/blocks/vector_source.h>
#include <gnuradio/flowgraph.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

namespace gr::digitizers {
void test_different_params(std::size_t size, double delta) {
    std::vector<float> vec;
    for (std::size_t i = 0; i < (size * delta * 8); i++) {
        vec.push_back(i);
    }

    auto src = blocks::vector_source_f::make({ vec });
    auto blk = stream_to_vector_overlay_ff::make({ size, 1, delta });
    auto snk = blocks::vector_sink_f::make({ size });

    auto fg  = gr::flowgraph::make("single_input_test");

    fg->connect(src, 0, blk, 0);
    fg->connect(blk, 0, snk, 0);

    fg->run();

    auto data = snk->data();

    for (std::size_t i = 0; i < data.size() / size; i++) {
        for (std::size_t j = 0; j < size; j++) {
            CPPUNIT_ASSERT_DOUBLES_EQUAL((i * delta + j) * 1.0, data.at(i * size + j), 0.02);
        }
    }
}
void qa_stream_to_vector_overlay::t1() {
    for (int i = 1; i < 10; i++) {
        for (int j = 1; j < 10; j++) {
            test_different_params(i, j);
        }
    }
}

} /* namespace gr::digitizers */

int main(int, char **) {
    CppUnit::TextTestRunner runner;
    runner.setOutputter(CppUnit::CompilerOutputter::defaultOutputter(
            &runner.result(),
            std::cerr));
    runner.addTest(gr::digitizers::qa_stream_to_vector_overlay::suite());

    bool was_successful = runner.run("", false);
    return was_successful ? 0 : 1;
}
