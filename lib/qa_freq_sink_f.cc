/* -*- c++ -*- */
/* 
 * Copyright 2018 <+YOU OR YOUR COMPANY+>.
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
#include "qa_freq_sink_f.h"
#include "qa_common.h"
#include <digitizers/freq_sink_f.h>
#include <digitizers/tags.h>

#include <gnuradio/top_block.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/blocks/tag_debug.h>
#include <functional>
#include <atomic>

namespace gr {
  namespace digitizers {

    struct freq_test_flowgraph_t
    {
      gr::top_block_sptr top;
      gr::blocks::vector_source_f::sptr freq_src;
      gr::blocks::vector_source_f::sptr magnitude_src;
      gr::blocks::vector_source_f::sptr phase_src;
      gr::blocks::tag_debug::sptr tag_debug;
      gr::digitizers::freq_sink_f::sptr freq_sink;
      std::vector<float> freq;
      std::vector<float> magnitude;
      std::vector<float> phase;

      freq_test_flowgraph_t(freq_sink_f::sptr &sink,
              const std::vector<tag_t> &tags, int vlen, int nvectors)
      {
        auto nsamples = nvectors * vlen;

        // generated some data
        freq = make_test_data(nsamples);
        magnitude = make_test_data(nsamples, 0.2);
        phase = make_test_data(nsamples, 0.4);

        top = gr::make_top_block("test");
        freq_src = gr::blocks::vector_source_f::make(freq, false, vlen);
        magnitude_src = gr::blocks::vector_source_f::make(magnitude, false, vlen, tags);
        phase_src = gr::blocks::vector_source_f::make(phase, false, vlen);
        tag_debug = gr::blocks::tag_debug::make(sizeof(float) * vlen, "tag_debug", "acq_info");
        tag_debug->set_display(false);

        freq_sink = sink;

        top->connect(magnitude_src, 0, tag_debug, 0);
        top->connect(magnitude_src, 0, freq_sink, 0);
        top->connect(phase_src, 0, freq_sink, 1);
        top->connect(freq_src, 0, freq_sink, 2);
      }

      void
      run()
      {
        top->run();
      }
    };

    static void invoke_function(const data_available_event_t *event, void *ptr) {
        (*static_cast<std::function<void(const data_available_event_t *)>*>(ptr))(event);
    }

    void
    qa_freq_sink_f::test_metadata()
    {
      std::string name = "My Name";
      std::string unit = "My Unit";
      auto sink = freq_sink_f::make(name, unit, 128, 1, FREQ_SINK_MODE_STREAMING);

      CPPUNIT_ASSERT_EQUAL(FREQ_SINK_MODE_STREAMING, sink->get_mode());
      CPPUNIT_ASSERT_EQUAL(128, sink->get_nbins());
      CPPUNIT_ASSERT_EQUAL(1, sink->get_nmeasurements());

      auto metadata = sink->get_metadata();
      CPPUNIT_ASSERT_EQUAL(name, metadata.name);
      CPPUNIT_ASSERT_EQUAL(unit, metadata.unit);
    }

    void
    qa_freq_sink_f::test_sink_no_tags()
    {
      auto nbins = 4096, nmeasurements = 10;
      auto samples = nbins * nmeasurements;
      auto sink = freq_sink_f::make("test", "", nbins, nmeasurements * 2, FREQ_SINK_MODE_STREAMING);

      freq_test_flowgraph_t fg(sink, std::vector<tag_t>{}, nbins, nmeasurements);
      fg.run();

      std::vector<spectra_measurement_t> minfo;
      std::vector<float> freq, mag, phase;
      auto retval = sink->get_measurements(nmeasurements, minfo, freq, mag, phase);

      CPPUNIT_ASSERT_EQUAL(nmeasurements, (int)retval);
      CPPUNIT_ASSERT_EQUAL(nmeasurements, (int)minfo.size());
      CPPUNIT_ASSERT_EQUAL(samples, (int)freq.size());
      CPPUNIT_ASSERT_EQUAL(samples, (int)mag.size());
      CPPUNIT_ASSERT_EQUAL(samples, (int)phase.size());

      ASSERT_VECTOR_EQUAL(fg.freq.begin(), fg.freq.end(), freq.begin());
      ASSERT_VECTOR_EQUAL(fg.magnitude.begin(), fg.magnitude.end(), mag.begin());
      ASSERT_VECTOR_EQUAL(fg.phase.begin(), fg.phase.end(), phase.begin());

      for (const auto &m : minfo) {
        CPPUNIT_ASSERT_EQUAL((uint32_t)nbins, m.number_of_bins);
        CPPUNIT_ASSERT_EQUAL((uint32_t)0, m.status);
        CPPUNIT_ASSERT_EQUAL((int64_t)-1, m.timestamp);
        CPPUNIT_ASSERT_EQUAL((int64_t)-1, m.trigger_timestamp);
      }

      retval = sink->get_measurements(nmeasurements, minfo, freq, mag, phase);

      CPPUNIT_ASSERT_EQUAL(0, (int)retval);
    }

    void
    qa_freq_sink_f::test_sink_tags()
    {
      auto nbins = 4096, nmeasurements = 8;
      auto samples = nbins * nmeasurements;
      auto sink = freq_sink_f::make("test", "", nbins, nmeasurements * 2, FREQ_SINK_MODE_STREAMING);

      acq_info_t info{};
      info.timestamp = 987654321;
      info.trigger_timestamp = 987654323;
      info.timebase = 0.00001;
      info.samples = samples;
      info.status = 0xFE;
      info.offset = 0;

      freq_test_flowgraph_t fg(sink, std::vector<tag_t>{ make_acq_info_tag(info) },
              nbins, nmeasurements);
      fg.run();

      CPPUNIT_ASSERT_EQUAL(1, fg.tag_debug->num_tags());

      std::vector<spectra_measurement_t> minfo;
      std::vector<float> freq, mag, phase;
      auto retval = sink->get_measurements(nmeasurements, minfo, freq, mag, phase);

      CPPUNIT_ASSERT_EQUAL(nmeasurements, (int)retval);
      CPPUNIT_ASSERT_EQUAL(nmeasurements, (int)minfo.size());
      CPPUNIT_ASSERT_EQUAL(samples, (int)freq.size());
      CPPUNIT_ASSERT_EQUAL(samples, (int)mag.size());
      CPPUNIT_ASSERT_EQUAL(samples, (int)phase.size());

      ASSERT_VECTOR_EQUAL(fg.freq.begin(), fg.freq.end(), freq.begin());
      ASSERT_VECTOR_EQUAL(fg.magnitude.begin(), fg.magnitude.end(), mag.begin());
      ASSERT_VECTOR_EQUAL(fg.phase.begin(), fg.phase.end(), phase.begin());

      for (auto i = 0; i < nmeasurements; i++) {
        auto m = minfo.at(i);
        CPPUNIT_ASSERT_EQUAL((uint32_t)nbins, m.number_of_bins);

        // status and trigger timestamp is the same for all the measurements
        CPPUNIT_ASSERT_EQUAL(info.status, m.status);
        CPPUNIT_ASSERT_EQUAL(info.trigger_timestamp, m.trigger_timestamp);

        // but timestamp of the first sample should change
        auto timestamp = info.timestamp + static_cast<int64_t>(i * nbins * info.timebase * 1000000000.0);
        CPPUNIT_ASSERT_DOUBLES_EQUAL((double)timestamp, (double)m.timestamp, 1.0);
      }
    }

    void
    qa_freq_sink_f::test_sink_callback()
    {
      auto nbins = 4096, nmeasurements = 10;
      auto sink = freq_sink_f::make("test", "", nbins, nmeasurements / 2, FREQ_SINK_MODE_SEQUENCE);

      // expect user callback to be called twice
      std::atomic<unsigned short> counter {};
      data_available_event_t local_event {};
      auto callback = new std::function<void(const data_available_event_t *)>(
      [&counter, &local_event] (const data_available_event_t *event)
      {
          local_event = *event;
          counter++;
      });

      sink->set_measurements_available_callback(&invoke_function, static_cast<void *>(callback));

      // in order to test trigger timestamp passed to the callback
      acq_info_t info{};
      info.timestamp = 987654321;
      info.trigger_timestamp = 987654323;
      info.offset = 0;

      freq_test_flowgraph_t fg(sink, std::vector<tag_t>{ make_acq_info_tag(info) }, nbins, nmeasurements);
      fg.top->start();

      // wait for the first callback
      auto timeout = 0;
      while (timeout++ < 10000 && counter < 1) {
          usleep(1000);
      }
      CPPUNIT_ASSERT(timeout < 10000);

      // test metadata provided to the callback
      CPPUNIT_ASSERT_EQUAL(info.trigger_timestamp, local_event.trigger_timestamp);
      CPPUNIT_ASSERT_EQUAL(std::string("test"), local_event.signal_name);

      std::vector<spectra_measurement_t> minfo;
      std::vector<float> freq, mag, phase;
      auto retval = sink->get_measurements(nmeasurements, minfo, freq, mag, phase);

      CPPUNIT_ASSERT_EQUAL(nmeasurements / 2, (int)retval);
      ASSERT_VECTOR_EQUAL(fg.freq.begin(), fg.freq.begin() + (nmeasurements / 2 * nbins), freq.begin());

      // wait for the second callback
      timeout = 0;
      while (timeout++ < 10000 && counter < 2) {
          usleep(1000);
      }
      CPPUNIT_ASSERT(timeout < 10000);

      retval = sink->get_measurements(nmeasurements, minfo, freq, mag, phase);
      CPPUNIT_ASSERT_EQUAL(nmeasurements / 2, (int)retval);
      ASSERT_VECTOR_EQUAL(fg.freq.begin() + (nmeasurements / 2 * nbins),
              fg.freq.end(), freq.begin());

      fg.top->stop();
      fg.top->wait();
    }

    void
    qa_freq_sink_f::test_snapshot_mode()
    {
      auto nbins = 4096, nmeasurements = 10;
      auto sink = freq_sink_f::make("test", "", nbins, nmeasurements, FREQ_SINK_MODE_SNAPSHOT);

      // in order to test trigger timestamp passed to the callback
      acq_info_t info{};
      info.last_beam_in_timestamp = info.timestamp = 987654321;
      info.timebase = 0.000015;
      info.trigger_timestamp = -1; // not relevant
      info.offset = 0;

      // snapshot points
      std::vector<float> snapshots = {
          (float)(nbins * 0.1 * info.timebase),
          (float)(nbins * 3.0 * info.timebase),    // 3th measurement
          (float)(nbins * 3.1 * info.timebase)     // 4th measurement
      };

      sink->set_snapshot_points(snapshots);

      freq_test_flowgraph_t fg(sink, std::vector<tag_t>{ make_acq_info_tag(info) }, nbins, nmeasurements);
      fg.run();

      std::vector<spectra_measurement_t> minfo;
      std::vector<float> freq, mag, phase;
      auto retval = sink->get_measurements(nmeasurements, minfo, freq, mag, phase);

      CPPUNIT_ASSERT_EQUAL(3, (int)retval);
    }


    // TODO: jgolob trigger related tests missing

  } /* namespace digitizers */
} /* namespace gr */

