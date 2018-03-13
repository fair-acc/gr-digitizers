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

#include <gnuradio/attributes.h>
#include <cppunit/TestAssert.h>
#include "qa_digitizer_block.h"
#include <digitizers/tags.h>
#include <gnuradio/top_block.h>
#include <gnuradio/blocks/vector_sink_f.h>
#include <digitizers/simulation_source.h>
#include <thread>
#include <chrono>

#include "utils.h"

namespace gr {
  namespace digitizers {
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
      auto top = gr::make_top_block("basics");
      int samples = 1000;
      int presamples = 50;
      fill_data(samples, presamples);

      auto source = gr::digitizers::simulation_source::make();
      source->set_auto_arm(true);
      source->set_samp_rate(10000.0);
      source->set_samples(samples, presamples);
      source->set_data(d_cha_vec, d_chb_vec, d_port_vec, 0); //sizes should match sum of sample and presample count
      source->set_aichan("A", true, 5.0, false);
      source->set_rapid_block(1);
      source->set_trigger_once(true);

      auto sink_sig = blocks::vector_sink_f::make(1);
      auto sink_err = blocks::vector_sink_f::make(1);

      top->connect(source, 0, sink_sig, 0);
      top->connect(source, 1, sink_err, 0);
      top->run();

      auto data = sink_sig->data();
      CPPUNIT_ASSERT_EQUAL(samples + presamples, (int)data.size());
    }

    void
    qa_digitizer_block::rapid_block_correct_tags()
    {
      auto top = gr::make_top_block("correct tags");
      int samples = 1000;
      int presamples = 50;
      fill_data(samples, presamples);

      auto source = gr::digitizers::simulation_source::make();
      source->set_auto_arm(true);
      source->set_samp_rate(10000.0);
      source->set_samples(samples, presamples);
      source->set_data(d_cha_vec, d_chb_vec, d_port_vec, 0); //sizes should match sum of sample and presample count
      source->set_aichan("A", true, 5.0, false);
      source->set_aichan_trigger("A", trigger_direction_t::TRIGGER_DIRECTION_RISING, 1.5);
      source->set_rapid_block(1);
      source->set_trigger_once(true);

      auto sink_sig = blocks::vector_sink_f::make(1);
      auto sink_err = blocks::vector_sink_f::make(1);

      top->connect(source, 0, sink_sig, 0);
      top->connect(source, 1, sink_err, 0);
      top->run();

      auto data_tags = sink_sig->tags();

      for (auto &tag: data_tags) {
        auto key = pmt::symbol_to_string(tag.key);
        CPPUNIT_ASSERT(key == "acq_info" || key == "timebase_info");

        if (key == "acq_info") {
          auto triggered_data = decode_acq_info_tag(tag);
          CPPUNIT_ASSERT_EQUAL(presamples, (int)triggered_data.pre_samples);
          CPPUNIT_ASSERT_EQUAL(samples, (int)triggered_data.samples);
          CPPUNIT_ASSERT_EQUAL(uint32_t{0}, triggered_data.status);
        }
        else {
          auto timebase = decode_timebase_info_tag(tag);
          CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0/10000.0, timebase, 0.0000001);
        }
      }
    }

    void
    qa_digitizer_block::streaming_basics()
    {
      auto top = gr::make_top_block("basics");
      int samples = 1001;
      int presamples = 50;
      fill_data(samples, presamples);

      auto source = gr::digitizers::simulation_source::make();
      source->set_auto_arm(true);
      source->set_samp_rate(10000.0);
      source->set_buffer_size(samples + presamples);
      source->set_samples(samples, presamples);
      source->set_data(d_cha_vec, d_chb_vec, d_port_vec, 0); //sizes should match sum of sample and presample count
      source->set_aichan("A", true, 5.0, false);
      source->set_streaming(0.0001);
      auto sink_sig = blocks::vector_sink_f::make(1);
      auto sink_err = blocks::vector_sink_f::make(1);

      top->connect(source, 0, sink_sig, 0);
      top->connect(source, 1, sink_err, 0);
      top->start();
      std::this_thread::sleep_for(std::chrono::microseconds(1000));
      top->stop();

      auto data = sink_sig->data();
      CPPUNIT_ASSERT(data.size() != 0);
    }

    void
    qa_digitizer_block::streaming_correct_tags()
    {
      auto top = gr::make_top_block("basics");
      int samples = 1001;
      int presamples = 50;
      fill_data(samples, presamples);

      auto source = gr::digitizers::simulation_source::make();
      source->set_auto_arm(true);
      source->set_samp_rate(10000.0);
      source->set_buffer_size(samples + presamples);
      source->set_samples(samples, presamples);
      source->set_data(d_cha_vec, d_chb_vec, d_port_vec, 0); //sizes should match sum of sample and presample count
      source->set_aichan("A", true, 5.0, false);
      source->set_aichan_trigger("A", trigger_direction_t::TRIGGER_DIRECTION_RISING, 1.5);
      source->set_streaming(0.0001);
      auto sink_sig = blocks::vector_sink_f::make(1);
      auto sink_err = blocks::vector_sink_f::make(1);

      top->connect(source, 0, sink_sig, 0);
      top->connect(source, 1, sink_err, 0);
      top->start();
      std::this_thread::sleep_for(std::chrono::microseconds(1000));
      top->stop();

      auto data_tags = sink_sig->tags();

      for (auto &tag: data_tags) {
        auto key = pmt::symbol_to_string(tag.key);
        CPPUNIT_ASSERT(key == "timebase_info" || key == "acq_info");

        if (key == "acq_info") {
          auto acq_info = decode_acq_info_tag(tag);
          if (acq_info.triggered_data) {
            CPPUNIT_ASSERT_EQUAL(presamples, (int)acq_info.pre_samples);
            CPPUNIT_ASSERT_EQUAL(samples, (int)acq_info.samples);
          }
          else{
            CPPUNIT_ASSERT_EQUAL(0, static_cast<int>(acq_info.offset)%samples);
          }
        }
        else {
          double timebase = decode_timebase_info_tag(tag);
          CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0/10000.0, timebase, 0.0000001);
        }
      }
    }


    void
    qa_digitizer_block::check_for_errors()
    {
      auto top = gr::make_top_block("basics");
      int samples = 1000;
      int presamples = 50;
      fill_data(samples, presamples);

      auto source = gr::digitizers::simulation_source::make();
      source->set_auto_arm(true);
      source->set_samp_rate(10000.0);
      source->set_samples(samples, presamples);
      source->set_data(d_cha_vec, d_chb_vec, d_port_vec, 0); //sizes should match sum of sample and presample count
      source->set_aichan("A", true, 5.0, false);
      source->set_rapid_block(1000);
      source->set_trigger_once(true);

      auto sink_sig = blocks::vector_sink_f::make(1);
      auto sink_err = blocks::vector_sink_f::make(1);

      top->connect(source, 0, sink_sig, 0);
      top->connect(source, 1, sink_err, 0);
      top->start();
      source->close();
      top->stop();

      auto errors = source->get_errors();
      CPPUNIT_ASSERT(size_t(0) < errors.size());
    }

  }
}
