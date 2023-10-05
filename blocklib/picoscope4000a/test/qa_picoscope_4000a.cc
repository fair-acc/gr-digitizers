#include <boost/ut.hpp>

#include <helper_blocks.hpp>
#include <picoscope4000a.hpp>

#include <scheduler.hpp>

namespace fair::picoscope4000a::test {

void
test_rapid_block_basic(std::size_t nr_captures) {
    using namespace boost::ut;
    using namespace fair::graph;
    using namespace gr::helpers;
    using namespace fair::picoscope;
    using namespace fair::picoscope4000a;

    constexpr std::size_t pre_samples   = 33;
    constexpr std::size_t post_samples  = 1000;
    const auto            total_samples = nr_captures * (pre_samples + post_samples);

    graph::graph          flow_graph;
    auto                 &ps = flow_graph.make_node<Picoscope4000a>({ { { "sample_rate", 10000. },
                                                                        { "pre_samples", pre_samples },
                                                                        { "post_samples", post_samples },
                                                                        { "acquisition_mode_string", "RAPID_BLOCK" },
                                                                        { "rapid_block_nr_captures", nr_captures },
                                                                        { "auto_arm", true },
                                                                        { "trigger_once", true } } });
    ps.set_channel_configuration({ { "A", { .range = 5., .coupling = coupling_t::AC_1M } } });
    auto &sink    = flow_graph.make_node<count_sink<float>>();
    auto &errsink = flow_graph.make_node<count_sink<float>>();

    expect(eq(connection_result_t::SUCCESS, flow_graph.connect<"values0">(ps).template to<"in">(sink)));
    expect(eq(connection_result_t::SUCCESS, flow_graph.connect<"errors0">(ps).template to<"in">(errsink)));

    scheduler::simple sched{ std::move(flow_graph) };
    sched.run_and_wait();

    expect(eq(sink.samples_seen, total_samples));
    expect(eq(errsink.samples_seen, total_samples));
}

const boost::ut::suite Picoscope4000aTests = [] {
    using namespace boost::ut;
    using namespace fair::graph;
    using namespace gr::helpers;
    using namespace fair::picoscope;
    using namespace fair::picoscope4000a;

    "open and close"_test = [] {
        graph::graph flow_graph;
        auto &ps = flow_graph.make_node<Picoscope4000a>();

        // this takes time, so we do it a few times only
        for (auto i = 0; i < 3; i++) {
            expect(nothrow([&ps] { ps.initialize(); }));
            expect(neq(ps.driver_version(), std::string()));
            expect(neq(ps.hardware_version(), std::string()));
            expect(nothrow([&ps] { ps.close(); })) << "doesn't throw";
        }
    };

    "invalid settings"_test = [] {
        graph::graph flow_graph;

        // good channel
        expect(nothrow([&flow_graph] {
            auto &ps = flow_graph.make_node<Picoscope4000a>();
            ps.set_channel_configuration({ { "A", { .range = 5., .coupling = coupling_t::AC_1M } } });
        }));

        // bad channel
        expect(throws<std::invalid_argument>([&flow_graph] {
            auto &ps = flow_graph.make_node<Picoscope4000a>();
            ps.set_channel_configuration({ { "INVALID", { .range = 5., .coupling = coupling_t::AC_1M } } });
        }));

        expect(nothrow([&flow_graph] { std::ignore = flow_graph.make_node<Picoscope4000a>(); }));
    };

    "streaming basics"_test = [] {
        graph::graph     flow_graph;

        constexpr double sample_rate = 80000.;
        constexpr auto   duration_ms = 2000;

        auto            &ps          = flow_graph.make_node<Picoscope4000a>(
                { { { "sample_rate", sample_rate }, { "acquisition_mode_string", "STREAMING" }, { "streaming_mode_poll_rate", 0.00001 }, { "auto_arm", true } } });
        ps.set_channel_configuration({ { "A", { .range = 5., .coupling = coupling_t::AC_1M } } });

        auto &sink    = flow_graph.make_node<count_sink<float>>();
        auto &errsink = flow_graph.make_node<count_sink<float>>();

        expect(eq(connection_result_t::SUCCESS, flow_graph.connect<"values0">(ps).template to<"in">(sink)));
        expect(eq(connection_result_t::SUCCESS, flow_graph.connect<"errors0">(ps).template to<"in">(errsink)));

        // Explicitly start unit because it takes quite some time
        expect(nothrow([&ps] { ps.start(); }));

        // This either hangs or terminates without producing anything if I increase the number of threads
        constexpr std::size_t min_threads = 2;
        constexpr std::size_t max_threads = 2;
        scheduler::simple<scheduler::multi_threaded> sched{ std::move(flow_graph), std::make_shared<fair::thread_pool::BasicThreadPool>("simple-scheduler-pool", fair::thread_pool::CPU_BOUND, min_threads, max_threads) };
        sched.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
        ps.force_quit(); // needed, otherwise stop() doesn't terminate (no matter if the PS works blocking or not)
        sched.stop();

        const auto measured_rate = static_cast<double>(sink.samples_seen) * 1000. / duration_ms;
        fmt::println("Produced in worker: {}", ps.produced_worker());
        fmt::println("Configured rate: {}, Measured rate: {} ({:.2f}%), Duration: {} ms", sample_rate, static_cast<std::size_t>(measured_rate), measured_rate / sample_rate * 100., duration_ms);
        fmt::println("Total: {}", sink.samples_seen);

        expect(eq(sink.samples_seen, errsink.samples_seen));
        expect(ge(sink.samples_seen, std::size_t{ 80000 }));
        expect(le(sink.samples_seen, std::size_t{ 160000 }));
    };

    "rapid block basics"_test            = [] { test_rapid_block_basic(1); };

    "rapid block multiple captures"_test = [] { test_rapid_block_basic(3); };

    "rapid block 4 channels"_test        = [] {
        constexpr std::size_t pre_samples   = 33;
        constexpr std::size_t post_samples  = 1000;
        constexpr std::size_t nr_captures   = 2;
        constexpr auto        total_samples = nr_captures * (pre_samples + post_samples);

        graph::graph          flow_graph;
        auto                 &ps = flow_graph.make_node<Picoscope4000a>({ { { "sample_rate", 10000. },
                                                                            { "pre_samples", pre_samples },
                                                                            { "post_samples", post_samples },
                                                                            { "acquisition_mode_string", "RAPID_BLOCK" },
                                                                            { "rapid_block_nr_captures", nr_captures },
                                                                            { "auto_arm", true },
                                                                            { "trigger_once", true } } });
        ps.set_channel_configuration({ { { "A", { .range = 5., .coupling = coupling_t::AC_1M } },
                                         { "B", { .range = 5., .coupling = coupling_t::AC_1M } },
                                         { "C", { .range = 5., .coupling = coupling_t::AC_1M } },
                                         { "D", { .range = 5., .coupling = coupling_t::AC_1M } } } });
        auto &sink0 = flow_graph.make_node<count_sink<float>>();
        auto &sink1 = flow_graph.make_node<count_sink<float>>();
        auto &sink2 = flow_graph.make_node<count_sink<float>>();
        auto &sink3 = flow_graph.make_node<count_sink<float>>();

        expect(eq(connection_result_t::SUCCESS, flow_graph.connect<"values0">(ps).template to<"in">(sink0)));
        expect(eq(connection_result_t::SUCCESS, flow_graph.connect<"values1">(ps).template to<"in">(sink1)));
        expect(eq(connection_result_t::SUCCESS, flow_graph.connect<"values2">(ps).template to<"in">(sink2)));
        expect(eq(connection_result_t::SUCCESS, flow_graph.connect<"values3">(ps).template to<"in">(sink3)));

        scheduler::simple sched{ std::move(flow_graph) };
        sched.run_and_wait();

        expect(eq(sink0.samples_seen, total_samples));
        expect(eq(sink1.samples_seen, total_samples));
        expect(eq(sink2.samples_seen, total_samples));
        expect(eq(sink3.samples_seen, total_samples));
    };

    "rapid block continuous"_test = [] {
        graph::graph flow_graph;
        auto &ps = flow_graph.make_node<Picoscope4000a>(
                { { { "sample_rate", 10000. }, { "post_samples", 1000 }, { "acquisition_mode_string", "RAPID_BLOCK" }, { "rapid_block_nr_captures", 1 }, { "auto_arm", true } } });

        ps.set_channel_configuration({ { "A", { .range = 5., .coupling = coupling_t::AC_1M } } });
        auto &sink0 = flow_graph.make_node<count_sink<float>>();

        expect(eq(connection_result_t::SUCCESS, flow_graph.connect<"values0">(ps).template to<"in">(sink0)));

        ps.start();

        // TODO tried multi_threaded scheduler with start(); sleep; stop(), something goes
        // wrong there (scheduler doesn't shovel data reliably)
        scheduler::simple sched{ std::move(flow_graph) };
        auto              quitter = std::async([&ps] {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            ps.force_quit();
        });

        sched.run_and_wait();
        expect(ge(sink0.samples_seen, std::size_t{ 3000 }));
        expect(le(sink0.samples_seen, std::size_t{ 15000 }));
    };
};

} // namespace fair::picoscope4000a::test

int
main() { /* tests are statically executed */
}
