#include <boost/ut.hpp>

#include <gnuradio-4.0/scheduler.hpp>

#include <HelperBlocks.hpp>
#include <Picoscope4000a.hpp>

using namespace std::string_literals;

namespace fair::picoscope4000a::test {

template<typename T>
void
testRapidBlockBasic(std::size_t nrCaptures) {
    using namespace boost::ut;
    using namespace gr;
    using namespace fair::helpers;
    using namespace fair::picoscope;

    constexpr std::size_t PRE_SAMPLES  = 33;
    constexpr std::size_t POST_SAMPLES = 1000;
    const auto            totalSamples = nrCaptures * (PRE_SAMPLES + POST_SAMPLES);

    gr::graph             flowGraph;
    auto                 &ps   = flowGraph.make_node<Picoscope4000a<T>>({ { { "sample_rate", 10000. },
                                                                            { "pre_samples", PRE_SAMPLES },
                                                                            { "post_samples", POST_SAMPLES },
                                                                            { "acquisition_mode", "RAPID_BLOCK" },
                                                                            { "rapid_block_nr_captures", nrCaptures },
                                                                            { "auto_arm", true },
                                                                            { "trigger_once", true },
                                                                            { "channel_ids", std::vector<std::string>{ "A" } },
                                                                            { "channel_ranges", std::vector{ 5. } },
                                                                            { "channel_couplings", std::vector<std::string>{ "AC_1M" } } } });

    auto                 &sink = flowGraph.make_node<CountSink<T>>();

    // TODO move back to static connect() once it can handle arrays
    expect(eq(connection_result_t::SUCCESS, flowGraph.dynamic_connect(ps, 0, sink, 0)));

    scheduler::simple sched{ std::move(flowGraph) };
    sched.run_and_wait();

    expect(eq(sink.samples_seen, totalSamples));
}

template<typename T>
void
test_streaming_basics() {
    using namespace boost::ut;
    using namespace gr;
    using namespace fair::helpers;
    using namespace fair::picoscope;
    gr::graph        flowGraph;

    constexpr double SAMPLE_RATE = 80000.;
    constexpr auto   DURATION_MS = 2000;

    auto            &ps          = flowGraph.make_node<Picoscope4000a<T>>({ { { "sample_rate", SAMPLE_RATE },
                                                                              { "acquisition_mode", "STREAMING" },
                                                                              { "streaming_mode_poll_rate", 0.00001 },
                                                                              { "auto_arm", true },
                                                                              { "channel_ids", std::vector<std::string>{ "A" } },
                                                                              { "channel_names", std::vector<std::string>{ "Test signal" } },
                                                                              { "channel_units", std::vector<std::string>{ "Test unit" } },
                                                                              { "channel_ranges", std::vector{ 5. } },
                                                                              { "channel_couplings", std::vector<std::string>{ "AC_1M" } } } });

    auto            &tagTracker  = flowGraph.make_node<TagDebug<T>>();
    auto            &sink        = flowGraph.make_node<CountSink<T>>();

    // TODO move back to static connect() once it can handle arrays
    expect(eq(connection_result_t::SUCCESS, flowGraph.dynamic_connect(ps, 0, tagTracker, 0)));
    expect(eq(connection_result_t::SUCCESS, flowGraph.connect<"out">(tagTracker).template to<"in">(sink)));

    // Explicitly start unit because it takes quite some time
    expect(nothrow([&ps] { ps.start(); }));

    // This either hangs or terminates without producing anything if I increase the number of threads
    constexpr std::size_t                        MIN_THREADS = 2;
    constexpr std::size_t                        MAX_THREADS = 2;
    scheduler::simple<scheduler::multi_threaded> sched{ std::move(flowGraph),
                                                        std::make_shared<thread_pool::BasicThreadPool>("simple-scheduler-pool", thread_pool::CPU_BOUND, MIN_THREADS, MAX_THREADS) };
    sched.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(DURATION_MS));
    ps.forceQuit(); // needed, otherwise stop() doesn't terminate (no matter if the PS works blocking or not)
    sched.stop();

    const auto measuredRate = static_cast<double>(sink.samples_seen) * 1000. / DURATION_MS;
    fmt::println("Produced in worker: {}", ps.producedWorker());
    fmt::println("Configured rate: {}, Measured rate: {} ({:.2f}%), Duration: {} ms", SAMPLE_RATE, static_cast<std::size_t>(measuredRate), measuredRate / SAMPLE_RATE * 100., DURATION_MS);
    fmt::println("Total: {}", sink.samples_seen);

    expect(ge(sink.samples_seen, std::size_t{ 80000 }));
    expect(le(sink.samples_seen, std::size_t{ 160000 }));
    expect(eq(tagTracker.seen_tags.size(), std::size_t{ 1 }));
    const auto &tag = tagTracker.seen_tags[0];
    expect(eq(tag.index, int64_t{ 0 }));
    expect(eq(std::get<float>(tag.at(std::string(tag::SAMPLE_RATE.key()))), static_cast<float>(SAMPLE_RATE)));
    expect(eq(std::get<std::string>(tag.at(std::string(tag::SIGNAL_NAME.key()))), "Test signal"s));
    expect(eq(std::get<std::string>(tag.at(std::string(tag::SIGNAL_UNIT.key()))), "Test unit"s));
    expect(eq(std::get<float>(tag.at(std::string(tag::SIGNAL_MIN.key()))), 0.f));
    expect(eq(std::get<float>(tag.at(std::string(tag::SIGNAL_MAX.key()))), 5.f));
}

const boost::ut::suite Picoscope4000aTests = [] {
    using namespace boost::ut;
    using namespace gr;
    using namespace fair::helpers;
    using namespace fair::picoscope;

    "open and close"_test = [] {
        gr::graph flowGraph;
        auto     &ps = flowGraph.make_node<Picoscope4000a<float>>();

        // this takes time, so we do it a few times only
        for (auto i = 0; i < 3; i++) {
            expect(nothrow([&ps] { ps.initialize(); }));
            expect(neq(ps.driverVersion(), std::string()));
            expect(neq(ps.hardwareVersion(), std::string()));
            expect(nothrow([&ps] { ps.close(); })) << "doesn't throw";
        }
    };

    "streaming basics"_test = [] {
        test_streaming_basics<float>();
        test_streaming_basics<int16_t>();
    };

    "rapid block basics"_test = [] {
        testRapidBlockBasic<float>(1);
        testRapidBlockBasic<int16_t>(1);
    };

    "rapid block multiple captures"_test = [] { testRapidBlockBasic<float>(3); };

    "rapid block 4 channels"_test        = [] {
        constexpr std::size_t PRE_SAMPLES  = 33;
        constexpr std::size_t POST_SAMPLES = 1000;
        constexpr std::size_t NR_CAPTURES  = 2;
        constexpr auto        totalSamples = NR_CAPTURES * (PRE_SAMPLES + POST_SAMPLES);

        gr::graph             flowGraph;
        auto                 &ps    = flowGraph.make_node<Picoscope4000a<float>>({ { { "sample_rate", 10000. },
                                                                                     { "pre_samples", PRE_SAMPLES },
                                                                                     { "post_samples", POST_SAMPLES },
                                                                                     { "acquisition_mode", "RAPID_BLOCK" },
                                                                                     { "rapid_block_nr_captures", NR_CAPTURES },
                                                                                     { "auto_arm", true },
                                                                                     { "trigger_once", true },
                                                                                     { "channel_ids", std::vector<std::string>{ "A", "B", "C", "D" } },
                                                                                     { "channel_ranges", std::vector{ { 5., 5., 5., 5. } } },
                                                                                     { "channel_couplings", std::vector<std::string>{ "AC_1M", "AC_1M", "AC_1M", "AC_1M" } } } });

        auto                 &sink0 = flowGraph.make_node<CountSink<float>>();
        auto                 &sink1 = flowGraph.make_node<CountSink<float>>();
        auto                 &sink2 = flowGraph.make_node<CountSink<float>>();
        auto                 &sink3 = flowGraph.make_node<CountSink<float>>();

        // TODO move back to static connect() once it can handle arrays
        expect(eq(connection_result_t::SUCCESS, flowGraph.dynamic_connect(ps, 0, sink0, 0)));
        expect(eq(connection_result_t::SUCCESS, flowGraph.dynamic_connect(ps, 1, sink1, 0)));
        expect(eq(connection_result_t::SUCCESS, flowGraph.dynamic_connect(ps, 2, sink2, 0)));
        expect(eq(connection_result_t::SUCCESS, flowGraph.dynamic_connect(ps, 3, sink3, 0)));

        scheduler::simple sched{ std::move(flowGraph) };
        sched.run_and_wait();

        expect(eq(sink0.samples_seen, totalSamples));
        expect(eq(sink1.samples_seen, totalSamples));
        expect(eq(sink2.samples_seen, totalSamples));
        expect(eq(sink3.samples_seen, totalSamples));
    };

    "rapid block continuous"_test = [] {
        gr::graph flowGraph;
        auto     &ps    = flowGraph.make_node<Picoscope4000a<float>>({ { { "sample_rate", 10000. },
                                                                         { "post_samples", std::size_t{ 1000 } },
                                                                         { "acquisition_mode", "RAPID_BLOCK" },
                                                                         { "rapid_block_nr_captures", std::size_t{ 1 } },
                                                                         { "auto_arm", true },
                                                                         { "channel_ids", std::vector<std::string>{ "A" } },
                                                                         { "channel_ranges", std::vector{ 5. } },
                                                                         { "channel_couplings", std::vector<std::string>{ "AC_1M" } } } });

        auto     &sink0 = flowGraph.make_node<CountSink<float>>();

        expect(eq(connection_result_t::SUCCESS, flowGraph.dynamic_connect(ps, 0, sink0, 0)));

        ps.start();

        // TODO tried multi_threaded scheduler with start(); sleep; stop(), something goes
        // wrong there (scheduler doesn't shovel data reliably)
        scheduler::simple sched{ std::move(flowGraph) };
        auto              quitter = std::async([&ps] {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            ps.forceQuit();
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
