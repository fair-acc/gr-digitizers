#include "qa_post_mortem_sink.h"

#include "qa_common.h"
#include "utils.h"

#include <digitizers/post_mortem_sink.h>
#include <digitizers/tags.h>

#include <gnuradio/blocks/vector_sink.h>
#include <gnuradio/blocks/vector_source.h>
#include <gnuradio/flowgraph.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

#include <functional>

namespace gr::digitizers {

void qa_post_mortem_sink::basics() {
    auto sink     = post_mortem_sink::make({ "test", "unit", 1234.0f, 2048 });

    auto metadata = sink->get_metadata();
    CPPUNIT_ASSERT_EQUAL(std::string("test"), metadata.name);
    CPPUNIT_ASSERT_EQUAL(std::string("unit"), metadata.unit);
    CPPUNIT_ASSERT_EQUAL(1234.0f, sink->sample_rate());

    std::size_t data_size = 10;

    const auto  pm_data   = sink->get_post_mortem_data(data_size);
    CPPUNIT_ASSERT_EQUAL(std::size_t{ 0 }, pm_data.values.size());
    CPPUNIT_ASSERT_EQUAL(std::size_t{ 0 }, pm_data.errors.size());
}

template<typename ItExpected, typename ItActual>
static void
assert_equal(ItExpected expected_begin, ItExpected expected_end, ItActual actual_begin) {
    for (; expected_begin != expected_end; ++expected_begin, ++actual_begin) {
        CPPUNIT_ASSERT_EQUAL(*expected_begin, *actual_begin);
    }
}

template<typename ItExpected, typename ItActual>
static void
assert_equal(const ItExpected &expected, const ItActual &returned) {
    for (std::size_t i = 0; i < expected.size(); i++) {
        CPPUNIT_ASSERT_EQUAL(expected[i], returned[i]);
    }
}

template<typename ItExpected, typename ItActual>
static void
assert_equal(const ItExpected &expected, const ItActual *returned) {
    for (std::size_t i = 0; i < expected.size(); i++) {
        CPPUNIT_ASSERT_EQUAL(expected[i], returned[i]);
    }
}

static const float DEFAULT_SAMP_RATE = 100000.0f;

void               qa_post_mortem_sink::buffer_not_full() {
                  auto fg = gr::flowgraph::make("test");

                  // test data
                  std::size_t data_size = 124;
                  auto        data      = make_test_data(data_size);

                  auto        source    = gr::blocks::vector_source_f::make({ data });
                  auto        pm        = post_mortem_sink::make({ "test", "unit", DEFAULT_SAMP_RATE, data_size * 2 });
                  auto        sink      = gr::blocks::vector_sink_f::make({});
                  auto        sink_errs = gr::blocks::vector_sink_f::make({});

                  // connect data and error and than run
                  fg->connect(source, 0, pm, 0);
                  fg->connect(source, 0, pm, 1);
                  fg->connect(pm, 0, sink, 0);
                  fg->connect(pm, 1, sink_errs, 0);
                  fg->run();

#ifdef PORT_DISABLED // Check if we can access this outside of the work function
    CPPUNIT_ASSERT_EQUAL(data_size, pm->nitems_read(0));
#endif

    const auto pm_data = pm->get_post_mortem_data(data_size);

    CPPUNIT_ASSERT_EQUAL(data_size, pm_data.values.size());
    CPPUNIT_ASSERT_EQUAL(data_size, pm_data.errors.size());
    assert_equal(data, pm_data.values.data());
    assert_equal(data, sink->data());
    CPPUNIT_ASSERT_EQUAL(int64_t{ -1 }, pm_data.info.timestamp);
}

void qa_post_mortem_sink::buffer_full() {
    auto fg = gr::flowgraph::make("test");

    // test data
    std::size_t data_size   = 124;
    auto        data        = make_test_data(data_size);
    auto        data_errs   = make_test_data(data_size, 0.01);

    auto        source      = gr::blocks::vector_source_f::make({ data });
    auto        source_errs = gr::blocks::vector_source_f::make({ data_errs });
    auto        pm          = post_mortem_sink::make({ "test", "unit", DEFAULT_SAMP_RATE, data_size });
    auto        sink        = gr::blocks::vector_sink_f::make({});
    auto        sink_errs   = gr::blocks::vector_sink_f::make({});

    // connect and run
    fg->connect(source, 0, pm, 0);
    fg->connect(source_errs, 0, pm, 1);
    fg->connect(pm, 0, sink, 0);
    fg->connect(pm, 1, sink_errs, 0);
    fg->run();

#ifdef PORT_DISABLED // Check if we can access this outside of the work function
    CPPUNIT_ASSERT_EQUAL(data_size, pm->nitems_read(0));
#endif

    const auto pm_data = pm->get_post_mortem_data(data_size * 2);

    CPPUNIT_ASSERT_EQUAL(data_size, pm_data.values.size());
    CPPUNIT_ASSERT_EQUAL(data_size, pm_data.errors.size());
    assert_equal(data, pm_data.values);
    assert_equal(data, sink->data());
    assert_equal(data_errs, pm_data.errors);
    assert_equal(data_errs, sink_errs->data());
    CPPUNIT_ASSERT_EQUAL(int64_t{ -1 }, pm_data.info.timestamp);
}

void qa_post_mortem_sink::buffer_overflow() {
    auto        fg          = gr::flowgraph::make("test");

    std::size_t data_size   = 124;
    auto        data        = make_test_data(data_size);
    auto        data_errs   = make_test_data(data_size, 0.01);

    auto        source      = gr::blocks::vector_source_f::make({ data });
    auto        source_errs = gr::blocks::vector_source_f::make({ data_errs });
    auto        pm          = post_mortem_sink::make({ "test", "unit", DEFAULT_SAMP_RATE, data_size - 1 });
    auto        sink        = gr::blocks::vector_sink_f::make({});
    auto        sink_errs   = gr::blocks::vector_sink_f::make({});

    // connect and run
    fg->connect(source, 0, pm, 0);
    fg->connect(source_errs, 0, pm, 1);
    fg->connect(pm, 0, sink, 0);
    fg->connect(pm, 1, sink_errs, 0);

    fg->run();

#ifdef PORT_DISABLED // Check if we can access this outside of the work function
    CPPUNIT_ASSERT_EQUAL(data_size, pm->nitems_read(0));
#endif

    const auto pm_data = pm->get_post_mortem_data(data_size);

    CPPUNIT_ASSERT_EQUAL(data_size - 1, pm_data.values.size());
    CPPUNIT_ASSERT_EQUAL(data_size - 1, pm_data.errors.size());
    assert_equal(data.begin() + 1, data.end(), pm_data.values.begin());
    assert_equal(data, sink->data());
    assert_equal(data_errs.begin() + 1, data_errs.end(), pm_data.errors.begin());
    assert_equal(data_errs, sink_errs->data());
    CPPUNIT_ASSERT_EQUAL(int64_t{ -1 }, pm_data.info.timestamp);
}

// Check if timestamp is calculated correctly
void qa_post_mortem_sink::acq_info() {
    std::size_t data_size = 200;

    // Monotone clock is assumed...
    double     timebase = 0.00015;
    acq_info_t info{};
    info.timebase               = timebase;
    info.timestamp              = 321;
    info.status                 = 1 << 1;

    std::vector<gr::tag_t> tags = {
        make_acq_info_tag(info, 0)
    };

    auto fg          = gr::flowgraph::make("test");

    auto data        = make_test_data(data_size);
    auto data_errs   = make_test_data(data_size, 0.01);

    auto source      = gr::blocks::vector_source_f::make({ .data = data, .tags = tags });
    auto source_errs = gr::blocks::vector_source_f::make({ data_errs });
    auto pm          = post_mortem_sink::make({ "test", "unit", DEFAULT_SAMP_RATE, data_size * 2 });
    auto sink        = gr::blocks::vector_sink_f::make({});
    auto sink_errs   = gr::blocks::vector_sink_f::make({});

    // connect and run
    fg->connect(source, 0, pm, 0);
    fg->connect(source_errs, 0, pm, 1);
    fg->connect(pm, 0, sink, 0);
    fg->connect(pm, 1, sink_errs, 0);

    fg->run();

    const auto pm_data = pm->get_post_mortem_data(data_size);
    CPPUNIT_ASSERT_EQUAL(data_size, pm_data.values.size());
    CPPUNIT_ASSERT_EQUAL(data_size, pm_data.errors.size());
    CPPUNIT_ASSERT_EQUAL(uint32_t{ 1 << 1 }, pm_data.info.status);
    CPPUNIT_ASSERT_EQUAL(timebase, pm_data.info.timebase);
}

} /* namespace gr::digitizers */

int main(int, char **) {
    CppUnit::TextTestRunner runner;
    runner.setOutputter(CppUnit::CompilerOutputter::defaultOutputter(
            &runner.result(),
            std::cerr));
    runner.addTest(gr::digitizers::qa_post_mortem_sink::suite());

    bool was_successful = runner.run("", false);
    return was_successful ? 0 : 1;
}
