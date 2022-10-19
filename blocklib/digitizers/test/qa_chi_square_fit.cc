#include "qa_chi_square_fit.h"

#include <gnuradio/attributes.h>
#include <gnuradio/flowgraph.h>

#include <digitizers/chi_square_fit.h>

#include <gnuradio/blocks/null_sink.h>
#include <gnuradio/blocks/vector_sink.h>
#include <gnuradio/blocks/vector_source.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

namespace gr::digitizers {

void qa_chi_square_fit::test_chi_square_simple_fitting() {
    // actual function variables
    std::size_t        signal_len      = 30;
    float              actual_gradient = 45;
    float              actual_offset   = 20;
    std::vector<float> signal;
    for (std::size_t i = 1; i <= signal_len; i++) {
        signal.push_back(i * actual_gradient + actual_offset);
    }

    // blocks
    auto                fg        = gr::flowgraph::make("basic_connection");
    auto                vec_src   = blocks::vector_source_f::make({ signal, false, signal_len });
    auto                vec_snk0  = blocks::vector_sink_f::make({ 2 });
    auto                vec_snk1  = blocks::vector_sink_f::make({ 2 });
    auto                null_snk0 = blocks::null_sink::make({ .itemsize = sizeof(float) });
    auto                null_snk1 = blocks::null_sink::make({ .itemsize = sizeof(int8_t) });

    std::string         names     = "gradient, offset";
    std::vector<double> inits({ actual_gradient + 3, 0.0 });
    std::vector<double> errs({ 0.0, 16.0 });
    std::vector<double> search_limit_up({ 1.2 * actual_gradient, 1.2 * actual_offset });
    std::vector<double> search_limit_dn({ -1.2 * actual_gradient, -1.2 * actual_offset });
    std::vector<int8_t> fittable = { 1, 1 };
    std::string         function = "x*[0] + 1.0*[1] ";
    auto                fitter   = digitizers::chi_square_fit<float>::make({ signal_len,
                             function,
                             static_cast<double>(signal_len),
                             1.0,
                             2,
                             names,
                             inits,
                             errs,
                             fittable,
                             search_limit_up,
                             search_limit_dn,
                             0.001 });

    // connections
    fg->connect(vec_src, 0, fitter, 0);
    fg->connect(fitter, 0, vec_snk0, 0);
    fg->connect(fitter, 1, vec_snk1, 0);
    fg->connect(fitter, 2, null_snk0, 0);
    fg->connect(fitter, 3, null_snk1, 0);

    // run this circuit
    fg->run();

    auto values = vec_snk0->data();
    auto errors = vec_snk1->data();

    // check correct results
    CPPUNIT_ASSERT(!values.empty());
    size_t min_correspondent = std::min(values.size(), errors.size());
    CPPUNIT_ASSERT_EQUAL(size_t(0), min_correspondent % 2);
    for (size_t i = 0; i < min_correspondent / 2; i++) {
        CPPUNIT_ASSERT_DOUBLES_EQUAL(actual_gradient, values.at(2 * i), 0.002);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(actual_offset, values.at(2 * i + 1), 0.002);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, errors.at(2 * i), 0.002);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, errors.at(2 * i + 1), 0.002);
    }
}

} // namespace gr::digitizers

int main(int, char **) {
    CppUnit::TextTestRunner runner;
    runner.setOutputter(CppUnit::CompilerOutputter::defaultOutputter(
            &runner.result(),
            std::cerr));
    runner.addTest(gr::digitizers::qa_chi_square_fit::suite());

    bool was_successful = runner.run("", false);
    return was_successful ? 0 : 1;
}
