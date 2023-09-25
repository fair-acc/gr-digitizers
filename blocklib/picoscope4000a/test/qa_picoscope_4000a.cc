#include <boost/ut.hpp>

#include <helper_blocks.h>
#include <picoscope4000a.h>

#include <scheduler.hpp>

namespace gr::picoscope4000a::test {

void test_rapid_block_basic(std::size_t nr_captures)
{
    using namespace boost::ut;
    using namespace fair::graph;
    using namespace gr::helpers;
    using namespace gr::picoscope;
    using namespace gr::picoscope4000a;

    constexpr std::size_t pre_samples = 33;
    constexpr std::size_t post_samples = 1000;
    const auto total_samples = nr_captures * (pre_samples + post_samples);

    graph flow_graph;
    auto& ps = flow_graph.make_node<Picoscope4000a>(Settings{
        .enabled_channels = { { "A", { .range = 5., .coupling = coupling_t::AC_1M } } },
        .sample_rate = 10000.,
        .pre_samples = pre_samples,
        .post_samples = post_samples,
        .acquisition_mode = acquisition_mode_t::RAPID_BLOCK,
        .rapid_block_nr_captures = nr_captures,
        .auto_arm = true,
        .trigger_once = true });

    auto& sink = flow_graph.make_node<count_sink<float>>();
    auto& errsink = flow_graph.make_node<count_sink<float>>();

    expect(eq(connection_result_t::SUCCESS,
              flow_graph.connect<"values0">(ps).template to<"in">(sink)));
    expect(eq(connection_result_t::SUCCESS,
              flow_graph.connect<"errors0">(ps).template to<"in">(errsink)));

    ps.start(); // TODO should be done by scheduler

    scheduler::simple sched{ std::move(flow_graph) };
    sched.run_and_wait();

    expect(eq(sink.samples_seen, total_samples));
    expect(eq(errsink.samples_seen, total_samples));
}

const boost::ut::suite Picoscope4000aTests = [] {
    using namespace boost::ut;
    using namespace fair::graph;
    using namespace gr::helpers;
    using namespace gr::picoscope;
    using namespace gr::picoscope4000a;

    "open and close"_test = [] {
        graph flow_graph;
        auto& ps = flow_graph.make_node<Picoscope4000a>();

        // this takes time, so we do it a few times only
        for (auto i = 0; i < 3; i++) {
            expect(nothrow([&ps] { ps.initialize(); }));
            expect(neq(ps.driver_version(), std::string()));
            expect(neq(ps.hardware_version(), std::string()));
            expect(nothrow([&ps] { ps.close(); })) << "doesn't throw";
        }
    };

    "streaming basics"_test = [] {
        graph flow_graph;
        auto& ps = flow_graph.make_node<Picoscope4000a>(Settings{
            .enabled_channels = { { "A",
                                    { .range = 5., .coupling = coupling_t::AC_1M } } },
            .sample_rate = 10000.,
            .driver_buffer_size = 50000,
            .acquisition_mode = acquisition_mode_t::STREAMING,
            .streaming_mode_poll_rate = 0.00001,
            .auto_arm = true });

        auto& sink = flow_graph.make_node<count_sink<float>>();
        auto& errsink = flow_graph.make_node<count_sink<float>>();

        expect(eq(connection_result_t::SUCCESS,
                  flow_graph.connect<"values0">(ps).template to<"in">(sink)));
        expect(eq(connection_result_t::SUCCESS,
                  flow_graph.connect<"errors0">(ps).template to<"in">(errsink)));

        // Explicitly open unit because it takes quite some time
        expect(nothrow([&ps] { ps.initialize(); }));

        ps.start(); // TODO should be done by scheduler

        // TODO tried multi_threaded scheduler with start(); sleep; stop(), something goes
        // wrong there (scheduler doesn't shovel data reliably)
        scheduler::simple sched{ std::move(flow_graph) };
        auto quitter = std::async([&ps] {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            ps.force_quit();
        });

        sched.run_and_wait();

        expect(eq(sink.samples_seen, errsink.samples_seen));
        expect(ge(sink.samples_seen, std::size_t{ 5000 }));
        expect(le(sink.samples_seen, std::size_t{ 20000 }));
    };


    "rapid block basics"_test = [] { test_rapid_block_basic(1); };

    "rapid block multiple captures"_test = [] { test_rapid_block_basic(3); };

    "rapid block 4 channels"_test = [] {
        constexpr std::size_t pre_samples = 33;
        constexpr std::size_t post_samples = 1000;
        constexpr std::size_t nr_captures = 2;
        constexpr auto total_samples = nr_captures * (pre_samples + post_samples);

        graph flow_graph;
        auto& ps = flow_graph.make_node<Picoscope4000a>(Settings{
            .enabled_channels = { { "A", { .range = 5., .coupling = coupling_t::AC_1M } },
                                  { "B", { .range = 5., .coupling = coupling_t::AC_1M } },
                                  { "C", { .range = 5., .coupling = coupling_t::AC_1M } },
                                  { "D",
                                    { .range = 5., .coupling = coupling_t::AC_1M } } },
            .sample_rate = 10000.,
            .pre_samples = pre_samples,
            .post_samples = post_samples,
            .acquisition_mode = acquisition_mode_t::RAPID_BLOCK,
            .rapid_block_nr_captures = nr_captures,
            .auto_arm = true,
            .trigger_once = true });

        auto& sink0 = flow_graph.make_node<count_sink<float>>();
        auto& sink1 = flow_graph.make_node<count_sink<float>>();
        auto& sink2 = flow_graph.make_node<count_sink<float>>();
        auto& sink3 = flow_graph.make_node<count_sink<float>>();

        expect(eq(connection_result_t::SUCCESS,
                  flow_graph.connect<"values0">(ps).template to<"in">(sink0)));
        expect(eq(connection_result_t::SUCCESS,
                  flow_graph.connect<"values1">(ps).template to<"in">(sink1)));
        expect(eq(connection_result_t::SUCCESS,
                  flow_graph.connect<"values2">(ps).template to<"in">(sink2)));
        expect(eq(connection_result_t::SUCCESS,
                  flow_graph.connect<"values3">(ps).template to<"in">(sink3)));

        ps.start(); // TODO should be done by scheduler

        scheduler::simple sched{ std::move(flow_graph) };
        sched.run_and_wait();

        expect(eq(sink0.samples_seen, total_samples));
        expect(eq(sink1.samples_seen, total_samples));
        expect(eq(sink2.samples_seen, total_samples));
        expect(eq(sink3.samples_seen, total_samples));
    };

    "rapid block continuous"_test = [] {
        graph flow_graph;
        auto& ps = flow_graph.make_node<Picoscope4000a>(Settings{
            .enabled_channels = { { "A",
                                    { .range = 5., .coupling = coupling_t::AC_1M } } },
            .sample_rate = 10000.,
            .post_samples = 1000,
            .acquisition_mode = acquisition_mode_t::RAPID_BLOCK,
            .rapid_block_nr_captures = 1,
            .auto_arm = true });

        auto& sink0 = flow_graph.make_node<count_sink<float>>();

        expect(eq(connection_result_t::SUCCESS,
                  flow_graph.connect<"values0">(ps).template to<"in">(sink0)));

        ps.start(); // TODO should be done by scheduler

        // TODO tried multi_threaded scheduler with start(); sleep; stop(), something goes
        // wrong there (scheduler doesn't shovel data reliably)
        scheduler::simple sched{ std::move(flow_graph) };
        auto quitter = std::async([&ps] {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            ps.force_quit();
        });

        sched.run_and_wait();

        expect(ge(sink0.samples_seen, std::size_t{ 3000 }));
        expect(le(sink0.samples_seen, std::size_t{ 15000 }));
    };
};

} // namespace gr::picoscope4000a::test

int main()
{ /* tests are statically executed */
}
