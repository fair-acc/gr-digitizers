/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#include "qa_post_mortem_sink.h"

#include <cppunit/TestAssert.h>
#include <digitizers/post_mortem_sink.h>
#include <gnuradio/top_block.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/blocks/vector_sink_f.h>
#include <gnuradio/blocks/tag_debug.h>
#include "utils.h"
#include "qa_common.h"
#include <digitizers/tags.h>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <functional>

namespace gr {
  namespace digitizers {

    void
    qa_post_mortem_sink::basics()
    {
        auto sink = post_mortem_sink::make("test", "unit", 1234.0f, 2048);

        auto metadata = sink->get_metadata();
        CPPUNIT_ASSERT_EQUAL(std::string("test"), metadata.name);
        CPPUNIT_ASSERT_EQUAL(std::string("unit"), metadata.unit);
        CPPUNIT_ASSERT_EQUAL(1234.0f, sink->get_sample_rate());

        size_t data_size = 10;
        float values[data_size];
        float errors[data_size];
        measurement_info_t info;

        auto retval = sink->get_items(data_size, values, errors, &info);
        CPPUNIT_ASSERT_EQUAL(size_t{0}, retval);
    }

    template <typename ItExpected, typename ItActual>
    static void
    assert_equal(ItExpected expected_begin, ItExpected expected_end, ItActual actual_begin)
    {
        for (; expected_begin != expected_end; ++expected_begin, ++actual_begin) {
            CPPUNIT_ASSERT_EQUAL(*expected_begin, *actual_begin);
        }
    }

    template <typename ItExpected, typename ItActual>
    static void
    assert_equal(const ItExpected &expected, const ItActual &returned)
    {
        for (size_t i=0; i<expected.size(); i++) {
            CPPUNIT_ASSERT_EQUAL(expected[i], returned[i]);
        }
    }

    template <typename ItExpected, typename ItActual>
    static void
    assert_equal(const ItExpected &expected, const ItActual *returned)
    {
        for (size_t i=0; i<expected.size(); i++) {
            CPPUNIT_ASSERT_EQUAL(expected[i], returned[i]);
        }
    }

    static void
    assert_zero(float *returned, size_t size)
    {
        for (size_t i=0; i<size; i++) {
            CPPUNIT_ASSERT_EQUAL(float{0.0}, returned[i]);
        }
    }

    static const float DEFAULT_SAMP_RATE = 100000.0f;

    void
    qa_post_mortem_sink::buffer_not_full()
    {
        auto top = gr::make_top_block("test");

        // test data
        size_t data_size = 124;
        auto data = make_test_data(data_size);

        auto source = gr::blocks::vector_source_f::make(data);
        auto pm = post_mortem_sink::make("test", "unit", DEFAULT_SAMP_RATE, data_size * 2);
        auto sink = gr::blocks::vector_sink_f::make();

        // connect and run
        top->connect(source, 0, pm, 0);
        top->connect(pm, 0, sink, 0);
        top->run();

        CPPUNIT_ASSERT_EQUAL(data_size, pm->nitems_read(0));

        float values[data_size * 2];
        float errors[data_size * 2];
        measurement_info_t info;

        auto retval = pm->get_items(data_size * 2, values, errors, &info);

        CPPUNIT_ASSERT_EQUAL(data_size, retval);
        assert_equal(data, values);
        assert_equal(data, sink->data());
        assert_zero(errors, data_size);
        CPPUNIT_ASSERT_EQUAL(int64_t{-1}, info.timestamp);
    }

    void
    qa_post_mortem_sink::buffer_full()
    {
        auto top = gr::make_top_block("test");

        // test data
        size_t data_size = 124;
        auto data = make_test_data(data_size);
        auto data_errs = make_test_data(data_size, 0.01);

        auto source = gr::blocks::vector_source_f::make(data);
        auto source_errs = gr::blocks::vector_source_f::make(data_errs);
        auto pm = post_mortem_sink::make("test", "unit", DEFAULT_SAMP_RATE, data_size);
        auto sink = gr::blocks::vector_sink_f::make();
        auto sink_errs = gr::blocks::vector_sink_f::make();

        // connect and run
        top->connect(source, 0, pm, 0);
        top->connect(source_errs, 0, pm, 1);
        top->connect(pm, 0, sink, 0);
        top->connect(pm, 1, sink_errs, 0);
        top->run();

        CPPUNIT_ASSERT_EQUAL(data_size, pm->nitems_read(0));

        float values[data_size * 2];
        float errors[data_size * 2];
        measurement_info_t info;

        auto retval = pm->get_items(data_size * 2, values, errors, &info);

        CPPUNIT_ASSERT_EQUAL(data_size, retval);
        assert_equal(data, values);
        assert_equal(data, sink->data());
        assert_equal(data_errs, errors);
        assert_equal(data_errs, sink_errs->data());
        CPPUNIT_ASSERT_EQUAL(int64_t{-1}, info.timestamp);
    }

    void
    qa_post_mortem_sink::buffer_overflow()
    {
        auto top = gr::make_top_block("test");

        size_t data_size = 124;
        auto data = make_test_data(data_size);
        auto data_errs = make_test_data(data_size, 0.01);

        auto source = gr::blocks::vector_source_f::make(data);
        auto source_errs = gr::blocks::vector_source_f::make(data_errs);
        auto pm = post_mortem_sink::make("test", "unit", DEFAULT_SAMP_RATE, data_size - 1);
        auto sink = gr::blocks::vector_sink_f::make();
        auto sink_errs = gr::blocks::vector_sink_f::make();

        // connect and run
        top->connect(source, 0, pm, 0);
        top->connect(source_errs, 0, pm, 1);
        top->connect(pm, 0, sink, 0);
        top->connect(pm, 1, sink_errs, 0);

        top->run();

        CPPUNIT_ASSERT_EQUAL(data_size, pm->nitems_read(0));

        float values[data_size];
        float errors[data_size];
        measurement_info_t info;

        auto retval = pm->get_items(data_size, values, errors, &info);

        CPPUNIT_ASSERT_EQUAL(data_size - 1, retval);
        assert_equal(data.begin() + 1, data.end(), values);
        assert_equal(data, sink->data());
        assert_equal(data_errs.begin() + 1, data_errs.end(), errors);
        assert_equal(data_errs, sink_errs->data());
        CPPUNIT_ASSERT_EQUAL(int64_t{-1}, info.timestamp);
    }

    // Check if timestamp is calculated correctly
    void
    qa_post_mortem_sink::acq_info()
    {
        size_t data_size = 200;

        // Monotone clock is assumed...
        double timebase = 0.00015;
        acq_info_t info{};
        info.timebase = timebase;
        info.timestamp = 321;
        info.status = 1<<1;
        info.samples = 50;

        std::vector<gr::tag_t> tags = {
              make_acq_info_tag(info)
        };

        auto top = gr::make_top_block("test");

        auto data = make_test_data(data_size);
        auto data_errs = make_test_data(data_size, 0.01);

        auto source = gr::blocks::vector_source_f::make(data, false, 1, tags);
        auto source_errs = gr::blocks::vector_source_f::make(data_errs);
        auto pm = post_mortem_sink::make("test", "unit", DEFAULT_SAMP_RATE, data_size * 2);
        auto sink = gr::blocks::vector_sink_f::make();
        auto sink_errs = gr::blocks::vector_sink_f::make();

        // connect and run
        top->connect(source, 0, pm, 0);
        top->connect(source_errs, 0, pm, 1);
        top->connect(pm, 0, sink, 0);
        top->connect(pm, 1, sink_errs, 0);

        top->run();

        float values[data_size];
        float errors[data_size];
        measurement_info_t minfo;

        auto retval = pm->get_items(data_size, values, errors, &minfo);
        CPPUNIT_ASSERT_EQUAL(data_size, retval);
        CPPUNIT_ASSERT_EQUAL(321, (int)minfo.timestamp);
        CPPUNIT_ASSERT_EQUAL(uint32_t{1<<1}, minfo.status);
        CPPUNIT_ASSERT_EQUAL(timebase, minfo.timebase);
    }

  } /* namespace digitizers */
} /* namespace gr */

