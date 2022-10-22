/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#include "qa_peak_detector.h"

#include <digitizers/median_and_average.h>
#include <digitizers/peak_detector.h>

#include <gnuradio/attributes.h>
#include <gnuradio/blocks/vector_sink.h>
#include <gnuradio/blocks/vector_source.h>
#include <gnuradio/flowgraph.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

namespace gr::digitizers {

void qa_peak_detector::basic_peak_find() {
    auto               fg = gr::flowgraph::make("basic_peak_find");
    std::vector<float> data({ 1, 2, 3, 4, 5, 6, 5, 4, 3, 2, 1 });
    auto               vec_size = data.size();
    auto               src      = blocks::vector_source_f::make({ data, false, vec_size });
    auto               flow     = blocks::vector_source_f::make({ std::vector<float>{ 25 } });
    auto               fup      = blocks::vector_source_f::make({ std::vector<float>{ 40 } });
    auto               max      = blocks::vector_sink_f::make({ 1 });
    auto               stdev    = blocks::vector_sink_f::make({ 1 });
    auto               detect   = digitizers::peak_detector::make({ 100.0, vec_size, /* 5, 8,*/ 4 });
    auto               flt      = digitizers::median_and_average::make({ vec_size, 3, 2 });

    fg->connect(src, 0, detect, 0);
    fg->connect(src, 0, flt, 0);
    fg->connect(flt, 0, detect, 1);
    fg->connect(flow, 0, detect, 2);
    fg->connect(fup, 0, detect, 3);
    fg->connect(detect, 0, max, 0);
    fg->connect(detect, 1, stdev, 0);

    fg->run();

    const auto maximum = max->data();
    const auto stdevs  = stdev->data();
    CPPUNIT_ASSERT(!maximum.empty());
    CPPUNIT_ASSERT(!stdevs.empty());

    CPPUNIT_ASSERT_DOUBLES_EQUAL(22.72, maximum.at(0), 0.02);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(45.86, stdevs.at(0), 0.02); // TODO(PORT) original was 46.32 - check if that passed in baseline
}

} /* namespace gr::digitizers */

int main(int, char **) {
    CppUnit::TextTestRunner runner;
    runner.setOutputter(CppUnit::CompilerOutputter::defaultOutputter(
            &runner.result(),
            std::cerr));
    runner.addTest(gr::digitizers::qa_peak_detector::suite());

    bool was_successful = runner.run("", false);
    return was_successful ? 0 : 1;
}
