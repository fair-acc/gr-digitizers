/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
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
        tag_debug = gr::blocks::tag_debug::make(sizeof(float) * vlen, "tag_debug", acq_info_tag_name);
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

    static const float SAMP_RATE_1KHZ = 1000.0;

    void
    qa_freq_sink_f::test_metadata()
    {
      std::string name = "My Name";
      auto sink = freq_sink_f::make(name, SAMP_RATE_1KHZ, 128, 1, 100, FREQ_SINK_MODE_STREAMING);

      CPPUNIT_ASSERT_EQUAL(FREQ_SINK_MODE_STREAMING, sink->get_sink_mode());
      CPPUNIT_ASSERT_EQUAL(size_t{128}, sink->get_nbins());
      CPPUNIT_ASSERT_EQUAL(size_t{1}, sink->get_nmeasurements());
      CPPUNIT_ASSERT_EQUAL(SAMP_RATE_1KHZ, sink->get_sample_rate());

      auto metadata = sink->get_metadata();
      CPPUNIT_ASSERT_EQUAL(name, metadata.name);
      CPPUNIT_ASSERT_EQUAL(std::string(""), metadata.unit);
    }

    void
    qa_freq_sink_f::test_sink_no_tags()
    {
      auto nbins = 4096, nmeasurements = 10;
      auto samples = nbins * nmeasurements;
      auto sink = freq_sink_f::make("test", SAMP_RATE_1KHZ, nbins,
              nmeasurements, 2, FREQ_SINK_MODE_STREAMING);

      freq_test_flowgraph_t fg(sink, std::vector<tag_t>{}, nbins, nmeasurements);
      fg.run();

      std::vector<spectra_measurement_t> minfo(nmeasurements);
      std::vector<float> freq(samples), mag(samples), phase(samples);
      auto retval = sink->get_measurements(nmeasurements, &minfo[0], &freq[0], &mag[0], &phase[0]);

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
        CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0f / SAMP_RATE_1KHZ, m.timebase, 1e-5);
        //CPPUNIT_ASSERT_EQUAL((int64_t)-1, m.trigger_timestamp); FIXME: What is expected ?
      }

      retval = sink->get_measurements(nmeasurements, &minfo[0], &freq[0], &mag[0], &phase[0]);

      CPPUNIT_ASSERT_EQUAL(0, (int)retval);
    }

    void
    qa_freq_sink_f::test_sink_tags()
    {
      auto nbins = 4096, nmeasurements = 8;
      auto samples = nbins * nmeasurements;
      auto sink = freq_sink_f::make("test", SAMP_RATE_1KHZ, nbins, nmeasurements, 2, FREQ_SINK_MODE_STREAMING);

      acq_info_t info{};
      info.timestamp = 987654321;
      info.timebase = 0.00001;
      info.status = 0xFE;

      freq_test_flowgraph_t fg(sink, std::vector<tag_t>{ make_acq_info_tag(info,0) },
              nbins, nmeasurements);
      fg.run();

      CPPUNIT_ASSERT_EQUAL(1, fg.tag_debug->num_tags());

      std::vector<spectra_measurement_t> minfo(nmeasurements);
      std::vector<float> freq(samples), mag(samples), phase(samples);
      auto retval = sink->get_measurements(nmeasurements, &minfo[0], &freq[0], &mag[0], &phase[0]);

      CPPUNIT_ASSERT_EQUAL(nmeasurements, (int)retval);
      CPPUNIT_ASSERT_EQUAL(nmeasurements, (int)minfo.size());
      CPPUNIT_ASSERT_EQUAL(samples, (int)freq.size());
      CPPUNIT_ASSERT_EQUAL(samples, (int)mag.size());
      CPPUNIT_ASSERT_EQUAL(samples, (int)phase.size());

      ASSERT_VECTOR_EQUAL(fg.freq.begin(), fg.freq.end(), freq.begin());
      ASSERT_VECTOR_EQUAL(fg.magnitude.begin(), fg.magnitude.end(), mag.begin());
      ASSERT_VECTOR_EQUAL(fg.phase.begin(), fg.phase.end(), phase.begin());

      // Timestamps available for the first measurement only
      auto first = minfo.front();
      //CPPUNIT_ASSERT_EQUAL(info.status, first.status); FIXME: Why an error is expected ?
      //CPPUNIT_ASSERT_EQUAL(info.timestamp, first.timestamp); FIXME: Test Fails since tag restructurization
      CPPUNIT_ASSERT_EQUAL((uint32_t)nbins, first.number_of_bins);

      // For other measurements no timestamp should be available
      //auto ns_per_sample = (int64_t)(1.0f / SAMP_RATE_1KHZ * 1000000000.0f);

      for (auto i = 1; i < nmeasurements; i++) {
        auto m = minfo.at(i);
        CPPUNIT_ASSERT_EQUAL((uint32_t)nbins, m.number_of_bins);
     //   int64_t expected_timestamp = info.timestamp + ((int64_t)i * ns_per_sample);
     //   CPPUNIT_ASSERT_DOUBLES_EQUAL((double)expected_timestamp, (double)m.timestamp, 10.0);  FIXME: Test Fails since tag restructurization
     //   CPPUNIT_ASSERT_EQUAL(info.status, m.status);
      }
    }

    void
    qa_freq_sink_f::test_sink_callback()
    {
      auto nbins = 4096, nmeasurements = 10;
      auto samples = nbins * nmeasurements;
      auto sink = freq_sink_f::make("test", SAMP_RATE_1KHZ, nbins, nmeasurements / 2, 2, FREQ_SINK_MODE_STREAMING);

      // expect user callback to be called twice
      std::atomic<unsigned short> counter {};
      data_available_event_t local_event {};
      auto callback = new std::function<void(const data_available_event_t *)>(
      [&counter, &local_event] (const data_available_event_t *event)
      {
          local_event = *event;
          counter++;
      });

      sink->set_callback(&invoke_function, static_cast<void *>(callback));

      // in order to test trigger timestamp passed to the callback
      acq_info_t info{};
      info.timestamp = 987654321;

      freq_test_flowgraph_t fg(sink, std::vector<tag_t>{ make_acq_info_tag(info,0) }, nbins, nmeasurements);
      fg.top->run();

      // test metadata provided to the callback
      CPPUNIT_ASSERT_EQUAL(std::string("test"), local_event.signal_name);

      std::vector<spectra_measurement_t> minfo(nmeasurements);
      std::vector<float> freq(samples), mag(samples), phase(samples);
      auto retval = sink->get_measurements(nmeasurements, &minfo[0], &freq[0], &mag[0], &phase[0]);

      CPPUNIT_ASSERT_EQUAL(nmeasurements / 2, (int)retval);
      ASSERT_VECTOR_EQUAL(fg.freq.begin(), fg.freq.begin() + (nmeasurements / 2 * nbins), freq.begin());
    }

  } /* namespace digitizers */
} /* namespace gr */

