#include "qa_common.h"
#include "qa_digitizer_block.h"

#include <digitizers/simulation_source.h>
#include <digitizers/tags.h>
#include <digitizers/timing_receiver_simulated.h>

#include <gnuradio/attributes.h>
#include <gnuradio/blocks/vector_sink.h>
#include <gnuradio/flowgraph.h>
#include <gnuradio/streamops/throttle.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

#include <chrono>
#include <optional>
#include <thread>

namespace gr::digitizers {

struct simulated_test_flowgraph_t {
    gr::flowgraph::sptr top;
    simulation_source::sptr source;
    gr::blocks::vector_sink_f::sptr sink_sig_a;
    gr::blocks::vector_sink_f::sptr sink_err_a;
    gr::blocks::vector_sink_f::sptr sink_sig_b;
    gr::blocks::vector_sink_f::sptr sink_err_b;
    gr::blocks::vector_sink_b::sptr sink_port;
    gr::streamops::throttle::sptr throttle;
    timing_receiver_simulated::sptr timing_receiver;
};

static simulation_source::block_args default_args()
{
    return {
        .sample_rate = 10000,
        .auto_arm = true,
    };
}

static simulated_test_flowgraph_t make_test_flowgraph(
    simulation_source::block_args args,
    std::optional<timing_receiver_simulated::block_args> timing_receiver_args = {})
{
    simulated_test_flowgraph_t fg;

    fg.top = gr::flowgraph::make("test");
    fg.source = gr::digitizers::simulation_source::make(std::move(args));

    fg.sink_sig_a = blocks::vector_sink_f::make({ 1 });
    fg.sink_err_a = blocks::vector_sink_f::make({ 1 });
    fg.sink_sig_b = blocks::vector_sink_f::make({ 1 });
    fg.sink_err_b = blocks::vector_sink_f::make({ 1 });
    fg.sink_port = blocks::vector_sink_b::make({ 1 });

    fg.throttle = streamops::throttle::make(
        { .samples_per_sec = args.sample_rate, .itemsize = sizeof(float) });

    if (timing_receiver_args) {
        fg.timing_receiver = timing_receiver_simulated::make(*timing_receiver_args);
    }

    fg.top->connect(fg.source, 0, fg.throttle, 0);
    fg.top->connect(fg.throttle, 0, fg.sink_sig_a, 0);
    fg.top->connect(fg.source, 1, fg.sink_err_a, 0);
    fg.top->connect(fg.source, 2, fg.sink_sig_b, 0);
    fg.top->connect(fg.source, 3, fg.sink_err_b, 0);
    fg.top->connect(fg.source, 4, fg.sink_port, 0);
    if (fg.timing_receiver) {
        fg.top->connect(fg.timing_receiver, "out", fg.source, "timing");
    }

    return fg;
}

template <typename T>
static std::vector<T>
make_timing_signal(std::size_t length, T lo, T hi, std::vector<std::size_t> triggers)
{
    std::vector<T> data(length);
    T current = lo;
    for (std::size_t i = 0; i < length; ++i) {
        const auto is_trigger_pos =
            std::find(triggers.begin(), triggers.end(), i) != triggers.end();
        if (is_trigger_pos) {
            current = (current == lo) ? hi : lo;
        }
        data[i] = current;
        if (is_trigger_pos && i != length - 1) {
            current = (current == lo) ? hi : lo;
            data[i + 1] = current;
            ++i;
        }
    }
    return data;
}

static std::vector<uint8_t> make_digital_timing_signal(std::size_t length,
                                                       std::vector<std::size_t> triggers)
{
    return make_timing_signal(length, uint8_t(0), uint8_t(1), triggers);
}

void qa_digitizer_block::fill_data(unsigned samples, unsigned presamples)
{
    float add = 0;
    for (unsigned i = 0; i < samples + presamples; i++) {
        if (i == presamples) {
            add = 2.0;
        }
        d_cha_vec.push_back(add + 2.0 * i / 1033.0);
        d_chb_vec.push_back(2.0 * i / 1033.0);
        d_port_vec.push_back(i % 256);
    }
}

void qa_digitizer_block::rapid_block_basics()
{
    int samples = 1000;
    int presamples = 50;
    fill_data(samples, presamples);

    auto args = default_args();
    args.trigger_once = true;
    args.rapid_block_nr_captures = 1;
    args.acquisition_mode = digitizer_acquisition_mode_t::RAPID_BLOCK;
    args.pre_samples = presamples;
    args.post_samples = samples;

    auto fg = make_test_flowgraph(args);
    auto source = fg.source;

    fg.source->set_data(d_cha_vec, d_chb_vec, d_port_vec);

    fg.top->run();

    auto dataa = fg.sink_sig_a->data();
    CPPUNIT_ASSERT_EQUAL(samples + presamples, (int)dataa.size());
    ASSERT_VECTOR_EQUAL(d_cha_vec.begin(), d_cha_vec.end(), dataa.begin());

    auto datab = fg.sink_sig_b->data();
    CPPUNIT_ASSERT_EQUAL(samples + presamples, (int)datab.size());
    ASSERT_VECTOR_EQUAL(d_chb_vec.begin(), d_chb_vec.end(), datab.begin());

    auto datap = fg.sink_port->data();
    CPPUNIT_ASSERT_EQUAL(samples + presamples, (int)datap.size());
    ASSERT_VECTOR_EQUAL(
        d_port_vec.begin(), d_port_vec.end(), reinterpret_cast<uint8_t*>(&datap[0]));
}

void qa_digitizer_block::rapid_block_channel_b_only()
{
    int samples = 1000;
    int presamples = 50;
    fill_data(samples, presamples);

    auto args = default_args();
    args.trigger_once = true;
    args.rapid_block_nr_captures = 1;
    args.acquisition_mode = digitizer_acquisition_mode_t::RAPID_BLOCK;
    args.pre_samples = presamples;
    args.post_samples = samples;

    auto fg = make_test_flowgraph(args);
    auto source = fg.source;

    // make sure we can leave gaps in the enabled channels
    fg.source->set_data({}, d_chb_vec, d_port_vec);

    fg.top->run();

    auto dataa = fg.sink_sig_a->data();
    CPPUNIT_ASSERT_EQUAL(samples + presamples, (int)dataa.size());

    auto datab = fg.sink_sig_b->data();
    CPPUNIT_ASSERT_EQUAL(samples + presamples, (int)datab.size());
    ASSERT_VECTOR_EQUAL(d_chb_vec.begin(), d_chb_vec.end(), datab.begin());

    auto datap = fg.sink_port->data();
    CPPUNIT_ASSERT_EQUAL(samples + presamples, (int)datap.size());
    ASSERT_VECTOR_EQUAL(
        d_port_vec.begin(), d_port_vec.end(), reinterpret_cast<uint8_t*>(&datap[0]));
}

void qa_digitizer_block::rapid_block_correct_tags()
{
    using namespace std::chrono_literals;

    const std::string trigger_name = "PPS";
    const auto trigger_timestamp = 123456789ns;
    const double trigger_offset = 0.;

    int samples = 2000;
    int presamples = 200;
    fill_data(samples, presamples);

    auto args = default_args();
    args.pre_samples = presamples;
    args.post_samples = samples;
    args.trigger_once = true;
    args.rapid_block_nr_captures = 1;
    args.acquisition_mode = digitizer_acquisition_mode_t::RAPID_BLOCK;

    timing_receiver_simulated::block_args timing_receiver_args = {
        .simulation_mode = timing_receiver_simulation_mode_t::MANUAL
    };

    auto fg = make_test_flowgraph(args, timing_receiver_args);
    auto source = fg.source;

    fg.source->set_data(d_cha_vec, d_chb_vec, d_port_vec);


    fg.top->start();
    fg.timing_receiver->post_timing_message(
        trigger_name, trigger_timestamp.count(), trigger_offset);
    std::this_thread::sleep_for(1s);
    fg.top->stop();
    fg.top->wait();

    auto data_tags = fg.sink_sig_a->tags();

    auto trigger_tags = data_tags;
    std::erase_if(trigger_tags, [](const auto& tag) {
        return !tag.get(tag::TRIGGER_TIME.key()).has_value();
    });
    CPPUNIT_ASSERT_EQUAL(std::size_t{ 1 }, trigger_tags.size());
    CPPUNIT_ASSERT_EQUAL(std::size_t{ presamples }, trigger_tags[0].offset());

    const auto trigger_data = decode_trigger_tag(trigger_tags[0]);
    CPPUNIT_ASSERT_EQUAL(trigger_name, trigger_data.name);
    CPPUNIT_ASSERT_EQUAL(trigger_timestamp, trigger_data.timestamp);
    CPPUNIT_ASSERT_EQUAL(0ns, trigger_data.offset);

    auto other_tags = data_tags;
    std::erase_if(other_tags, [](const auto& tag) {
        return tag.get(tag::TRIGGER_TIME.key()).has_value();
    });

    for (auto& tag : other_tags) {
        CPPUNIT_ASSERT_EQUAL(tag.map().size(), std::size_t{ 1 });
        const auto key = tag.map().begin()->first;

        CPPUNIT_ASSERT(key == acq_info_tag_name || key == timebase_info_tag_name);

        if (key == timebase_info_tag_name) {
            auto timebase = decode_timebase_info_tag(tag);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0 / 10000.0, timebase, 0.0000001);
        }
        else if (key == acq_info_tag_name) {
            CPPUNIT_ASSERT_EQUAL(static_cast<uint64_t>(presamples), tag.offset());
        }
        else {
            CPPUNIT_FAIL("unknown tag. key: " + key);
        }
    }
}

void qa_digitizer_block::streaming_basics()
{
    int samples = 2000;
    int presamples = 200;
    int buffer_size = samples + presamples;

    fill_data(samples, presamples);

    auto args = default_args();
    args.acquisition_mode = digitizer_acquisition_mode_t::STREAMING;
    args.streaming_mode_poll_rate = 0.0001;
    args.buffer_size = buffer_size;

    auto fg = make_test_flowgraph(args);

    fg.source->set_data(d_cha_vec, d_chb_vec, d_port_vec);

    fg.top->start();
    std::this_thread::sleep_for(std::chrono::seconds(1));
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
    ASSERT_VECTOR_EQUAL(d_port_vec.begin(), d_port_vec.begin() + size, datap.begin());

    // no timing receiver configured, no trigger tags expected
    const auto tags = fg.sink_sig_a->tags();
    const auto is_trigger_tag = [](const auto& tag) {
        return tag.get(tag::TRIGGER_TIME.key()).has_value();
    };
    CPPUNIT_ASSERT(!std::any_of(tags.begin(), tags.end(), is_trigger_tag));
}

void qa_digitizer_block::streaming_correct_tags()
{
    int samples = 2000;
    int presamples = 200;
    int buffer_size = samples + presamples;

    fill_data(samples, presamples);

    auto args = default_args();
    args.buffer_size = buffer_size;
    args.acquisition_mode = digitizer_acquisition_mode_t::STREAMING;
    args.streaming_mode_poll_rate = 0.0001;

    auto fg = make_test_flowgraph(args);
    auto source = fg.source;

    fg.source->set_data(d_cha_vec, d_chb_vec, d_port_vec);

    fg.top->start();
    std::this_thread::sleep_for(std::chrono::microseconds(500));
    fg.top->stop();
    fg.top->wait();

    auto data_tags = fg.sink_sig_a->tags();

    for (auto& tag : data_tags) {
        CPPUNIT_ASSERT_EQUAL(tag.map().size(), std::size_t{ 1 });
        const auto key = tag.map().begin()->first;
        CPPUNIT_ASSERT(key == timebase_info_tag_name || key == acq_info_tag_name);

        if (key == acq_info_tag_name) {
            CPPUNIT_ASSERT_EQUAL(0, static_cast<int>(tag.offset()) % buffer_size);
        }
        else {
            double timebase = decode_timebase_info_tag(tag);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0 / 10000.0, timebase, 0.0000001);
        }
    }
}

void qa_digitizer_block::streaming_timing_no_signal()
{
    using namespace std::chrono_literals;

    int samples = 2000;
    int presamples = 200;
    int buffer_size = samples + presamples;
    const std::string trigger_name = "PPS";
    const auto trigger_timestamp = 123456789ns;
    const double trigger_offset = 0.;

    fill_data(samples, presamples);

    auto args = default_args();
    args.buffer_size = buffer_size;
    args.acquisition_mode = digitizer_acquisition_mode_t::STREAMING;
    args.streaming_mode_poll_rate = 0.0001;

    timing_receiver_simulated::block_args timing_receiver_args = {
        .simulation_mode = timing_receiver_simulation_mode_t::MANUAL
    };

    auto fg = make_test_flowgraph(args, timing_receiver_args);
    auto source = fg.source;

    fg.source->set_data(d_cha_vec, d_chb_vec, d_port_vec);

    fg.top->start();
    fg.timing_receiver->post_timing_message(
        trigger_name, trigger_timestamp.count(), trigger_offset);
    std::this_thread::sleep_for(1s);
    fg.top->stop();
    fg.top->wait();

    auto data_tags = fg.sink_sig_a->tags();

    auto trigger_tags = data_tags;
    std::erase_if(trigger_tags, [](const auto& tag) {
        return !tag.get(tag::TRIGGER_TIME.key()).has_value();
    });
    CPPUNIT_ASSERT(!trigger_tags.empty());

    CPPUNIT_ASSERT_EQUAL(std::size_t{ 0 }, trigger_tags[0].offset());
    for (const auto& tag : trigger_tags) {
        const auto trigger_data = decode_trigger_tag(tag);
        CPPUNIT_ASSERT_EQUAL(trigger_name, trigger_data.name);
        CPPUNIT_ASSERT_EQUAL(std::chrono::nanoseconds(trigger_timestamp),
                             trigger_data.timestamp);
        CPPUNIT_ASSERT_EQUAL(0ns, trigger_data.offset);
        CPPUNIT_ASSERT_EQUAL(std::size_t{ 0 }, tag.offset() % buffer_size);
    }
}

void qa_digitizer_block::streaming_timing_analog_input()
{
    using namespace std::chrono_literals;

    int samples = 2000;
    int presamples = 200;
    int buffer_size = samples + presamples;
    const std::string trigger_name = "PPS";
    const std::vector<std::chrono::nanoseconds> trigger_timestamps = { 12345ns,
                                                                       1234567ns,
                                                                       123456789ns };
    const double trigger_offset = 0.;
    const std::vector<std::size_t> trigger_offsets = { 100, 742, 2100 };
    const auto trigger_signal_data =
        make_timing_signal(samples + presamples, 0.3f, 1.7f, trigger_offsets);

    fill_data(samples, presamples);

    auto args = default_args();
    args.buffer_size = buffer_size;
    args.acquisition_mode = digitizer_acquisition_mode_t::STREAMING;
    args.streaming_mode_poll_rate = 0.0001;

    timing_receiver_simulated::block_args timing_receiver_args = {
        .simulation_mode = timing_receiver_simulation_mode_t::MANUAL
    };

    auto fg = make_test_flowgraph(args, timing_receiver_args);
    auto source = fg.source;

    fg.source->set_data(d_cha_vec, trigger_signal_data, d_port_vec);
    fg.source->set_aichan_trigger("B", trigger_direction_t::HIGH, 1.3);

    fg.top->start();
    for (const auto timestamp : trigger_timestamps) {
        fg.timing_receiver->post_timing_message(
            trigger_name, timestamp.count(), trigger_offset);
    }
    std::this_thread::sleep_for(1s);
    fg.top->stop();
    fg.top->wait();

    auto data_tags = fg.sink_sig_a->tags();

    auto trigger_tags = data_tags;
    std::erase_if(trigger_tags, [](const auto& tag) {
        return !tag.get(tag::TRIGGER_TIME.key()).has_value();
    });
    CPPUNIT_ASSERT_EQUAL(std::size_t{ 3 }, trigger_tags.size());

    const std::vector<trigger_t> trigger_data = { decode_trigger_tag(trigger_tags[0]),
                                                  decode_trigger_tag(trigger_tags[1]),
                                                  decode_trigger_tag(trigger_tags[2]) };
    CPPUNIT_ASSERT_EQUAL(trigger_offsets[0], trigger_tags[0].offset());
    CPPUNIT_ASSERT_EQUAL(trigger_offsets[1], trigger_tags[1].offset());
    CPPUNIT_ASSERT_EQUAL(trigger_offsets[2], trigger_tags[2].offset());
    CPPUNIT_ASSERT_EQUAL(trigger_timestamps[0], trigger_data[0].timestamp);
    CPPUNIT_ASSERT_EQUAL(trigger_timestamps[1], trigger_data[1].timestamp);
    CPPUNIT_ASSERT_EQUAL(trigger_timestamps[2], trigger_data[2].timestamp);

    for (const auto& tag : trigger_tags) {
        const auto trigger_data = decode_trigger_tag(tag);
        CPPUNIT_ASSERT_EQUAL(trigger_name, trigger_data.name);
        CPPUNIT_ASSERT_EQUAL(0ns, trigger_data.offset);
    }
}

void qa_digitizer_block::streaming_timing_digital_input()
{
    using namespace std::chrono_literals;

    int samples = 2000;
    int presamples = 200;
    int buffer_size = samples + presamples;
    const std::string trigger_name = "PPS";
    const std::vector<std::chrono::nanoseconds> trigger_timestamps = { 12345ns,
                                                                       1234567ns,
                                                                       123456789ns };
    const double trigger_offset = 0.;
    const std::vector<std::size_t> trigger_offsets = { 100, 742, 2100 };
    const auto trigger_signal_data =
        make_digital_timing_signal(samples + presamples, trigger_offsets);

    fill_data(samples, presamples);

    auto args = default_args();
    args.buffer_size = buffer_size;
    args.acquisition_mode = digitizer_acquisition_mode_t::STREAMING;
    args.streaming_mode_poll_rate = 0.0001;

    timing_receiver_simulated::block_args timing_receiver_args = {
        .simulation_mode = timing_receiver_simulation_mode_t::MANUAL
    };

    auto fg = make_test_flowgraph(args, timing_receiver_args);
    auto source = fg.source;

    fg.source->set_data(d_cha_vec, d_chb_vec, trigger_signal_data);
    fg.source->set_di_trigger(0, trigger_direction_t::HIGH);

    fg.top->start();
    for (const auto timestamp : trigger_timestamps) {
        fg.timing_receiver->post_timing_message(
            trigger_name, timestamp.count(), trigger_offset);
    }
    std::this_thread::sleep_for(1s);
    fg.top->stop();
    fg.top->wait();

    auto data_tags = fg.sink_sig_a->tags();

    auto trigger_tags = data_tags;
    std::erase_if(trigger_tags, [](const auto& tag) {
        return !tag.get(tag::TRIGGER_TIME.key()).has_value();
    });
    CPPUNIT_ASSERT_EQUAL(std::size_t{ 3 }, trigger_tags.size());

    const std::vector<trigger_t> trigger_data = { decode_trigger_tag(trigger_tags[0]),
                                                  decode_trigger_tag(trigger_tags[1]),
                                                  decode_trigger_tag(trigger_tags[2]) };
    CPPUNIT_ASSERT_EQUAL(trigger_offsets[0], trigger_tags[0].offset());
    CPPUNIT_ASSERT_EQUAL(trigger_offsets[1], trigger_tags[1].offset());
    CPPUNIT_ASSERT_EQUAL(trigger_offsets[2], trigger_tags[2].offset());
    CPPUNIT_ASSERT_EQUAL(trigger_timestamps[0], trigger_data[0].timestamp);
    CPPUNIT_ASSERT_EQUAL(trigger_timestamps[1], trigger_data[1].timestamp);
    CPPUNIT_ASSERT_EQUAL(trigger_timestamps[2], trigger_data[2].timestamp);

    for (const auto& tag : trigger_tags) {
        const auto trigger_data = decode_trigger_tag(tag);
        CPPUNIT_ASSERT_EQUAL(trigger_name, trigger_data.name);
        CPPUNIT_ASSERT_EQUAL(0ns, trigger_data.offset);
    }
}

} // namespace gr::digitizers

int main(int, char**)
{
    CppUnit::TextTestRunner runner;
    runner.setOutputter(
        CppUnit::CompilerOutputter::defaultOutputter(&runner.result(), std::cerr));
    runner.addTest(gr::digitizers::qa_digitizer_block::suite());

    bool was_successful = runner.run("", false);
    return was_successful ? 0 : 1;
}
