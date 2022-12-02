
#include "qa_signal_averager.h"
#include "signal_averager.h"

#include <digitizers/tags.h>

#include <gnuradio/attributes.h>
#include <gnuradio/blocks/vector_sink.h>
#include <gnuradio/blocks/vector_source.h>
#include <gnuradio/flowgraph.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

namespace gr::digitizers {

void qa_signal_averager::single_input_test()
{
    double samp_rate = 5000000;

    size_t num_of_repeats = 8;
    auto top = gr::flowgraph::make("single_input_test");

    std::vector<float> simple({ 1, 2, 3, 4, 5, 6, 5, 4, 3, 2 });
    std::vector<float> vec;
    for (size_t i = 0; i < num_of_repeats; i++) {
        vec.insert(vec.end(), simple.begin(), simple.end());
        for (auto it = simple.begin(); it != simple.end(); ++it) {
            *it *= 2;
        }
    }

    trigger_t tag1;
    trigger_t tag2;

    acq_info_t acq_info_tag1;
    acq_info_tag1.status = 2;
    acq_info_t acq_info_tag2;
    acq_info_tag2.status = 1;

    std::vector<gr::tag_t> tags = { make_trigger_tag(tag1, 40),
                                    make_trigger_tag(tag2, 73),
                                    make_acq_info_tag(acq_info_tag1, 74),
                                    make_acq_info_tag(acq_info_tag2, 75) };

    auto src = blocks::vector_source_f::make({ .data = vec, .tags = tags });
    auto avg = signal_averager::make({ 1, simple.size(), samp_rate });
    auto snk = blocks::vector_sink_f::make({ 1 });

    top->connect(src, 0, avg, 0);
    top->connect(avg, 0, snk, 0);

    top->run();
    auto data = snk->data();
    auto tags_out = snk->tags();
    CPPUNIT_ASSERT_EQUAL(num_of_repeats, data.size());
    CPPUNIT_ASSERT_EQUAL(size_t(3), tags_out.size());
    CPPUNIT_ASSERT_EQUAL(uint64_t(4), tags_out[0].offset());
    CPPUNIT_ASSERT_EQUAL(uint64_t(7), tags_out[1].offset()); // round down
    CPPUNIT_ASSERT_EQUAL(uint64_t(7), tags_out[2].offset()); // round down

    acq_info_t acq_info_tag = decode_acq_info_tag(tags_out.at(2));

    CPPUNIT_ASSERT_EQUAL(uint32_t(3), acq_info_tag.status); // logical OR of all stati

    float desired_avg = data.at(0);
    for (auto it = data.begin(); it != data.end(); ++it) {
        CPPUNIT_ASSERT_DOUBLES_EQUAL(desired_avg, *it, 0.0001);
        desired_avg *= 2;
    }
}

void qa_signal_averager::multiple_input_test()
{
    double samp_rate = 5000000;
    std::size_t number_of = 8;
    auto top = gr::flowgraph::make("single_input_test");

    std::vector<float> simple({ 1, 2, 3, 4, 5, 6, 5, 4, 3, 2 });
    std::vector<float> vec0, vec1;
    for (unsigned i = 0; i < number_of; i++) {
        vec0.insert(vec0.end(), simple.begin(), simple.end());
        for (auto it = simple.begin(); it != simple.end(); ++it) {
            *it *= 2.0;
        }
        vec1.insert(vec1.end(), simple.begin(), simple.end());
    }

    auto src0 = blocks::vector_source_f::make({ vec0 });
    auto src1 = blocks::vector_source_f::make({ vec1 });
    auto avg = signal_averager::make({ 2, simple.size(), samp_rate });
    auto snk0 = blocks::vector_sink_f::make({ 1 });
    auto snk1 = blocks::vector_sink_f::make({ 1 });

    top->connect(src0, 0, avg, 0);
    top->connect(src1, 0, avg, 1);
    top->connect(avg, 0, snk0, 0);
    top->connect(avg, 1, snk1, 0);

    top->run();
    auto data0 = snk0->data();
    auto data1 = snk1->data();
    CPPUNIT_ASSERT_EQUAL(number_of, data0.size());
    CPPUNIT_ASSERT_EQUAL(number_of, data1.size());
    float desired_avg = data0.at(0);
    for (std::size_t i = 0; i < number_of; i++) {
        CPPUNIT_ASSERT_DOUBLES_EQUAL(desired_avg, data0.at(i), 0.0001);
        desired_avg *= 2;
        CPPUNIT_ASSERT_DOUBLES_EQUAL(desired_avg, data1.at(i), 0.0001);
    }
}

void qa_signal_averager::offset_trigger_tag_test()
{
    double samp_rate = 1000000; // sample_to_sample distance = 1µs
    std::size_t decim = 100;
    std::size_t size = 1000000;
    std::vector<float> samples;

    for (std::size_t i = 0; i < size; i++)
        samples.push_back(1.);

    trigger_t tag0, tag1, tag2, tag3;

    std::vector<gr::tag_t> tags = {
        make_trigger_tag(
            tag0, 400), // samples 400 till 490 should be merged. "Middle" is sample 450
                        // So a negative offset of 50 samples (= -50µs) is expected.
        make_trigger_tag(
            tag1, 410), // samples 400 till 490 should be merged. "Middle" is sample 450
                        // So a negative offset of 4 samples (= -40µs) is expected.
        make_trigger_tag(tag2, 100750), // samples 700 till 790 should be merged. "Middle"
                                        // is sample 750 So no offset is expected.
        make_trigger_tag(
            tag3, 100790), // samples 700 till 790 should be merged. "Middle" is sample
                           // 750 So a positive offset of 4 samples (= +40µs) is expected.
    };

    auto top = gr::flowgraph::make("single_input_test");
    auto src = blocks::vector_source_f::make({ samples, false, 1, tags });
    auto avg = signal_averager::make({ 1, decim, samp_rate });
    auto snk = blocks::vector_sink_f::make({ 1 });

    top->connect(src, 0, avg, 0);
    top->connect(avg, 0, snk, 0);

    top->run();
    auto data = snk->data();
    auto tags_out = snk->tags();
    CPPUNIT_ASSERT_EQUAL(size_t(4), tags_out.size());
    CPPUNIT_ASSERT_EQUAL(size_t(size / decim), data.size());
}

} // namespace gr::digitizers

int main(int, char**)
{
    CppUnit::TextTestRunner runner;
    runner.setOutputter(
        CppUnit::CompilerOutputter::defaultOutputter(&runner.result(), std::cerr));
    runner.addTest(gr::digitizers::qa_signal_averager::suite());

    bool was_successful = runner.run("", false);
    return was_successful ? 0 : 1;
}
