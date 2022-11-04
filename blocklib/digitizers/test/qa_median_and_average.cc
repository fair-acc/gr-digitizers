#include <gnuradio/flowgraph.h>

#include "qa_median_and_average.h"
#include <digitizers/median_and_average.h>
#include <gnuradio/attributes.h>
#include <gnuradio/blocks/vector_sink.h>
#include <gnuradio/blocks/vector_source.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

namespace gr::digitizers {

void qa_median_and_average::basic_median_and_average() {
    // Put test here
    auto               fg = gr::flowgraph::make("basic_median_and_average");
    std::vector<float> data({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 });
    std::size_t        vec_size = data.size();
    auto               src      = blocks::vector_source_f::make({ data, false, vec_size });
    auto               snk      = blocks::vector_sink_f::make({ vec_size });
    auto               flt      = digitizers::median_and_average::make({ vec_size, 3, 2 });

    fg->connect(src, 0, flt, 0);
    fg->connect(flt, 0, snk, 0);

    fg->run();

    auto results = snk->data();
    CPPUNIT_ASSERT_EQUAL(data.size(), results.size());
    for (auto val : results) {
        CPPUNIT_ASSERT(val > 0.0);
        CPPUNIT_ASSERT(val < 10.0);
    }
}

} /* namespace gr::digitizers */

int main(int, char **) {
    CppUnit::TextTestRunner runner;
    runner.setOutputter(CppUnit::CompilerOutputter::defaultOutputter(
            &runner.result(),
            std::cerr));
    runner.addTest(gr::digitizers::qa_median_and_average::suite());

    bool was_successful = runner.run("", false);
    return was_successful ? 0 : 1;
}
