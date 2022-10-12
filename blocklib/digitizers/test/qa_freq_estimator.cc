#include "qa_freq_estimator.h"
#include <digitizers/freq_estimator.h>

#include <gnuradio/attributes.h>
#include <gnuradio/blocks/vector_sink.h>
#include <gnuradio/blocks/vector_source.h>
#include <gnuradio/flowgraph.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

namespace gr::digitizers {

void qa_freq_estimator::basic_frequency_estimation() {
    int  sig_avg_window  = 4;
    int  freq_avg_window = 10;
    auto fg              = gr::flowgraph::make("basic_connection");
    // Put test here
    std::vector<float> cycle({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, -1, -2, -3, -4, -5, -6, -7, -8, -9, -8, -7, -6, -5, -4, -3, -2, -1 });
    std::vector<float> sig;
    for (int i = 0; i < 100; i++) {
        sig.insert(sig.end(), cycle.begin(), cycle.end());
    }
    auto sine = gr::blocks::vector_source_f::make({ sig });
    auto freq = digitizers::freq_estimator<float>::make({ 36000, sig_avg_window, freq_avg_window, 1 });
    auto sink = blocks::vector_sink_f::make({ 1 });

    fg->connect(sine, 0, freq, 0);
    fg->connect(freq, 0, sink, 0);

    fg->run();
    auto data = sink->data();
    CPPUNIT_ASSERT(data.size() > 0);
    for (size_t i = freq_avg_window * cycle.size(); i < data.size(); i++) {
        CPPUNIT_ASSERT_DOUBLES_EQUAL(1000, data.at(i), 10.0);
    }
}
} /* namespace gr::digitizers */

int main(int, char **) {
    CppUnit::TextTestRunner runner;
    runner.setOutputter(CppUnit::CompilerOutputter::defaultOutputter(
            &runner.result(),
            std::cerr));
    runner.addTest(gr::digitizers::qa_freq_estimator::suite());

    bool was_successful = runner.run("", false);
    return was_successful ? 0 : 1;
}