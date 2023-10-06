#include <boost/ut.hpp>

#include <helper_blocks.hpp>
#include <picoscope4000a.hpp>

#include <scheduler.hpp>

using namespace std::string_literals;

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

    fair::graph::graph    flow_graph;
    auto                 &ps      = flow_graph.make_node<Picoscope4000a>({ { { "sample_rate", 10000. },
                                                                             { "pre_samples", pre_samples },
                                                                             { "post_samples", post_samples },
                                                                             { "acquisition_mode", "RAPID_BLOCK" },
                                                                             { "rapid_block_nr_captures", nr_captures },
                                                                             { "auto_arm", true },
                                                                             { "trigger_once", true },
                                                                             { "channel_ids", "A" },
                                                                             { "channel_ranges", std::vector{ { 5. } } },
                                                                             { "channel_couplings", "AC_1M" } } });

    auto                 &sink    = flow_graph.make_node<count_sink<float>>();
    auto                 &errsink = flow_graph.make_node<count_sink<float>>();

    // TODO move back to static connect() once it can handle arrays
    expect(eq(connection_result_t::SUCCESS, flow_graph.dynamic_connect(ps, 0, sink, 0)));
    expect(eq(connection_result_t::SUCCESS, flow_graph.dynamic_connect(ps, /*errors[0]*/ 8, errsink, 0)));

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
        fair::graph::graph flow_graph;
        auto              &ps = flow_graph.make_node<Picoscope4000a>();

        // this takes time, so we do it a few times only
        for (auto i = 0; i < 3; i++) {
            expect(nothrow([&ps] { ps.initialize(); }));
            expect(neq(ps.driver_version(), std::string()));
            expect(neq(ps.hardware_version(), std::string()));
            expect(nothrow([&ps] { ps.close(); })) << "doesn't throw";
        }
    };

    "streaming basics"_test = [] {
        fair::graph::graph flow_graph;

        constexpr double   sample_rate = 80000.;
        constexpr auto     duration_ms = 2000;

        auto              &ps          = flow_graph.make_node<Picoscope4000a>({ { { "sample_rate", sample_rate },
                                                                                  { "acquisition_mode", "STREAMING" },
                                                                                  { "streaming_mode_poll_rate", 0.00001 },
                                                                                  { "auto_arm", true },
                                                                                  { "channel_ids", "A" },
                                                                                  { "channel_names", "Test signal" },
                                                                                  { "channel_units", "Test unit" },
                                                                                  { "channel_ranges", std::vector{ { 5. } } },
                                                                                  { "channel_couplings", "AC_1M" } } });

        auto              &tag_tracker = flow_graph.make_node<tag_debug<float>>();
        auto              &sink        = flow_graph.make_node<count_sink<float>>();
        auto              &errsink     = flow_graph.make_node<count_sink<float>>();

        // TODO move back to static connect() once it can handle arrays
        expect(eq(connection_result_t::SUCCESS, flow_graph.dynamic_connect(ps, 0, tag_tracker, 0)));
        expect(eq(connection_result_t::SUCCESS, flow_graph.connect<"out">(tag_tracker).template to<"in">(sink)));
        expect(eq(connection_result_t::SUCCESS, flow_graph.dynamic_connect(ps, /*errors[0]*/ 8, errsink, 0)));

        // Explicitly start unit because it takes quite some time
        expect(nothrow([&ps] { ps.start(); }));

        // This either hangs or terminates without producing anything if I increase the number of threads
        constexpr std::size_t                        min_threads = 2;
        constexpr std::size_t                        max_threads = 2;
        scheduler::simple<scheduler::multi_threaded> sched{ std::move(flow_graph),
                                                            std::make_shared<fair::thread_pool::BasicThreadPool>("simple-scheduler-pool", fair::thread_pool::CPU_BOUND, min_threads, max_threads) };
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
        expect(eq(tag_tracker.seen_tags.size(), std::size_t{ 1 }));
        const auto &tag = tag_tracker.seen_tags[0];
        expect(eq(tag.index, int64_t{ 0 }));
        expect(eq(std::get<float>(tag.at(std::string(tag::SAMPLE_RATE.key()))), static_cast<float>(sample_rate)));
        expect(eq(std::get<std::string>(tag.at(std::string(tag::SIGNAL_NAME.key()))), "Test signal"s));
        expect(eq(std::get<std::string>(tag.at(std::string(tag::SIGNAL_UNIT.key()))), "Test unit"s));
        expect(eq(std::get<float>(tag.at(std::string(tag::SIGNAL_MIN.key()))), 0.f));
        expect(eq(std::get<float>(tag.at(std::string(tag::SIGNAL_MAX.key()))), 5.f));
    };

    "rapid block basics"_test            = [] { test_rapid_block_basic(1); };

    "rapid block multiple captures"_test = [] { test_rapid_block_basic(3); };

    "rapid block 4 channels"_test        = [] {
        constexpr std::size_t pre_samples   = 33;
        constexpr std::size_t post_samples  = 1000;
        constexpr std::size_t nr_captures   = 2;
        constexpr auto        total_samples = nr_captures * (pre_samples + post_samples);

        fair::graph::graph    flow_graph;
        auto                 &ps    = flow_graph.make_node<Picoscope4000a>({ { { "sample_rate", 10000. },
                                                                               { "pre_samples", pre_samples },
                                                                               { "post_samples", post_samples },
                                                                               { "acquisition_mode", "RAPID_BLOCK" },
                                                                               { "rapid_block_nr_captures", nr_captures },
                                                                               { "auto_arm", true },
                                                                               { "trigger_once", true },
                                                                               { "channel_ids", "A,B,C,D" },
                                                                               { "channel_ranges", std::vector{ { 5., 5., 5., 5. } } },
                                                                               { "channel_couplings", "AC_1M,AC_1M,AC_1M,AC_1M" } } });

        auto                 &sink0 = flow_graph.make_node<count_sink<float>>();
        auto                 &sink1 = flow_graph.make_node<count_sink<float>>();
        auto                 &sink2 = flow_graph.make_node<count_sink<float>>();
        auto                 &sink3 = flow_graph.make_node<count_sink<float>>();

        // TODO move back to static connect() once it can handle arrays
        expect(eq(connection_result_t::SUCCESS, flow_graph.dynamic_connect(ps, 0, sink0, 0)));
        expect(eq(connection_result_t::SUCCESS, flow_graph.dynamic_connect(ps, 1, sink1, 0)));
        expect(eq(connection_result_t::SUCCESS, flow_graph.dynamic_connect(ps, 2, sink2, 0)));
        expect(eq(connection_result_t::SUCCESS, flow_graph.dynamic_connect(ps, 3, sink3, 0)));

        scheduler::simple sched{ std::move(flow_graph) };
        sched.run_and_wait();

        expect(eq(sink0.samples_seen, total_samples));
        expect(eq(sink1.samples_seen, total_samples));
        expect(eq(sink2.samples_seen, total_samples));
        expect(eq(sink3.samples_seen, total_samples));
    };

    "rapid block continuous"_test = [] {
        fair::graph::graph flow_graph;
        auto              &ps    = flow_graph.make_node<Picoscope4000a>({ { { "sample_rate", 10000. },
                                                                            { "post_samples", std::size_t{ 1000 } },
                                                                            { "acquisition_mode", "RAPID_BLOCK" },
                                                                            { "rapid_block_nr_captures", std::size_t{ 1 } },
                                                                            { "auto_arm", true },
                                                                            { "channel_ids", "A" },
                                                                            { "channel_ranges", std::vector{ { 5. } } },
                                                                            { "channel_couplings", "AC_1M" } } });

        auto              &sink0 = flow_graph.make_node<count_sink<float>>();

        expect(eq(connection_result_t::SUCCESS, flow_graph.dynamic_connect(ps, 0, sink0, 0)));

        ps.start();

        // TODO tried multi_threaded scheduler with start(); sleep; stop(), something goes
        // wrong there (scheduler doesn't shovel data reliably)
        scheduler::simple sched{ std::move(flow_graph) };
        auto              quitter = std::async([&ps] {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            ps.force_quit();
        });

        sched.run_and_wait();
        expect(ge(sink0.samples_seen, std::size_t{ 2000 }));
        expect(le(sink0.samples_seen, std::size_t{ 10000 }));
    };
};

} // namespace fair::picoscope4000a::test

int
main() { /* tests are statically executed */
}
