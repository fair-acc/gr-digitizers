/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#include "qa_time_domain_sink.h"

#include <cppunit/TestAssert.h>
#include <digitizers/time_domain_sink.h>
#include <digitizers/status.h>
#include <gnuradio/top_block.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/blocks/tag_debug.h>

#include "utils.h"
#include "qa_common.h"

#include <boost/thread.hpp>
#include <boost/chrono.hpp>

#include <functional>

namespace gr {
  namespace digitizers {

    static std::vector<float>
    get_test_data(size_t size, float gain=1)
    {
        std::vector<float> data;
        for (size_t i=0; i<size; i++) {
            data.push_back(static_cast<float>(i) * gain);
        }
        return data;
    }

    void copy_data_callback(const float           *values,
                 std::size_t             values_size,
                 const float            *errors,
                 std::size_t             errors_size,
                 std::vector<gr::tag_t>& tags,
                 void* userdata)
    {
        qa_time_domain_sink::Test *object = static_cast<qa_time_domain_sink::Test *>(userdata);
        object->callback(values, values_size, errors, errors_size, tags);
    }

    qa_time_domain_sink::Test::Test(std::size_t size)
    {
        values_ = new float[size];
        errors_ = new float[size];
        p_values_ = values_;
        p_errors_ = errors_;
        callback_calls_ = 0;
        values_size_ = 0;
        errors_size_ = 0;
        size_max_ = size;
    }

    qa_time_domain_sink::Test::~Test()
    {
        delete[] values_;
        delete[] errors_;
    }

    void
    qa_time_domain_sink::Test::check_errors_zero()
    {
        for (size_t i=0; i<values_size_; i++) {
            CPPUNIT_ASSERT_EQUAL(float{0.0}, errors_[i]);
        }
    }

    void
    qa_time_domain_sink::Test::check_values_equal(std::vector<float>& values)
    {
        for (size_t i=0; i<values_size_; i++) {
            CPPUNIT_ASSERT_EQUAL(values[i], values_[i]);
        }
    }

    void
    qa_time_domain_sink::Test::check_errors_equal(std::vector<float>& errors)
    {
        for (size_t i=0; i<errors_size_; i++) {
            CPPUNIT_ASSERT_EQUAL(errors[i], errors_[i]);
        }
    }

    void qa_time_domain_sink::Test::callback(const float           *values,
                 std::size_t             values_size,
                 const float            *errors,
                 std::size_t             errors_size,
                 std::vector<gr::tag_t>& tags)
    {
        CPPUNIT_ASSERT( values_size + values_size_ <= size_max_ );
        CPPUNIT_ASSERT( errors_size + errors_size_ <= size_max_ );

        callback_calls_++;
        memcpy(p_values_, values, values_size * sizeof(float));
        if (errors_size > 0)
          memcpy(p_errors_, errors, errors_size * sizeof(float));

        values_size_ += values_size;
        errors_size_ += errors_size;

        p_values_ += values_size;
        p_errors_ += errors_size;

        std::vector<acq_info_t> acq_info_tags;
        for (auto tag : tags)
        {
            acq_info_tags.push_back(decode_acq_info_tag(tag));
        }
        tags_per_call_.push_back(acq_info_tags);
    }

    /// ######################## Testcases start here ############################################

    void
    qa_time_domain_sink::stream_basics()
    {
        auto sink = time_domain_sink::make("test", "unit", 1000.0, 2048, TIME_SINK_MODE_STREAMING);
        CPPUNIT_ASSERT_EQUAL(size_t{2048}, sink->get_output_package_size());
        CPPUNIT_ASSERT_EQUAL(1000.0f, sink->get_sample_rate());
    }

    /*
     * Test what happens when no tags are passed to the sink.
     */
    void
    qa_time_domain_sink::stream_values_no_tags()
    {
        auto top = gr::make_top_block("test");

        // test data
        size_t chunk_size = 124;
        auto data = get_test_data(chunk_size);
        Test test(chunk_size);

        auto source = gr::blocks::vector_source_f::make(data);
        auto sink = time_domain_sink::make("test", "unit", 1000.0, chunk_size, TIME_SINK_MODE_STREAMING);

        // connect and run
        top->connect(source, 0, sink, 0);
        sink->set_callback(copy_data_callback, &test);
        top->run();

        CPPUNIT_ASSERT_EQUAL(chunk_size, test.values_size_);
        test.check_values_equal(data);
        CPPUNIT_ASSERT(test.tags_per_call_[0].empty());
        CPPUNIT_ASSERT_EQUAL(test.errors_size_, size_t(0));
    }

    /*
     * In addition to the test above the error values are tested.
     */
    void
    qa_time_domain_sink::stream_values()
    {
        auto top = gr::make_top_block("test");

        // test data
        size_t chunk_size = 124;
        Test test(chunk_size);
        auto data = get_test_data(chunk_size);
        auto source = gr::blocks::vector_source_f::make(data);

        auto data_errs = get_test_data(chunk_size, 0.01);
        auto source_errs = gr::blocks::vector_source_f::make(data_errs);

        auto sink = time_domain_sink::make("test", "unit", 1000.0, chunk_size, TIME_SINK_MODE_STREAMING);

        // connect and run
        top->connect(source, 0, sink, 0);
        top->connect(source_errs, 0, sink, 1);
        sink->set_callback(copy_data_callback, &test);
        top->run();

        CPPUNIT_ASSERT_EQUAL(chunk_size, test.values_size_);
        CPPUNIT_ASSERT_EQUAL(chunk_size, test.errors_size_);
        test.check_values_equal(data);
        test.check_errors_equal(data_errs);
        CPPUNIT_ASSERT(test.tags_per_call_[0].empty());
    }

    /*
     * Should still run, even if no callback is available
     */
    void
    qa_time_domain_sink::stream_no_callback()
    {
        auto top = gr::make_top_block("test");

        size_t data_size = 20;

        auto data = get_test_data(data_size);
        auto source = gr::blocks::vector_source_f::make(data);

        auto sink = time_domain_sink::make("no_callback_test", "unit", 1000.0, data_size, TIME_SINK_MODE_STREAMING);

        // connect and run
        top->connect(source, 0, sink, 0);
        top->run();

        // Just run it without failure
    }

    static gr::tag_t
    make_test_acq_info_tag(int64_t timestamp, double timebase, double user_delay, uint32_t samples, uint32_t status, uint64_t offset)
    {
        acq_info_t info{};
        info.timestamp = timestamp;
        info.timebase = timebase;
        info.user_delay = user_delay;
        info.actual_delay = user_delay;
        info.samples = samples;
        info.status = status;
        info.offset = offset;

        return make_acq_info_tag(info);
    };
    
    /*
     * tags are forwarded, split across multiple the packages
     */
    void
    qa_time_domain_sink::stream_acq_info_tag()
    {
        auto top = gr::make_top_block("test acq_info");

        size_t data_size = 300;
        int number_of_chunks = 3;
        std::vector<float> data = get_test_data(data_size);

        Test test(data_size);

        // Monotone clock is assumed...
        double timebase = 0.00015;
        int64_t timebase_ns = static_cast<int64_t>(timebase * 1000000000.0);
        std::vector<gr::tag_t> tags = {
                make_test_acq_info_tag(50  * timebase_ns, timebase, 0.1, 0, 1<<1, 50),
                make_test_acq_info_tag(150  * timebase_ns, timebase, 0.1, 0, 1<<2, 150),
                make_test_acq_info_tag(250  * timebase_ns, timebase, 0.1, 0, 1<<3, 250)
        };

        auto source = gr::blocks::vector_source_f::make(data, false, 1, tags);
        auto sink = gr::digitizers::time_domain_sink::make("test", "unit", 1.0f/timebase, data_size/number_of_chunks, TIME_SINK_MODE_STREAMING);

        sink->set_callback(copy_data_callback, &test);

        // connect and run
        top->connect(source, 0, sink, 0);
        top->run();

        CPPUNIT_ASSERT_EQUAL(test.callback_calls_, number_of_chunks);
        test.check_values_equal(data);

        for ( auto tags_on_call : test.tags_per_call_ )
        {
            CPPUNIT_ASSERT_EQUAL(size_t(1),tags_on_call.size()); // one tag per chunk is expected
        }
    }

  } /* namespace digitizers */
} /* namespace gr */

