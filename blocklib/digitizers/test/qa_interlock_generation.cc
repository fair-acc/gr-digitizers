#include "qa_interlock_generation.h"
#include <digitizers/interlock_generation.h>

#include <gnuradio/attributes.h>
#include <gnuradio/blocks/vector_sink.h>
#include <gnuradio/blocks/vector_source.h>
#include <gnuradio/flowgraph.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

namespace gr::digitizers {

int count_interlock_calls = 0;

void interlock(int64_t, void*) { count_interlock_calls++; }

void qa_interlock_generation::interlock_generation_test()
{
    auto fg = gr::flowgraph::make("interlock_generation_test");

    std::vector<float> sig_v;
    std::vector<float> min_v;
    std::vector<float> max_v;

    // signal gets smaller than minimum
    for (int i = 0; i < 10; i++) {
        sig_v.push_back(0);
        min_v.push_back(i - 5);
        max_v.push_back(i + 5);
    }
    // signal gets bigger than maximum
    for (int i = 0; i < 10; i++) {
        sig_v.push_back(0);
        min_v.push_back(i - 15);
        max_v.push_back(i - 5);
    }
    auto sig = blocks::vector_source_f::make({ sig_v });
    auto min = blocks::vector_source_f::make({ min_v });
    auto max = blocks::vector_source_f::make({ max_v });

    auto i_lk = interlock_generation::make({ -100, 100 });

    i_lk->set_callback(&interlock, nullptr);

    auto snk = blocks::vector_sink_f::make({ 1 });

    fg->connect(sig, 0, i_lk, 0);
    fg->connect(min, 0, i_lk, 1);
    fg->connect(max, 0, i_lk, 2);
    fg->connect(i_lk, 0, snk, 0);

    fg->run();

    auto interlocks = snk->data();

    CPPUNIT_ASSERT_EQUAL(sig_v.size(), interlocks.size());
    CPPUNIT_ASSERT_EQUAL(1, count_interlock_calls);

    for (size_t i = 0; i < sig_v.size(); i++) {
        bool exp = (sig_v.at(i) < max_v.at(i) && sig_v.at(i) > min_v.at(i));
        bool act = (interlocks.at(i) == 0);
        CPPUNIT_ASSERT_EQUAL(exp, act);
    }
}

} /* namespace gr::digitizers */

int main(int, char**)
{
    CppUnit::TextTestRunner runner;
    runner.setOutputter(
        CppUnit::CompilerOutputter::defaultOutputter(&runner.result(), std::cerr));
    runner.addTest(gr::digitizers::qa_interlock_generation::suite());

    bool was_successful = runner.run("", false);
    return was_successful ? 0 : 1;
}