#include "qa_freq_sink.h"
#include "qa_common.h"

#include <digitizers/freq_sink.h>
#include <digitizers/tags.h>

#include <gnuradio/attributes.h>
#include <gnuradio/blocks/vector_source.h>
#include <gnuradio/flowgraph.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

#include <atomic>
#include <functional>

namespace gr::digitizers {

struct freq_test_flowgraph_t {
    gr::flowgraph::sptr               fg;
    gr::blocks::vector_source_f::sptr freq_src;
    gr::blocks::vector_source_f::sptr magnitude_src;
    gr::blocks::vector_source_f::sptr phase_src;
#ifdef PORT_DISALBED
    gr::blocks::tag_debug::sptr tag_debug
#endif
            gr::digitizers::freq_sink::sptr sink;
    std::vector<float>                      freq;
    std::vector<float>                      magnitude;
    std::vector<float>                      phase;

    freq_test_flowgraph_t(freq_sink::sptr freq_sink_,
            const std::vector<tag_t> &tags, std::size_t vlen, std::size_t nvectors) {
        auto nsamples = nvectors * vlen;

        // generated some data
        freq          = make_test_data(nsamples);
        magnitude     = make_test_data(nsamples, 0.2);
        phase         = make_test_data(nsamples, 0.4);

        fg            = gr::flowgraph::make("test");
        freq_src      = gr::blocks::vector_source_f::make({ freq, false, vlen });
        magnitude_src = gr::blocks::vector_source_f::make({ magnitude, false, vlen, tags });
        phase_src     = gr::blocks::vector_source_f::make({ phase, false, vlen });
#ifdef PORT_DISABLED
        tag_debug = gr::blocks::tag_debug::make({ sizeof(float) * vlen, "tag_debug", acq_info_tag_name });
        tag_debug->set_display(false);
#endif
        sink = freq_sink_;

#ifdef PORT_DISABLED
        fg->connect(magnitude_src, 0, tag_debug, 0);
#endif
        fg->connect(magnitude_src, 0, freq_sink_, 0);
        fg->connect(phase_src, 0, freq_sink_, 1);
        fg->connect(freq_src, 0, freq_sink_, 2);
    }

    void
    run() {
        fg->run();
    }
};

static void invoke_function(const data_available_event_t *event, void *ptr) {
    (*static_cast<std::function<void(const data_available_event_t *)> *>(ptr))(event);
}

static const float SAMP_RATE_1KHZ = 1000.0;

void               qa_freq_sink::test_metadata() {
                  std::string name = "My Name";
                  auto        sink = freq_sink::make({ name, SAMP_RATE_1KHZ, 128, 1, 100, freq_sink_mode_t::FREQ_SINK_MODE_STREAMING });

                  CPPUNIT_ASSERT_EQUAL(freq_sink_mode_t::FREQ_SINK_MODE_STREAMING, sink->mode());
                  CPPUNIT_ASSERT_EQUAL(size_t{ 128 }, sink->nbins());
                  CPPUNIT_ASSERT_EQUAL(size_t{ 1 }, sink->nmeasurements());
                  CPPUNIT_ASSERT_EQUAL(SAMP_RATE_1KHZ, sink->sample_rate());

                  auto metadata = sink->get_metadata();
                  CPPUNIT_ASSERT_EQUAL(name, metadata.name);
                  CPPUNIT_ASSERT_EQUAL(std::string(""), metadata.unit);
}

void qa_freq_sink::test_sink_no_tags() {
    std::size_t           nbins         = 4096;
    std::size_t           nmeasurements = 10;
    auto                  samples       = nbins * nmeasurements;
    auto                  sink          = freq_sink::make({ "test", SAMP_RATE_1KHZ, nbins,
                                      nmeasurements, 2, freq_sink_mode_t::FREQ_SINK_MODE_STREAMING });

    freq_test_flowgraph_t fg(sink, std::vector<tag_t>{}, nbins, nmeasurements);
    fg.run();

    const auto measurements = sink->get_measurements(nmeasurements);

    CPPUNIT_ASSERT_EQUAL(nmeasurements, measurements.nmeasurements());
    CPPUNIT_ASSERT_EQUAL(nmeasurements, measurements.metadata.size());
    CPPUNIT_ASSERT_EQUAL(samples, measurements.frequency.size());
    CPPUNIT_ASSERT_EQUAL(samples, measurements.magnitude.size());
    CPPUNIT_ASSERT_EQUAL(samples, measurements.phase.size());

    ASSERT_VECTOR_EQUAL(fg.freq.begin(), fg.freq.end(), measurements.frequency.begin());
    ASSERT_VECTOR_EQUAL(fg.magnitude.begin(), fg.magnitude.end(), measurements.magnitude.begin());
    ASSERT_VECTOR_EQUAL(fg.phase.begin(), fg.phase.end(), measurements.phase.begin());

    for (const auto &m : measurements.metadata) {
        CPPUNIT_ASSERT_EQUAL((uint32_t) nbins, m.number_of_bins);
        CPPUNIT_ASSERT_EQUAL((uint32_t) 0, m.status);
        CPPUNIT_ASSERT_EQUAL((int64_t) -1, m.timestamp);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0f / SAMP_RATE_1KHZ, m.timebase, 1e-5);
        // CPPUNIT_ASSERT_EQUAL((int64_t)-1, m.trigger_timestamp); FIXME: What is expected ?
    }

    const auto measurements2 = sink->get_measurements(nmeasurements);

    CPPUNIT_ASSERT_EQUAL(std::size_t{ 0 }, measurements2.nmeasurements());
}

void qa_freq_sink::test_sink_tags() {
    std::size_t nbins         = 4096;
    std::size_t nmeasurements = 8;
    auto        samples       = nbins * nmeasurements;
    auto        sink          = freq_sink::make({ "test", SAMP_RATE_1KHZ, nbins, nmeasurements, 2, freq_sink_mode_t::FREQ_SINK_MODE_STREAMING });

    acq_info_t  info{};
    info.timestamp = 987654321;
    info.timebase  = 0.00001;
    info.status    = 0xFE;

    freq_test_flowgraph_t fg(sink, std::vector<tag_t>{ make_acq_info_tag(info, 0) },
            nbins, nmeasurements);
    fg.run();

#ifdef PORT_DISABLED
    CPPUNIT_ASSERT_EQUAL(1, fg.tag_debug->num_tags());
#endif

    const auto measurements = sink->get_measurements(nmeasurements);

    CPPUNIT_ASSERT_EQUAL(nmeasurements, measurements.nmeasurements());
    CPPUNIT_ASSERT_EQUAL(nmeasurements, measurements.metadata.size());

    CPPUNIT_ASSERT_EQUAL(samples, measurements.frequency.size());
    CPPUNIT_ASSERT_EQUAL(samples, measurements.magnitude.size());
    CPPUNIT_ASSERT_EQUAL(samples, measurements.phase.size());

    ASSERT_VECTOR_EQUAL(fg.freq.begin(), fg.freq.end(), measurements.frequency.begin());
    ASSERT_VECTOR_EQUAL(fg.magnitude.begin(), fg.magnitude.end(), measurements.magnitude.begin());
    ASSERT_VECTOR_EQUAL(fg.phase.begin(), fg.phase.end(), measurements.phase.begin());

    // Timestamps available for the first measurement only
    auto first = measurements.metadata.front();
    // CPPUNIT_ASSERT_EQUAL(info.status, first.status); FIXME: Why an error is expected ?
    // CPPUNIT_ASSERT_EQUAL(info.timestamp, first.timestamp); FIXME: Test Fails since tag restructurization
    CPPUNIT_ASSERT_EQUAL((uint32_t) nbins, first.number_of_bins);

    // For other measurements no timestamp should be available
    // auto ns_per_sample = (int64_t)(1.0f / SAMP_RATE_1KHZ * 1000000000.0f);

    for (std::size_t i = 1; i < nmeasurements; i++) {
        auto m = measurements.metadata[i];
        CPPUNIT_ASSERT_EQUAL((uint32_t) nbins, m.number_of_bins);
        //   int64_t expected_timestamp = info.timestamp + ((int64_t)i * ns_per_sample);
        //   CPPUNIT_ASSERT_DOUBLES_EQUAL((double)expected_timestamp, (double)m.timestamp, 10.0);  FIXME: Test Fails since tag restructurization
        //   CPPUNIT_ASSERT_EQUAL(info.status, m.status);
    }
}

void qa_freq_sink::test_sink_callback() {
    std::size_t nbins         = 4096;
    std::size_t nmeasurements = 10;
    auto        samples       = nbins * nmeasurements;
    auto        sink          = freq_sink::make({ "test", SAMP_RATE_1KHZ, nbins, nmeasurements / 2, 2, freq_sink_mode_t::FREQ_SINK_MODE_STREAMING });

    // expect user callback to be called twice
    std::atomic<unsigned short> counter{};
    data_available_event_t      local_event{};
    auto                        callback = new std::function<void(const data_available_event_t *)>(
            [&counter, &local_event](const data_available_event_t *event) {
                local_event = *event;
                counter++;
            });

#ifdef PORT_DISABLED
    sink->set_callback(&invoke_function, static_cast<void *>(callback));
#endif
    // in order to test trigger timestamp passed to the callback
    acq_info_t info{};
    info.timestamp = 987654321;

    freq_test_flowgraph_t fg(sink, std::vector<tag_t>{ make_acq_info_tag(info, 0) }, nbins, nmeasurements);
    fg.fg->run();

#ifdef PORT_DISABLED
    // test metadata provided to the callback
    CPPUNIT_ASSERT_EQUAL(std::string("test"), local_event.signal_name);
#endif

    const auto measurements = sink->get_measurements(nmeasurements);

    CPPUNIT_ASSERT_EQUAL(nmeasurements / 2, measurements.nmeasurements());
    ASSERT_VECTOR_EQUAL(fg.freq.begin(), fg.freq.begin() + (nmeasurements / 2 * nbins), measurements.frequency.begin());
}

} // namespace gr::digitizers

int main(int, char **) {
    CppUnit::TextTestRunner runner;
    runner.setOutputter(CppUnit::CompilerOutputter::defaultOutputter(
            &runner.result(),
            std::cerr));
    runner.addTest(gr::digitizers::qa_freq_sink::suite());

    bool was_successful = runner.run("", false);
    return was_successful ? 0 : 1;
}
