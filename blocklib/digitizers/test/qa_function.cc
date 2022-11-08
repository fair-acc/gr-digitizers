/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#include "qa_function.h"
#include "function.h"
#include "qa_common.h"

#include <digitizers/tags.h>
#include <gnuradio/attributes.h>
#include <gnuradio/blocks/vector_sink.h>
#include <gnuradio/blocks/vector_source.h>
#include <gnuradio/flowgraph.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

namespace gr::digitizers {

struct function_test_flowgraph_t {
    gr::flowgraph::sptr               top;
    gr::blocks::vector_source_f::sptr timing;
    function::sptr                    func;
    gr::blocks::vector_sink_f::sptr   ref_sink;
    gr::blocks::vector_sink_f::sptr   min_sink;
    gr::blocks::vector_sink_f::sptr   max_sink;

    function_test_flowgraph_t(std::size_t nsamples, function::sptr function, const std::vector<tag_t> &tags = std::vector<tag_t>()) {
        func     = function;

        top      = gr::flowgraph::make("test");
        timing   = gr::blocks::vector_source_f::make({ .data = std::vector<float>(nsamples), .tags = tags });
        ref_sink = gr::blocks::vector_sink_f::make({});
        min_sink = gr::blocks::vector_sink_f::make({});
        max_sink = gr::blocks::vector_sink_f::make({});

        top->connect(timing, 0, func, 0);

        top->connect(func, 0, ref_sink, 0);
        top->connect(func, 1, min_sink, 0);
        top->connect(func, 2, max_sink, 0);
    }

    void run() {
        top->run();
    }

#ifdef PORT_DISABLED // TODO(PORT) sink->reset() not functional
    void reset() {
        ref_sink->reset();
        min_sink->reset();
        max_sink->reset();
        timing->rewind();
    }
#endif
};

void qa_function::test_no_timing() {
    std::vector<float>        time     = { 0.0, 1.0 };
    std::vector<float>        ref      = { 1.5, 2.5 };
    std::vector<float>        min      = { 1.0, 2.0 };
    std::vector<float>        max      = { 2.0, 3.0 };

    auto                      func     = function::make({ 1, time, ref, min, max });

    size_t                    nsamples = 10000;
    function_test_flowgraph_t fg(nsamples, func);
    fg.run();

    auto dref = fg.ref_sink->data();
    auto dmin = fg.min_sink->data();
    auto dmax = fg.max_sink->data();

    CPPUNIT_ASSERT_EQUAL(nsamples, dref.size());
    CPPUNIT_ASSERT_EQUAL(nsamples, dmin.size());
    CPPUNIT_ASSERT_EQUAL(nsamples, dmax.size());

    ASSERT_VECTOR_OF(dref, 0.0f);
    ASSERT_VECTOR_OF(dmin, 0.0f);
    ASSERT_VECTOR_OF(dmax, 0.0f);

#ifdef PORT_DISABLED // TODO(PORT) reset() not functional
    fg.reset();
    fg.run();

    dref = fg.ref_sink->data();
    dmin = fg.min_sink->data();
    dmax = fg.max_sink->data();

    CPPUNIT_ASSERT_EQUAL(nsamples, dref.size());
    CPPUNIT_ASSERT_EQUAL(nsamples, dmin.size());
    CPPUNIT_ASSERT_EQUAL(nsamples, dmax.size());

    ASSERT_VECTOR_EQUAL(ref.begin(), dref.begin(), dref.end());
    ASSERT_VECTOR_EQUAL(min.begin(), dmin.begin(), dmin.end());
    ASSERT_VECTOR_EQUAL(max.begin(), dmax.begin(), dmax.end());
#endif
}

void qa_function::test_function() {
    std::vector<float> time     = { 0.5, 0.9 };
    std::vector<float> ref      = { 1.5, 2.5 };
    std::vector<float> min      = { 1.0, 2.0 };
    std::vector<float> max      = { 2.0, 3.0 };

    auto               func     = function::make({ 1, time, ref, min, max });

    std::size_t        nsamples = 10000;
    acq_info_t         info{};
    info.timebase = 1.0 / nsamples;

    function_test_flowgraph_t fg(nsamples, func, std::vector<gr::tag_t>{ make_acq_info_tag(info, 0) });
    fg.run();

    auto dref = fg.ref_sink->data();
    auto dmin = fg.min_sink->data();
    auto dmax = fg.max_sink->data();

    CPPUNIT_ASSERT_EQUAL(ref[0], dref[0]);
    CPPUNIT_ASSERT_EQUAL(min[0], min[0]);
    CPPUNIT_ASSERT_EQUAL(max[0], max[0]);

    CPPUNIT_ASSERT_EQUAL(ref[1], dref[nsamples - 1]);
    CPPUNIT_ASSERT_EQUAL(min[1], dmin[nsamples - 1]);
    CPPUNIT_ASSERT_EQUAL(max[1], dmax[nsamples - 1]);
}

} // namespace gr::digitizers

int main(int, char **) {
    CppUnit::TextTestRunner runner;
    runner.setOutputter(CppUnit::CompilerOutputter::defaultOutputter(
            &runner.result(),
            std::cerr));
    runner.addTest(gr::digitizers::qa_function::suite());

    bool was_successful = runner.run("", false);
    return was_successful ? 0 : 1;
}
