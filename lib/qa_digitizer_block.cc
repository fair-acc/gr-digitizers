/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#include <gnuradio/attributes.h>
#include <cppunit/TestAssert.h>
#include "qa_digitizer_block.h"
#include <digitizers/tags.h>
#include <gnuradio/top_block.h>
#include <gnuradio/blocks/vector_sink.h>
#include <gnuradio/blocks/throttle.h>
#include <digitizers/simulation_source.h>
#include <thread>
#include <chrono>

#include "utils.h"
#include "qa_common.h"

namespace gr {
  namespace digitizers {

    struct simulated_test_flowgraph_t {
      gr::top_block_sptr top;
      gr::digitizers::simulation_source::sptr source;
      gr::blocks::vector_sink_f::sptr sink_sig_a;
      gr::blocks::vector_sink_f::sptr sink_err_a;
      gr::blocks::vector_sink_f::sptr sink_sig_b;
      gr::blocks::vector_sink_f::sptr sink_err_b;
      gr::blocks::vector_sink_b::sptr sink_port;
      gr::blocks::throttle::sptr throttle;
    };

    static simulated_test_flowgraph_t
    make_test_flowgraph(float samp_rate=100000)
    {
      simulated_test_flowgraph_t fg;

      fg.top = gr::make_top_block("test");
      fg.source = gr::digitizers::simulation_source::make();

      fg.sink_sig_a = blocks::vector_sink_f::make(1);
      fg.sink_err_a = blocks::vector_sink_f::make(1);
      fg.sink_sig_b = blocks::vector_sink_f::make(1);
      fg.sink_err_b = blocks::vector_sink_f::make(1);
      fg.sink_port =  blocks::vector_sink_b::make(1);

      fg.throttle = gr::blocks::throttle::make(sizeof(float), samp_rate, true);

      fg.top->connect(fg.source, 0, fg.throttle, 0);
      fg.top->connect(fg.throttle, 0, fg.sink_sig_a, 0);
      fg.top->connect(fg.source, 1, fg.sink_err_a, 0);
      fg.top->connect(fg.source, 2, fg.sink_sig_b, 0);
      fg.top->connect(fg.source, 3, fg.sink_err_b, 0);
      fg.top->connect(fg.source, 4, fg.sink_port, 0);

      fg.source->set_auto_arm(true);
      fg.source->set_samp_rate(samp_rate);

      return fg;
    }


    void
    qa_digitizer_block::fill_data(unsigned samples, unsigned presamples)
    {
      float add = 0;
      for(unsigned i = 0; i < samples+presamples; i++) {
        if(i == presamples) { add = 2.0; }
        d_cha_vec.push_back(add + 2.0*i/1033.0);
        d_chb_vec.push_back(2.0*i/1033.0);
        d_port_vec.push_back(i%256);
      }
    }

    void
    qa_digitizer_block::rapid_block_basics()
    {
      int samples = 1000;
      int presamples = 50;
      fill_data(samples, presamples);

      auto fg = make_test_flowgraph();
      auto source = fg.source;


      fg.source->set_samples(presamples, samples);
      fg.source->set_data(d_cha_vec, d_chb_vec, d_port_vec);
      fg.source->set_rapid_block(1);
      fg.source->set_trigger_once(true);

      fg.top->run();

      auto dataa = fg.sink_sig_a->data();
      CPPUNIT_ASSERT_EQUAL(samples + presamples, (int)dataa.size());
      ASSERT_VECTOR_EQUAL(d_cha_vec.begin(), d_cha_vec.end(), dataa.begin());

      auto datab = fg.sink_sig_b->data();
      CPPUNIT_ASSERT_EQUAL(samples + presamples, (int)datab.size());
      ASSERT_VECTOR_EQUAL(d_chb_vec.begin(), d_chb_vec.end(), datab.begin());

      auto datap = fg.sink_port->data();
      CPPUNIT_ASSERT_EQUAL(samples + presamples, (int)datap.size());
      ASSERT_VECTOR_EQUAL(d_port_vec.begin(), d_port_vec.end(), reinterpret_cast<uint8_t *>(&datap[0]));
    }

    void
    qa_digitizer_block::rapid_block_correct_tags()
    {
      int samples = 2000;
      int presamples = 200;
      fill_data(samples, presamples);

      auto fg = make_test_flowgraph(10000.0);
      auto source = fg.source;

      fg.source->set_samples(presamples, samples);
      fg.source->set_data(d_cha_vec, d_chb_vec, d_port_vec);
      fg.source->set_rapid_block(1);
      fg.source->set_trigger_once(true);

      fg.top->run();

      auto data_tags = fg.sink_sig_a->tags();

      for (auto &tag: data_tags) {
        auto key = pmt::symbol_to_string(tag.key);
        CPPUNIT_ASSERT(key == acq_info_tag_name
                || key == timebase_info_tag_name
                || key == trigger_tag_name);

        if (key == trigger_tag_name)
        {
          auto triggered_data = decode_trigger_tag(tag);
          CPPUNIT_ASSERT_EQUAL(uint32_t{0}, triggered_data.status);
        }
        else if (key == timebase_info_tag_name)
        {
          auto timebase = decode_timebase_info_tag(tag);
          CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0/10000.0, timebase, 0.0000001);
        }
        else if (key == acq_info_tag_name)
        {
          CPPUNIT_ASSERT_EQUAL(static_cast<uint64_t>(presamples), tag.offset);
        }
        else
        {
            CPPUNIT_FAIL("unknown tag. key: " + key );
        }
      }
    }

    void
    qa_digitizer_block::streaming_basics()
    {
      int samples = 2000;
      int presamples = 200;
      int buffer_size = samples + presamples;

      fill_data(samples, presamples);

      auto fg = make_test_flowgraph();
      auto source = fg.source;

      fg.source->set_buffer_size(buffer_size);
      fg.source->set_data(d_cha_vec, d_chb_vec, d_port_vec);
      fg.source->set_streaming(0.0001);

      fg.top->start();
      std::this_thread::sleep_for(std::chrono::microseconds(2000));
      fg.top->stop();
      fg.top->wait();

      auto dataa = fg.sink_sig_a->data();
      CPPUNIT_ASSERT(dataa.size() != 0);

      auto size = std::min(dataa.size(), d_cha_vec.size());
      ASSERT_VECTOR_EQUAL(d_cha_vec.begin(), d_cha_vec.begin() + size, dataa.begin());

      auto datab = fg.sink_sig_b->data();
      CPPUNIT_ASSERT(datab.size() != 0);
      size = std::min(datab.size(), d_chb_vec.size());
      ASSERT_VECTOR_EQUAL(d_chb_vec.begin(), d_chb_vec.begin() + size, datab.begin());

      auto datap = fg.sink_port->data();
      CPPUNIT_ASSERT(datap.size() != 0);
      size = std::min(datap.size(), d_port_vec.size());
      ASSERT_VECTOR_EQUAL(d_port_vec.begin(), d_port_vec.begin() + size, reinterpret_cast<uint8_t *>(&datap[0]));
    }

    void
    qa_digitizer_block::streaming_correct_tags()
    {
      int samples = 2000;
      int presamples = 200;
      int buffer_size = samples + presamples;

      fill_data(samples, presamples);

      auto fg = make_test_flowgraph(10000.0);
      auto source = fg.source;

      fg.source->set_samp_rate(10000.0);
      fg.source->set_buffer_size(buffer_size);
      fg.source->set_data(d_cha_vec, d_chb_vec, d_port_vec);
      fg.source->set_streaming(0.0001);

      fg.top->start();
      std::this_thread::sleep_for(std::chrono::microseconds(500));
      fg.top->stop();
      fg.top->wait();

      auto data_tags = fg.sink_sig_a->tags();

      for (auto &tag: data_tags) {
        auto key = pmt::symbol_to_string(tag.key);
        CPPUNIT_ASSERT(key == timebase_info_tag_name || key == acq_info_tag_name);

        if (key == acq_info_tag_name) {
          CPPUNIT_ASSERT_EQUAL(0, static_cast<int>(tag.offset) % buffer_size);
        }
        else {
          double timebase = decode_timebase_info_tag(tag);
          CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0/10000.0, timebase, 0.0000001);
        }
      }
    }
  }
}
