/* -*- c++ -*- */
/* 
 * Copyright 2017 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
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

#include <boost/thread.hpp>
#include <boost/chrono.hpp>

#include <functional>

namespace gr {
  namespace digitizers {

    void
    qa_time_domain_sink::stream_basics()
    {
        auto sink = time_domain_sink::make("test", "unit", 2048, TIME_SINK_MODE_STREAMING);
        CPPUNIT_ASSERT_EQUAL(size_t{0}, sink->get_items_count());

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

    static void
    check_data_equal(std::vector<float> &expected, float *returned)
    {
        for (size_t i=0; i<expected.size(); i++) {
            CPPUNIT_ASSERT_EQUAL(expected[i], returned[i]);
        }
    }

    template <typename ItExpected, typename ItActual>
    static void
    check_data_equal(ItExpected expected_begin, ItExpected expected_end, ItActual actual_begin)
    {
        for (; expected_begin != expected_end; ++expected_begin, ++actual_begin) {
            CPPUNIT_ASSERT_EQUAL(*expected_begin, *actual_begin);
        }
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
        size_t data_size = 124;
        auto data = get_test_data(data_size);

        auto source = gr::blocks::vector_source_f::make(data);
        auto sink = time_domain_sink::make("test", "unit", data_size * 2, TIME_SINK_MODE_STREAMING);

        // connect and run
        top->connect(source, 0, sink, 0);
        top->run();

        CPPUNIT_ASSERT_EQUAL(data_size, sink->get_items_count());

        float values[data_size * 2];
        float errors[data_size * 2];
        measurement_info_t info;

        auto retval = sink->get_items(data_size * 2, values, errors, &info);

        CPPUNIT_ASSERT_EQUAL(data_size, retval);
        check_data_equal(data, values);
        check_data_zero(errors, data_size);
        CPPUNIT_ASSERT_EQUAL(int64_t{-1}, info.timestamp);
        CPPUNIT_ASSERT_EQUAL(uint32_t{0}, info.status);
        CPPUNIT_ASSERT_EQUAL(uint32_t{0}, info.pre_trigger_samples);
        CPPUNIT_ASSERT_EQUAL(data_size, size_t{info.post_trigger_samples});
    }

    /*
     * In addition to the test above the error values are tested.
     */
    void
    qa_time_domain_sink::stream_values()
    {
        auto top = gr::make_top_block("test");

        // test data
        size_t data_size = 124;

        auto data = get_test_data(data_size);
        auto source = gr::blocks::vector_source_f::make(data);

        auto data_errs = get_test_data(data_size, 0.01);
        auto source_errs = gr::blocks::vector_source_f::make(data_errs);

        auto sink = time_domain_sink::make("test", "unit", data_size * 2, TIME_SINK_MODE_STREAMING);

        // connect and run
        top->connect(source, 0, sink, 0);
        top->connect(source_errs, 0, sink, 1);
        top->run();

        CPPUNIT_ASSERT_EQUAL(data_size, sink->get_items_count());

        float values[data_size * 2];
        float errors[data_size * 2];
        measurement_info_t info;

        auto retval = sink->get_items(data_size * 2, values, errors, &info);

        CPPUNIT_ASSERT_EQUAL(data_size, retval);
        check_data_equal(data, values);
        check_data_equal(data_errs, errors);
        CPPUNIT_ASSERT_EQUAL(int64_t{-1}, info.timestamp);
        CPPUNIT_ASSERT_EQUAL(uint32_t{0}, info.status);
        CPPUNIT_ASSERT_EQUAL(uint32_t{0}, info.pre_trigger_samples);
        CPPUNIT_ASSERT_EQUAL(data_size, size_t{info.post_trigger_samples});
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
     * With the slow data sink, no samples are ever lost. The sink waits
     * until all samples are read out befor consuming new samples.
     */
    void
    qa_time_domain_sink::stream_overflow()
    {
        auto top = gr::make_top_block("test");

        size_t data_size = 124;

        auto data = get_test_data(data_size);
        auto source = gr::blocks::vector_source_f::make(data);

        auto data_errs = get_test_data(data_size, 0.01);
        auto source_errs = gr::blocks::vector_source_f::make(data_errs);

        auto sink = time_domain_sink::make("test", "unit", data_size/2, TIME_SINK_MODE_STREAMING);

        // connect and run
        top->connect(source, 0, sink, 0);
        top->connect(source_errs, 0, sink, 1);

        top->start();

        // Wait data ready
        auto timeout = relaxed_wait_for([&sink] () { return sink->is_data_rdy();}, 0.5);
        CPPUNIT_ASSERT_EQUAL(false, timeout);
        CPPUNIT_ASSERT_EQUAL(data_size/2, sink->get_items_count());

        float values[data_size * 2];
        float errors[data_size * 2];
        measurement_info_t info;

        auto retval = sink->get_items(data_size, values, errors, &info);
        CPPUNIT_ASSERT_EQUAL(data_size/2, retval);

        // Do that again in order to get all the data
        timeout = relaxed_wait_for([&sink] () { return sink->is_data_rdy();}, 0.5);
        CPPUNIT_ASSERT_EQUAL(false, timeout);
        CPPUNIT_ASSERT_EQUAL(data_size/2, sink->get_items_count());

        retval = sink->get_items(data_size/2, &values[data_size/2], &errors[data_size/2], &info);
        CPPUNIT_ASSERT_EQUAL(data_size/2, retval);

        check_data_equal(data, values);
        check_data_equal(data_errs, errors);

        top->stop();
        top->wait();
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

        auto sink = time_domain_sink::make("test", "unit", data_size, TIME_SINK_MODE_STREAMING);

        data_available_event_t local_event;
        auto callback = new std::function<void(const data_available_event_t *)>(
        [&local_event] (const data_available_event_t *event)
        {
            local_event = *event;
        });

        sink->set_items_available_callback(&invoke_function, static_cast<void *>(callback));

        // connect and run
        top->connect(source, 0, sink, 0);
        top->run();

        // be nice and clean up
        delete callback;

        CPPUNIT_ASSERT_EQUAL(data_size, sink->get_items_count());
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
        auto sink = gr::digitizers::time_domain_sink::make("test", "unit", data_size * 2, TIME_SINK_MODE_STREAMING);
        auto sink_debug = gr::blocks::tag_debug::make(sizeof(float), "test", "acq_info");
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
        CPPUNIT_ASSERT_EQUAL(int64_t{-1}, info.timestamp);
        CPPUNIT_ASSERT_EQUAL(uint32_t{0xE}, info.status);
        CPPUNIT_ASSERT_EQUAL(timebase, info.timebase);
        CPPUNIT_ASSERT_EQUAL(0.1, info.user_delay);
        CPPUNIT_ASSERT_EQUAL(0.1, info.actual_delay);
    }

    void
    qa_time_domain_sink::stream_partial_readout()
    {
        auto top = gr::make_top_block("test");

        size_t data_size = 124;

        auto data = get_test_data(data_size);
        auto source = gr::blocks::vector_source_f::make(data);
        auto sink = time_domain_sink::make("test", "unit", data_size, TIME_SINK_MODE_STREAMING);

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

    void
    qa_time_domain_sink::fast_basics()
    {
        auto sink = time_domain_sink::make("fast", "unit", 2048, TIME_SINK_MODE_TRIGGERED);
        CPPUNIT_ASSERT_EQUAL(size_t{0}, sink->get_items_count());

        auto metadata = sink->get_metadata();
        CPPUNIT_ASSERT_EQUAL(std::string("fast"), metadata.name);
        CPPUNIT_ASSERT_EQUAL(std::string("unit"), metadata.unit);
    }

    void
    qa_time_domain_sink::fast_values_no_tags()
    {
        auto top = gr::make_top_block("test");

        // test data
        size_t data_size = 124;
        auto data = get_test_data(data_size);

        auto source = gr::blocks::vector_source_f::make(data);
        auto sink = time_domain_sink::make("fast", "unit", data_size, TIME_SINK_MODE_TRIGGERED);

        // connect and run
        top->connect(source, 0, sink, 0);
        top->run();

        CPPUNIT_ASSERT_EQUAL(size_t{0}, sink->get_items_count());
    }

    void
    qa_time_domain_sink::fast_values()
    {
        auto top = gr::make_top_block("test");

        size_t data_size = 16000;
        auto data = get_test_data(data_size);
        auto data_errors = get_test_data(data_size, 0.02);

        acq_info_t trigger;
        trigger.timestamp = 987654321;
        trigger.trigger_timestamp = 9876543221;
        trigger.timebase = 0.0002;
        trigger.actual_delay = 0.004;
        trigger.user_delay = 0.0001;
        trigger.pre_samples = 1000;
        trigger.samples = 10000;
        trigger.status = 0x1;
        trigger.triggered_data = true;
        trigger.offset = 1000;

        std::vector<gr::tag_t> tags = {
           make_timebase_info_tag(trigger.timebase),
           make_acq_info_tag(trigger)
        };

        auto source = gr::blocks::vector_source_f::make(data, false, 1, tags);
        auto source_errors = gr::blocks::vector_source_f::make(data_errors, false);
        auto sink = gr::digitizers::time_domain_sink::make("test", "unit", data_size, TIME_SINK_MODE_TRIGGERED);

        // connect and run
        top->connect(source, 0, sink, 0);
        top->connect(source_errors, 0, sink, 1);
        top->run();

        float values[data_size];
        float errors[data_size];
        measurement_info_t info;

        auto retval = sink->get_items(data_size, values, errors, &info);
        auto expected_retval = (size_t)(trigger.samples + trigger.pre_samples);

        CPPUNIT_ASSERT_EQUAL(expected_retval, retval);
        check_data_equal(data.begin() + trigger.offset,
                data.begin() + trigger.offset + expected_retval,
                values);
        check_data_equal(data_errors.begin() + trigger.offset,
                data_errors.begin() + trigger.offset + expected_retval,
                errors);
        CPPUNIT_ASSERT_EQUAL(trigger.trigger_timestamp, info.trigger_timestamp);
        CPPUNIT_ASSERT_EQUAL(trigger.status, info.status);
        CPPUNIT_ASSERT_EQUAL(trigger.pre_samples, info.pre_trigger_samples);
        CPPUNIT_ASSERT_EQUAL(trigger.samples, info.post_trigger_samples);
        CPPUNIT_ASSERT_EQUAL(trigger.timebase, info.timebase);
    }

    void
    qa_time_domain_sink::fast_callback()
    {
        auto top = gr::make_top_block("test");

        size_t data_size = 16000;
        auto data = get_test_data(data_size);

        acq_info_t trigger;
        trigger.timestamp = 987654321;
        trigger.trigger_timestamp = 9876543221;
        trigger.timebase = 0.0002;
        trigger.actual_delay = 0.004;
        trigger.user_delay = 0.0001;
        trigger.pre_samples = 1000;
        trigger.samples = 10000;
        trigger.status = 0x1;
        trigger.offset = 1000;
        trigger.triggered_data = true;

        std::vector<gr::tag_t> tags = {
           make_timebase_info_tag(trigger.timebase),
           make_acq_info_tag(trigger)
        };

        auto source = gr::blocks::vector_source_f::make(data, false, 1, tags);
        auto sink = gr::digitizers::time_domain_sink::make("test", "unit",
                trigger.samples, TIME_SINK_MODE_TRIGGERED); // Trigger data doesn't fit in this buffer

        data_available_event_t local_event;
        auto callback = new std::function<void(const data_available_event_t *)>(
        [&local_event] (const data_available_event_t *event)
        {
            local_event = *event;
        });
        sink->set_items_available_callback(&invoke_function, static_cast<void *>(callback));

        // connect and run
        top->connect(source, 0, sink, 0);
        top->run();

        // check if a callback were called
        CPPUNIT_ASSERT_EQUAL(std::string("test"), local_event.signal_name);
        CPPUNIT_ASSERT_EQUAL(local_event.trigger_timestamp, trigger.trigger_timestamp);

        float values[data_size];
        float errors[data_size];
        measurement_info_t info;

        auto retval = sink->get_items(data_size, values, errors, &info);
        auto expected_retval = (size_t)(trigger.samples);

        CPPUNIT_ASSERT_EQUAL(expected_retval, retval);
        check_data_equal(data.begin() + trigger.offset,
                data.begin() + trigger.offset + expected_retval,
                values);
        CPPUNIT_ASSERT_EQUAL(trigger.trigger_timestamp, info.trigger_timestamp);
        CPPUNIT_ASSERT_EQUAL(trigger.status | CHANNEL_STATUS_NOT_ALL_DATA_EXTRACTED, info.status);
        CPPUNIT_ASSERT_EQUAL(trigger.pre_samples, info.pre_trigger_samples);
        CPPUNIT_ASSERT_EQUAL(trigger.samples, info.post_trigger_samples);
    }

  } /* namespace digitizers */
} /* namespace gr */

