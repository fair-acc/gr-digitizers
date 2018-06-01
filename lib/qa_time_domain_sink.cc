/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#include "qa_time_domain_sink.h"

#include <cppunit/TestAssert.h>
#include <digitizers/time_domain_sink.h>
#include <digitizers/status.h>
#include <digitizers/tags.h>
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

    void
    qa_time_domain_sink::stream_basics()
    {
        auto sink = time_domain_sink::make("test", "unit", 1000.0, 2048, 2, TIME_SINK_MODE_STREAMING);
        CPPUNIT_ASSERT_EQUAL(size_t{2048}, sink->get_buffer_size());
        CPPUNIT_ASSERT_EQUAL(1000.0f, sink->get_sample_rate());

        auto metadata = sink->get_metadata();
        CPPUNIT_ASSERT_EQUAL(std::string("test"), metadata.name);
        CPPUNIT_ASSERT_EQUAL(std::string("unit"), metadata.unit);
    }

    static std::vector<float>
    get_test_data(size_t size, float gain=1)
    {
        std::vector<float> data;
        for (size_t i=0; i<size; i++) {
            data.push_back(static_cast<float>(i) * gain);
        }
        return data;
    }

    void
    check_data_zero(float *returned, size_t size)
    {
        for (size_t i=0; i<size; i++) {
            CPPUNIT_ASSERT_EQUAL(float{0.0}, returned[i]);
        }
    }

    /*
     * Test what happens when no tags are passed to the sink. It is
     * expected the timestamp to be invalid (-1) and that the status
     * reads 0.
     */
    void
    qa_time_domain_sink::stream_values_no_tags()
    {
        auto top = gr::make_top_block("test");

        // test data
        size_t chunk_size = 124;
        auto data = get_test_data(chunk_size);

        auto source = gr::blocks::vector_source_f::make(data);
        auto sink = time_domain_sink::make("test", "unit", 1000.0, chunk_size, 2, TIME_SINK_MODE_STREAMING);

        // connect and run
        top->connect(source, 0, sink, 0);
        top->run();

        float values[chunk_size * 2];
        float errors[chunk_size * 2];
        measurement_info_t info;

        auto retval = sink->get_items(chunk_size, values, errors, &info);

        CPPUNIT_ASSERT_EQUAL(chunk_size, retval);
        ASSERT_VECTOR_EQUAL(data.begin(), data.end(), values);
        check_data_zero(errors, chunk_size);
        CPPUNIT_ASSERT_EQUAL(int64_t{-1}, info.timestamp);
        CPPUNIT_ASSERT_EQUAL(uint32_t{0}, info.status);
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

        auto data = get_test_data(chunk_size);
        auto source = gr::blocks::vector_source_f::make(data);

        auto data_errs = get_test_data(chunk_size, 0.01);
        auto source_errs = gr::blocks::vector_source_f::make(data_errs);

        auto sink = time_domain_sink::make("test", "unit", 1000.0, chunk_size, 2, TIME_SINK_MODE_STREAMING);

        // connect and run
        top->connect(source, 0, sink, 0);
        top->connect(source_errs, 0, sink, 1);
        top->run();

        float values[chunk_size * 2];
        float errors[chunk_size * 2];
        measurement_info_t info;

        auto retval = sink->get_items(chunk_size, values, errors, &info);

        CPPUNIT_ASSERT_EQUAL(chunk_size, retval);
        ASSERT_VECTOR_EQUAL(data.begin(), data.end(), values);
        ASSERT_VECTOR_EQUAL(data_errs.begin(), data_errs.end(), errors);
        CPPUNIT_ASSERT_EQUAL(int64_t{-1}, info.timestamp);
        CPPUNIT_ASSERT_EQUAL(uint32_t{0}, info.status);
    }

    /*!
     * \brief Sleep is used in order to relax the CPU allowing the GR machinery
     * to do its job.
     *
     * \returns true in case of timeout, else false
     */
    template<typename predicate_type>
    bool relaxed_wait_for(predicate_type pred, double timeout /*in seconds*/)
    {
      auto loop_count = static_cast<long>(timeout * 1000.0);
      for (auto i = loop_count; i > 0; --i) {
        if (pred()) {
          return false;
        }
        boost::this_thread::sleep_for(boost::chrono::milliseconds(1));;
      }
      return true;
    }

    /*
     * This test checks if samples are lost.
     */
    void
    qa_time_domain_sink::stream_overflow()
    {
        auto top = gr::make_top_block("test");

        size_t chunk_size = 124;

        auto data = get_test_data(chunk_size);
        auto source = gr::blocks::vector_source_f::make(data);

        auto data_errs = get_test_data(chunk_size, 0.01);
        auto source_errs = gr::blocks::vector_source_f::make(data_errs);

        auto sink = time_domain_sink::make("test", "unit", 1000.0, chunk_size/2, 1, TIME_SINK_MODE_STREAMING);

        // connect and run
        top->connect(source, 0, sink, 0);
        top->connect(source_errs, 0, sink, 1);

        top->run();

        float values[chunk_size];
        float errors[chunk_size];
        measurement_info_t info;

        auto retval = sink->get_items(chunk_size, values, errors, &info);
        CPPUNIT_ASSERT_EQUAL(chunk_size/2, retval);

        ASSERT_VECTOR_EQUAL(&data[0], &data[chunk_size/2], &values[0]);
        ASSERT_VECTOR_EQUAL(&data_errs[0], &data_errs[chunk_size/2], &errors[0]);
        //number of samples lost is not yet known, because the block needs to
        //receive the next block  before the next get_items call, to see that
        //there was data loss.
    }

    static void invoke_function(const data_available_event_t *event, void *ptr) {
        (*static_cast<std::function<void(const data_available_event_t *)>*>(ptr))(event);
    }

    /*
     * Note, callback is called only once the buffer is full.
     */
    void
    qa_time_domain_sink::stream_callback()
    {
        auto top = gr::make_top_block("test");

        size_t data_size = 20;

        auto data = get_test_data(data_size);
        auto source = gr::blocks::vector_source_f::make(data);

        auto sink = time_domain_sink::make("test", "unit", 1000.0, data_size, 2, TIME_SINK_MODE_STREAMING);

        data_available_event_t local_event;
        auto callback = new std::function<void(const data_available_event_t *)>(
        [&local_event] (const data_available_event_t *event)
        {
            local_event = *event;
        });

        sink->set_callback(&invoke_function, static_cast<void *>(callback));

        // connect and run
        top->connect(source, 0, sink, 0);
        top->run();

        // be nice and clean up
        delete callback;

        CPPUNIT_ASSERT_EQUAL(std::string("test"), local_event.signal_name);
        CPPUNIT_ASSERT_EQUAL(int64_t{-1}, local_event.trigger_timestamp);
    }

    static gr::tag_t
    make_test_acq_info_tag(int64_t timestamp, double timebase,
            double user_delay, uint32_t samples, uint32_t status, uint64_t offset)
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
     * Status tags are simply merged.
     */
    void
    qa_time_domain_sink::stream_acq_info_tag()
    {
        auto top = gr::make_top_block("test acq_info");

        size_t data_size = 200;
        std::vector<float> data = get_test_data(data_size);

        // Monotone clock is assumed...
        double timebase = 0.00015;
        int64_t timebase_ns = static_cast<int64_t>(timebase * 1000000000.0);
        std::vector<gr::tag_t> tags = {
                make_test_acq_info_tag(51  * timebase_ns, timebase, 0.1, 0, 1<<1, 50),
                make_test_acq_info_tag(101  * timebase_ns, timebase, 0.1, 0, 1<<2, 100),
                make_test_acq_info_tag(151  * timebase_ns, timebase, 0.1, 0, 1<<3, 150)
        };

        auto source = gr::blocks::vector_source_f::make(data, false, 1, tags);
        auto sink = gr::digitizers::time_domain_sink::make("test", "unit", 1.0f/timebase, data_size, 10, TIME_SINK_MODE_STREAMING);
        auto sink_debug = gr::blocks::tag_debug::make(sizeof(float), "test", acq_info_tag_name);
        sink_debug->set_display(false);

        // connect and run
        top->connect(source, 0, sink, 0);
        top->connect(source, 0, sink_debug, 0);
        top->run();

        CPPUNIT_ASSERT_EQUAL(3, sink_debug->num_tags());

        float values[data_size * 2];
        float errors[data_size * 2];
        measurement_info_t info;

        auto retval = sink->get_items(data_size, values, errors, &info);
        CPPUNIT_ASSERT_EQUAL(size_t{data_size}, retval);
        CPPUNIT_ASSERT_EQUAL(int64_t{timebase_ns}, info.timestamp);
        CPPUNIT_ASSERT_EQUAL(uint32_t{0xE}, info.status);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(timebase, info.timebase, 1E-5);
    }

    void
    qa_time_domain_sink::stream_partial_readout()
    {
        auto top = gr::make_top_block("test");

        size_t data_size = 124;

        auto data = get_test_data(data_size);
        auto source = gr::blocks::vector_source_f::make(data);
        auto sink = time_domain_sink::make("test", "unit", 1000.0, data_size, 2, TIME_SINK_MODE_STREAMING);

        // connect and run
        top->connect(source, 0, sink, 0);
        top->run();

        float values[data_size * 2];
        float errors[data_size * 2];
        measurement_info_t info;

        auto retval = sink->get_items(data_size/2, values, errors, &info);
        CPPUNIT_ASSERT_EQUAL(data_size/2, retval);

        // Do that again in order to get all the data, ups we've lost the data :)
        // meaning the user is responsible to read-out all the data else everything
        // is lost.
        retval = sink->get_items(data_size, values, errors, &info);
        CPPUNIT_ASSERT_EQUAL(size_t{0}, retval);

        top->stop();
        top->wait();
    }

  } /* namespace digitizers */
} /* namespace gr */

