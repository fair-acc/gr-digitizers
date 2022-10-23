#include "qa_stft_goertzl_dynamic_decimated.h"

#include <gnuradio/attributes.h>
#include <gnuradio/blocks/vector_sink.h>
#include <gnuradio/blocks/vector_source.h>
#include <gnuradio/flowgraph.h>

#include <gnuradio/digitizers/stft_goertzl_dynamic_decimated.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

namespace gr::digitizers {

void qa_stft_goertzl_dynamic_decimated::t1() {
    auto               fg        = gr::flowgraph::make("basic_goertzl_dynamic");

    std::size_t        win_size  = 1024;
    double             freq      = 512;
    double             samp_rate = 10000;
    std::size_t        nbins     = 100;
    std::vector<float> min_v;
    std::vector<float> max_v;
    std::vector<float> sig_v;
    for (std::size_t i = 0; i < win_size * 2; i++) {
        sig_v.push_back(sin(2.0 * M_PI * i * freq / samp_rate));
        if (i < win_size) {
            min_v.push_back(0);
            max_v.push_back(2 * freq);
        } else {
            min_v.push_back(freq);
            max_v.push_back(samp_rate / 2);
        }
    }
    auto src  = blocks::vector_source_f::make({ sig_v });
    auto min  = blocks::vector_source_f::make({ min_v });
    auto max  = blocks::vector_source_f::make({ max_v });

    auto snk0 = blocks::vector_sink_f::make({ nbins });
    auto snk1 = blocks::vector_sink_f::make({ nbins });
    auto snk2 = blocks::vector_sink_f::make({ nbins });
    auto stft = stft_goertzl_dynamic_decimated::make({ samp_rate, (1.0 * win_size) / samp_rate, win_size, nbins });

    fg->connect(src, 0, stft, 0);
    fg->connect(min, 0, stft, 1);
    fg->connect(max, 0, stft, 2);
    fg->connect(stft, 0, snk0, 0);
    fg->connect(stft, 1, snk1, 0);
    fg->connect(stft, 2, snk2, 0);

    fg->run();

    auto mag_data = snk0->data();

    CPPUNIT_ASSERT_EQUAL(size_t(2 * nbins), mag_data.size());

    size_t max_i = 0;

    for (std::size_t i = 0; i < nbins; i++) {
        if (mag_data.at(i) > mag_data.at(max_i)) {
            max_i = i;
        }
    }
    CPPUNIT_ASSERT_EQUAL(size_t(49), max_i);
    // next window maximum
    max_i = nbins;
    for (std::size_t i = nbins; i < 2 * nbins; i++) {
        if (mag_data.at(i) > mag_data.at(max_i)) {
            max_i = i;
        }
    }
    CPPUNIT_ASSERT_EQUAL(size_t(100), max_i);
}

} /* namespace gr::digitizers */

int main(int, char **) {
    CppUnit::TextTestRunner runner;
    runner.setOutputter(CppUnit::CompilerOutputter::defaultOutputter(
            &runner.result(),
            std::cerr));
    runner.addTest(gr::digitizers::qa_stft_goertzl_dynamic_decimated::suite());

    bool was_successful = runner.run("", false);
    return was_successful ? 0 : 1;
}