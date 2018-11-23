/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#include <gnuradio/attributes.h>
#include <cppunit/TestAssert.h>
#include "qa_picoscope_4000a.h"
#include <digitizers/picoscope_4000a.h>
#include <digitizers/time_domain_sink.h>
#include <thread>
#include <chrono>
#include <digitizers/tags.h>
#include <gnuradio/top_block.h>
#include <gnuradio/blocks/null_sink.h>
#include <gnuradio/blocks/vector_sink_f.h>
#include <gnuradio/blocks/vector_sink_b.h>
#include <gnuradio/blocks/null_sink.h>
#include <gnuradio/blocks/tag_debug.h>
#include "utils.h"

using namespace std::chrono;

namespace gr {
  namespace digitizers {

    void
    qa_picoscope_4000a::open_close()
    {
      auto ps = picoscope_4000a::make("", true);

      // this takes time, so we do it a few times only
      for (auto i=0; i<3; i++) {
        CPPUNIT_ASSERT_NO_THROW(
          ps->initialize();
        );

        auto driver_version = ps->get_driver_version();
        CPPUNIT_ASSERT(!driver_version.empty());

        auto hw_version = ps->get_hardware_version();
        CPPUNIT_ASSERT(!hw_version.empty());

        CPPUNIT_ASSERT_NO_THROW(
          ps->close();
        );
      }
    }

    void
    qa_picoscope_4000a::rapid_block_basics()
    {
      auto top = gr::make_top_block("basics");

      auto ps = picoscope_4000a::make("", true);
      ps->set_aichan("A", true, 5.0, false);
      ps->set_samp_rate(10000.0);
      ps->set_samples(1000, 33);
      ps->set_rapid_block(1);
      ps->set_trigger_once(true);

      auto sink = blocks::vector_sink_f::make(1);
      auto errsink = blocks::null_sink::make(sizeof(float));

      // connect and run
      top->connect(ps, 0, sink, 0);
      top->connect(ps, 1, errsink, 0);
      top->run();

      auto data = sink->data();
      CPPUNIT_ASSERT_EQUAL(size_t{1033}, data.size());

      // run once more, note configuration is applied at start
      sink->reset();
      ps->set_samples(800, 200);

      top->run();

      data = sink->data();
      CPPUNIT_ASSERT_EQUAL(size_t{1000}, data.size());

      // check actual sample rate
      auto actual_samp_rate = ps->get_samp_rate();
      CPPUNIT_ASSERT_DOUBLES_EQUAL(10000.0, actual_samp_rate, 0.0001);
    }

    void
    qa_picoscope_4000a::rapid_block_channels()
    {
      auto top = gr::make_top_block("channels");

      auto ps = picoscope_4000a::make("", true);

      ps->set_aichan("A", true, 5.0, false);
      ps->set_aichan("B", true, 5.0, false);
      ps->set_aichan("C", true, 5.0, false);
      ps->set_aichan("D", true, 5.0, false);

      ps->set_samp_rate(10000.0);
      ps->set_samples(1000, 50);
      ps->set_rapid_block(1);
      ps->set_trigger_once(true);

      auto sinkA = blocks::vector_sink_f::make(1);
      auto sinkB = blocks::vector_sink_f::make(1);
      auto sinkC = blocks::vector_sink_f::make(1);
      auto sinkD = blocks::vector_sink_f::make(1);
      auto errsinkA = blocks::null_sink::make(sizeof(float));
      auto errsinkB = blocks::null_sink::make(sizeof(float));
      auto errsinkC = blocks::null_sink::make(sizeof(float));
      auto errsinkD = blocks::null_sink::make(sizeof(float));

      // connect and run
      top->connect(ps, 0, sinkA, 0); top->connect(ps, 1, errsinkA, 0);
      top->connect(ps, 2, sinkB, 0); top->connect(ps, 3, errsinkB, 0);
      top->connect(ps, 4, sinkC, 0); top->connect(ps, 5, errsinkC, 0);
      top->connect(ps, 6, sinkD, 0); top->connect(ps, 7, errsinkD, 0);

      top->run();

      CPPUNIT_ASSERT_EQUAL(1050, (int)sinkA->data().size());
      CPPUNIT_ASSERT_EQUAL(1050, (int)sinkB->data().size());
      CPPUNIT_ASSERT_EQUAL(1050, (int)sinkC->data().size());
      CPPUNIT_ASSERT_EQUAL(1050, (int)sinkD->data().size());
    }

    void
    qa_picoscope_4000a::rapid_block_continuous()
    {
      auto top = gr::make_top_block("continuous");

      auto ps = picoscope_4000a::make("", true);
      ps->set_aichan("A", true, 5.0, false);
      ps->set_samp_rate(10000.0); // 10kHz
      ps->set_samples(1000, 0);
      ps->set_rapid_block(1);

      auto sink = blocks::vector_sink_f::make(1);
      auto errsink = blocks::null_sink::make(sizeof(float));

      // connect and run
      top->connect(ps, 0, sink, 0);
      top->connect(ps, 1, errsink, 0);

      // We explicitly open unit because it takes quite some time
      // and we don't want to time this part
      ps->initialize();

      top->start();
      sleep(1);
      top->stop();
      top->wait();

      // We should be able to acquire around 10k samples now but since
      // sleep can be imprecise and we want stable tests we just check if
      // we've got a good number of samples
      auto samples = sink->data().size();
      CPPUNIT_ASSERT_MESSAGE("actual samples: " + std::to_string(samples),
              samples > 3000 && samples < 15000);
    }

    void
    qa_picoscope_4000a::rapid_block_downsampling_basics()
    {
      auto top = gr::make_top_block("downsampling");

      auto ps = picoscope_4000a::make("", true);
      ps->set_aichan("A", true, 5.0, false);
      ps->set_samp_rate(10000.0);
      ps->set_samples(1000, 200);
      ps->set_downsampling(downsampling_mode_t::DOWNSAMPLING_MODE_DECIMATE, 4);
      ps->set_rapid_block(1);
      ps->set_trigger_once(true);

      auto sink = blocks::vector_sink_f::make(1);
      auto errsink = blocks::null_sink::make(sizeof(float));

      // connect and run
      top->connect(ps, 0, sink, 0);
      top->connect(ps, 1, errsink, 0);
      top->run();

      auto data = sink->data();
      CPPUNIT_ASSERT_EQUAL(300, (int)data.size());
    }

    void
    qa_picoscope_4000a::run_rapid_block_downsampling(downsampling_mode_t mode)
    {
        auto top = gr::make_top_block("channels");

        auto ps = picoscope_4000a::make("", true);

        ps->set_aichan("A", true, 5.0, false);
        ps->set_aichan("B", true, 5.0, false);
        ps->set_aichan("C", true, 5.0, false);
        ps->set_aichan("D", true, 5.0, false);

        ps->set_samp_rate(10000.0);
        ps->set_samples(10000, 1000);
        ps->set_rapid_block(1);
        ps->set_trigger_once(true);

        ps->set_downsampling(mode, 10);

        auto sinkA = blocks::vector_sink_f::make(1);
        auto sinkB = blocks::vector_sink_f::make(1);
        auto sinkC = blocks::vector_sink_f::make(1);
        auto sinkD = blocks::vector_sink_f::make(1);
        auto errsinkA = blocks::vector_sink_f::make(1);
        auto errsinkB = blocks::vector_sink_f::make(1);
        auto errsinkC = blocks::vector_sink_f::make(1);
        auto errsinkD = blocks::vector_sink_f::make(1);


        // connect and run
        top->connect(ps, 0, sinkA, 0); top->connect(ps, 1, errsinkA, 0);
        top->connect(ps, 2, sinkB, 0); top->connect(ps, 3, errsinkB, 0);
        top->connect(ps, 4, sinkC, 0); top->connect(ps, 5, errsinkC, 0);
        top->connect(ps, 6, sinkD, 0); top->connect(ps, 7, errsinkD, 0);

        top->run();

        CPPUNIT_ASSERT_EQUAL(1100, (int)sinkA->data().size());
        CPPUNIT_ASSERT_EQUAL(1100, (int)sinkB->data().size());
        CPPUNIT_ASSERT_EQUAL(1100, (int)sinkC->data().size());
        CPPUNIT_ASSERT_EQUAL(1100, (int)sinkD->data().size());

        CPPUNIT_ASSERT_EQUAL(1100, (int)errsinkA->data().size());
        CPPUNIT_ASSERT_EQUAL(1100, (int)errsinkB->data().size());
        CPPUNIT_ASSERT_EQUAL(1100, (int)errsinkC->data().size());
        CPPUNIT_ASSERT_EQUAL(1100, (int)errsinkD->data().size());
    }

    void
    qa_picoscope_4000a::rapid_block_downsampling()
    {
      run_rapid_block_downsampling(DOWNSAMPLING_MODE_AVERAGE);
      run_rapid_block_downsampling(DOWNSAMPLING_MODE_MIN_MAX_AGG);
      run_rapid_block_downsampling(DOWNSAMPLING_MODE_DECIMATE);
    }

    void
    qa_picoscope_4000a::rapid_block_tags()
    {
      auto top = gr::make_top_block("tags");

      auto samp_rate = 10000.0;
      auto ps = picoscope_4000a::make("", true);
      ps->set_aichan("A", true, 5.0, false);
      ps->set_samp_rate(samp_rate);
      ps->set_samples(1000, 200);
      ps->set_rapid_block(1);
      ps->set_trigger_once(true);

      auto sink = blocks::vector_sink_f::make(1);
      auto errsink = blocks::vector_sink_f::make(1);

      // connect and run
      top->connect(ps, 0, sink, 0);
      top->connect(ps, 1, errsink, 0);
      top->run();

      auto data_tags = sink->tags();
      CPPUNIT_ASSERT_EQUAL(3, (int)data_tags.size());

      for (auto &tag: data_tags) {
        auto key = pmt::symbol_to_string(tag.key);
        CPPUNIT_ASSERT(key == acq_info_tag_name || key == timebase_info_tag_name || key == trigger_tag_name);

        if (key == trigger_tag_name) {
            auto triggered_data = decode_trigger_tag(tag);
            CPPUNIT_ASSERT_EQUAL(200, (int)triggered_data.pre_trigger_samples);
            CPPUNIT_ASSERT_EQUAL(800, (int)triggered_data.post_trigger_samples);
            CPPUNIT_ASSERT_EQUAL(uint32_t{0}, triggered_data.status);
        }
        else if (key == timebase_info_tag_name) {
          auto timebase = decode_timebase_info_tag(tag);
          CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0/samp_rate, timebase, 0.0000001);
        }
        else {
          CPPUNIT_ASSERT_EQUAL(static_cast<uint64_t>(200), tag.offset);
        }
      }
    }

    void
    qa_picoscope_4000a::rapid_block_trigger()
    {
      auto top = gr::make_top_block("tags");

      auto samp_rate = 10000.0;
      auto ps = picoscope_4000a::make("", true);
      ps->set_aichan("A", true, 5.0, false);
      ps->set_diport("port0", true, 1.5);
      ps->set_samp_rate(samp_rate);
      ps->set_samples(1000, 200);
      ps->set_rapid_block(1);
      ps->set_trigger_once(true);
      ps->set_aichan_trigger("A", trigger_direction_t::TRIGGER_DIRECTION_RISING, 1.5);

      auto sink = blocks::vector_sink_f::make(1);
      auto errsink = blocks::vector_sink_f::make(1);

      auto bsink = blocks::null_sink::make(sizeof(float));
      auto csink = blocks::null_sink::make(sizeof(float));
      auto dsink = blocks::null_sink::make(sizeof(float));
      auto berrsink = blocks::null_sink::make(sizeof(float));
      auto cerrsink = blocks::null_sink::make(sizeof(float));
      auto derrsink = blocks::null_sink::make(sizeof(float));

      auto port0 = blocks::vector_sink_b::make(1);

      // connect and run
      top->connect(ps, 0, sink, 0);
      top->connect(ps, 1, errsink, 0);
      top->connect(ps, 2, bsink, 0);
      top->connect(ps, 3, berrsink, 0);
      top->connect(ps, 4, csink, 0);
      top->connect(ps, 5, cerrsink, 0);
      top->connect(ps, 6, dsink, 0);
      top->connect(ps, 7, derrsink, 0);
      top->connect(ps, 8, port0, 0);
      top->run();

      auto data = sink->data();
      CPPUNIT_ASSERT_EQUAL(1200, (int)data.size());
    }

    void
    qa_picoscope_4000a::streaming_basics()
    {
      auto top = gr::make_top_block("streaming_basic");
      auto ps = picoscope_4000a::make("", true);

      ps->set_aichan("A", true, 5.0, false);
      ps->set_samp_rate(10000.0);
      ps->set_buffer_size(100000);
      ps->set_streaming(0.00001);


      auto sink = blocks::vector_sink_f::make(1);
      auto errsink = blocks::null_sink::make(sizeof(float));

      // connect and run
      top->connect(ps, 0, sink, 0);
      top->connect(ps, 1, errsink, 0);

      // Explicitly open unit because it takes quite some time
      ps->initialize();

      top->start();
      sleep(2);
      top->stop();
      top->wait();

      auto data = sink->data();
      CPPUNIT_ASSERT(data.size() <= 20000 && data.size() >= 5000);

      //ps->initialize();

      sink->reset();
      top->start();
      sleep(2);
      top->stop();
      top->wait();

      data = sink->data();
      CPPUNIT_ASSERT(data.size() <= 20000 && data.size() >= 5000);

    }

  } /* namespace digitizers */
} /* namespace gr */

