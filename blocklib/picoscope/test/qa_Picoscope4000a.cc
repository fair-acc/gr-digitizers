#include <boost/ut.hpp>

#include <gnuradio-4.0/Scheduler.hpp>

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

    constexpr std::size_t kPreSamples  = 33;
    constexpr std::size_t kPostSamples = 1000;
    const auto            totalSamples = nrCaptures * (kPreSamples + kPostSamples);

    Graph                 flowGraph;
    auto                 &ps   = flowGraph.emplaceBlock<Picoscope4000a<T>>({ { { "sample_rate", 10000. },
                                                                               { "pre_samples", kPreSamples },
                                                                               { "post_samples", kPostSamples },
                                                                               { "acquisition_mode", "RapidBlock" },
                                                                               { "rapid_block_nr_captures", nrCaptures },
                                                                               { "auto_arm", true },
                                                                               { "trigger_once", true },
                                                                               { "channel_ids", std::vector<std::string>{ "A" } },
                                                                               { "channel_ranges", std::vector{ 5. } },
                                                                               { "channel_couplings", std::vector<std::string>{ "AC_1M" } } } });

    auto                 &sink = flowGraph.emplaceBlock<CountSink<T>>();

    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"analog_out", 0>(ps).template to<"in">(sink)));

    scheduler::Simple sched{ std::move(flowGraph) };
    sched.runAndWait();

    expect(eq(sink.samples_seen, totalSamples));
}

template<typename T>
void
testStreamingBasics() {
    using namespace std::chrono;
    using namespace boost::ut;
    using namespace gr;
    using namespace fair::helpers;
    using namespace fair::picoscope;
    Graph            flowGraph;

    constexpr double kSampleRate = 80000.;
    constexpr auto   kDuration   = seconds(2);

    auto            &ps          = flowGraph.emplaceBlock<Picoscope4000a<T>>({ { { "sample_rate", kSampleRate },
                                                                                 { "acquisition_mode", "Streaming" },
                                                                                 { "streaming_mode_poll_rate", 0.00001 },
                                                                                 { "auto_arm", true },
                                                                                 { "channel_ids", std::vector<std::string>{ "A" } },
                                                                                 { "channel_names", std::vector<std::string>{ "Test signal" } },
                                                                                 { "channel_units", std::vector<std::string>{ "Test unit" } },
                                                                                 { "channel_ranges", std::vector{ 5. } },
                                                                                 { "channel_couplings", std::vector<std::string>{ "AC_1M" } } } });

    auto            &tagTracker  = flowGraph.emplaceBlock<TagDebug<T>>();
    auto            &sink        = flowGraph.emplaceBlock<CountSink<T>>();

    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"analog_out", 0>(ps).template to<"in">(tagTracker)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out">(tagTracker).template to<"in">(sink)));

    // Explicitly start unit because it takes quite some time
    expect(nothrow([&ps] { ps.start(); }));

    // This either hangs or terminates without producing anything if I increase the number of threads
    constexpr std::size_t                                        kMinThreads = 2;
    constexpr std::size_t                                        kMaxThreads = 2;
    scheduler::Simple<scheduler::ExecutionPolicy::multiThreaded> sched{ std::move(flowGraph),
                                                                        std::make_shared<thread_pool::BasicThreadPool>("simple-scheduler-pool", thread_pool::CPU_BOUND, kMinThreads, kMaxThreads) };
    sched.start();
    std::this_thread::sleep_for(kDuration);
    ps.forceQuit(); // needed, otherwise stop() doesn't terminate (no matter if the PS works blocking or not)
    sched.stop();

    const auto measuredRate = sink.samples_seen / duration<double>(kDuration).count();
    fmt::println("Produced in worker: {}", ps.producedWorker());
    fmt::println("Configured rate: {}, Measured rate: {} ({:.2f}%), Duration: {} ms", kSampleRate, static_cast<std::size_t>(measuredRate), measuredRate / kSampleRate * 100.,
                 duration_cast<milliseconds>(kDuration).count());
    fmt::println("Total: {}", sink.samples_seen);

    expect(ge(sink.samples_seen, std::size_t{ 80000 }));
    expect(le(sink.samples_seen, std::size_t{ 160000 }));
    expect(eq(tagTracker.seen_tags.size(), std::size_t{ 1 }));
    const auto &tag = tagTracker.seen_tags[0];
    expect(eq(tag.index, int64_t{ 0 }));
    expect(eq(std::get<float>(tag.at(std::string(tag::SAMPLE_RATE.key()))), static_cast<float>(kSampleRate)));
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
        Graph flowGraph;
        auto &ps = flowGraph.emplaceBlock<Picoscope4000a<float>>();

        // this takes time, so we do it a few times only
        for (auto i = 0; i < 3; i++) {
            expect(nothrow([&ps] { ps.initialize(); }));
            expect(neq(ps.driverVersion(), std::string()));
            expect(neq(ps.hardwareVersion(), std::string()));
            expect(nothrow([&ps] { ps.close(); })) << "doesn't throw";
        }
    };

    "streaming basics"_test = [] {
        testStreamingBasics<float>();
        testStreamingBasics<double>();
        testStreamingBasics<int16_t>();
    };

    "rapid block basics"_test = [] {
        testRapidBlockBasic<float>(1);
        testRapidBlockBasic<double>(1);
        testRapidBlockBasic<int16_t>(1);
    };

    "rapid block multiple captures"_test = [] { testRapidBlockBasic<float>(3); };

    "rapid block 4 channels"_test        = [] {
        constexpr std::size_t kPreSamples   = 33;
        constexpr std::size_t kPostSamples  = 1000;
        constexpr std::size_t kNrCaptures   = 2;
        constexpr auto        kTotalSamples = kNrCaptures * (kPreSamples + kPostSamples);

        Graph                 flowGraph;
        auto                 &ps    = flowGraph.emplaceBlock<Picoscope4000a<float>>({ { { "sample_rate", 10000. },
                                                                                        { "pre_samples", kPreSamples },
                                                                                        { "post_samples", kPostSamples },
                                                                                        { "acquisition_mode", "RapidBlock" },
                                                                                        { "rapid_block_nr_captures", kNrCaptures },
                                                                                        { "auto_arm", true },
                                                                                        { "trigger_once", true },
                                                                                        { "channel_ids", std::vector<std::string>{ "A", "B", "C", "D" } },
                                                                                        { "channel_ranges", std::vector{ { 5., 5., 5., 5. } } },
                                                                                        { "channel_couplings", std::vector<std::string>{ "AC_1M", "AC_1M", "AC_1M", "AC_1M" } } } });

        auto                 &sink0 = flowGraph.emplaceBlock<CountSink<float>>();
        auto                 &sink1 = flowGraph.emplaceBlock<CountSink<float>>();
        auto                 &sink2 = flowGraph.emplaceBlock<CountSink<float>>();
        auto                 &sink3 = flowGraph.emplaceBlock<CountSink<float>>();

        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"analog_out", 0>(ps).to<"in">(sink0)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"analog_out", 1>(ps).to<"in">(sink1)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"analog_out", 2>(ps).to<"in">(sink2)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"analog_out", 3>(ps).to<"in">(sink3)));

        scheduler::Simple sched{ std::move(flowGraph) };
        sched.runAndWait();

        expect(eq(sink0.samples_seen, kTotalSamples));
        expect(eq(sink1.samples_seen, kTotalSamples));
        expect(eq(sink2.samples_seen, kTotalSamples));
        expect(eq(sink3.samples_seen, kTotalSamples));
    };

    "rapid block continuous"_test = [] {
        Graph flowGraph;
        auto &ps    = flowGraph.emplaceBlock<Picoscope4000a<float>>({ { { "sample_rate", 10000. },
                                                                        { "post_samples", std::size_t{ 1000 } },
                                                                        { "acquisition_mode", "RapidBlock" },
                                                                        { "rapid_block_nr_captures", std::size_t{ 1 } },
                                                                        { "auto_arm", true },
                                                                        { "channel_ids", std::vector<std::string>{ "A" } },
                                                                        { "channel_ranges", std::vector{ 5. } },
                                                                        { "channel_couplings", std::vector<std::string>{ "AC_1M" } } } });

        auto &sink0 = flowGraph.emplaceBlock<CountSink<float>>();

        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"analog_out", 0>(ps).to<"in">(sink0)));

        ps.start();

        // TODO tried multi_threaded scheduler with start(); sleep; stop(), something goes
        // wrong there (scheduler doesn't shovel data reliably)
        scheduler::Simple sched{ std::move(flowGraph) };
        auto              quitter = std::async([&ps] {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            ps.forceQuit();
        });

        sched.runAndWait();
        expect(ge(sink0.samples_seen, std::size_t{ 2000 }));
        expect(le(sink0.samples_seen, std::size_t{ 10000 }));
    };
};

} // namespace fair::picoscope4000a::test

int
main() { /* tests are statically executed */
}
