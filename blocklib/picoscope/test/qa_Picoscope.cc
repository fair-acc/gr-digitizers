#include <boost/ut.hpp>

#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/testing/TagMonitors.hpp>

#include <HelperBlocks.hpp>
#include <Picoscope4000a.hpp>
#include <Picoscope5000a.hpp>

using namespace std::string_literals;

namespace fair::picoscope::test {

// Replace with your connected Picoscope device
template<typename T>
using PicoscopeT = Picoscope5000a<T>;

static_assert(gr::HasProcessBulkFunction<PicoscopeT<float>>);
static_assert(gr::HasProcessBulkFunction<PicoscopeT<float>>);

template<gr::DataSetLike T>
void testRapidBlockBasic(std::size_t nCaptures) {
    using namespace boost::ut;
    using namespace gr;
    using namespace fair::helpers;
    using namespace fair::picoscope;

    fmt::println("testRapidBlockBasic({}) - {}", nCaptures, gr::meta::type_name<T>());

    constexpr float      sampleRate       = 1234567.f;
    constexpr float      actualSampleRate = 1250000.f; // for Picoscope4000a
    constexpr gr::Size_t preSamples       = 33;
    constexpr gr::Size_t postSamples      = 1000;
    const gr::Size_t     totalSamples     = preSamples + postSamples;

    Graph flowGraph;
    auto& ps = flowGraph.emplaceBlock<PicoscopeT<T>>({{{"disconnect_on_done", true}, {"sample_rate", sampleRate}, {"pre_samples", preSamples}, {"post_samples", postSamples}, {"n_captures", nCaptures}, //
        {"auto_arm", true}, {"trigger_once", true}, {"channel_ids", std::vector<std::string>{"A"}}, {"channel_ranges", std::vector<float>{5.f}},                                                         //
        {"channel_couplings", std::vector<std::string>{"AC_1M"}}}});

    auto& sinkA = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", true}, {"log_tags", false}}});
    auto& sinkB = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", true}, {"log_tags", false}}});
    auto& sinkC = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
    auto& sinkD = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});

    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).template to<"in">(sinkA)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkD)));

    if constexpr (std::is_same_v<Picoscope4000a<T>, PicoscopeT<T>>) {
        auto& sinkE = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
        auto& sinkF = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
        auto& sinkG = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
        auto& sinkH = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});

        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 4>(ps).template to<"in">(sinkE)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 5>(ps).template to<"in">(sinkF)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 6>(ps).template to<"in">(sinkG)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 7>(ps).template to<"in">(sinkH)));
    }

    scheduler::Simple sched{std::move(flowGraph)};
    expect(sched.runAndWait().has_value());

    if constexpr (std::is_same_v<Picoscope4000a<T>, PicoscopeT<T>>) {
        expect(eq(ps._actualSampleRate, actualSampleRate));
    }

    expect(eq(sinkA._nSamplesProduced, nCaptures)); // number of DataSets
    expect(eq(sinkA._samples.size(), nCaptures));   // number of DataSets
    if (sinkA._samples.size() >= nCaptures) {
        for (std::size_t iC = 0; iC < sinkA._samples.size(); iC++) {
            expect(eq(sinkA._samples[iC].signal_values.size(), totalSamples));
        }
    }

    expect(eq(sinkB._nSamplesProduced, nCaptures)); // empty DataSets are published if channel is not active, TODO: this logic can be changed later
    expect(eq(sinkB._samples.size(), nCaptures));
    if (sinkB._samples.size() >= nCaptures) {
        for (std::size_t iC = 0; iC < sinkB._samples.size(); iC++) {
            expect(eq(sinkB._samples[iC].signal_values.size(), 0UZ));
        }
    }
}

template<typename T>
void testStreamingBasics() {
    using namespace std::chrono;
    using namespace std::chrono_literals;
    using namespace boost::ut;
    using namespace gr;
    using namespace fair::helpers;
    using namespace fair::picoscope;
    Graph flowGraph;

    fmt::println("testStreamingBasics - {}", gr::meta::type_name<T>());

    constexpr float sampleRate   = 83000.f;
    constexpr auto  testDuration = 2s;

    auto& ps = flowGraph.emplaceBlock<PicoscopeT<T>>({{{"sample_rate", sampleRate},                                                                                      //
        {"streaming_mode_poll_rate", 0.00001f}, {"auto_arm", true}, {"channel_ids", std::vector<std::string>{"A"}},                                                      //
        {"signal_names", std::vector<std::string>{"Test signal"}}, {"signal_units", std::vector<std::string>{"Test unit"}}, {"channel_ranges", std::vector<float>{5.f}}, //
        {"channel_couplings", std::vector<std::string>{"AC_1M"}}}});

    auto& tagMonitor = flowGraph.emplaceBlock<testing::TagMonitor<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", true}}});
    auto& sinkA      = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
    auto& sinkB      = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
    auto& sinkC      = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
    auto& sinkD      = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});

    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).template to<"in">(tagMonitor)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out">(tagMonitor).template to<"in">(sinkA)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkD)));

    if constexpr (std::is_same_v<Picoscope4000a<T>, PicoscopeT<T>>) {
        auto& sinkE = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
        auto& sinkF = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
        auto& sinkG = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
        auto& sinkH = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});

        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 4>(ps).template to<"in">(sinkE)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 5>(ps).template to<"in">(sinkF)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 6>(ps).template to<"in">(sinkG)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 7>(ps).template to<"in">(sinkH)));
    }

    // Explicitly start unit because it takes quite some time
    expect(nothrow([&ps] { ps.start(); }));

    scheduler::Simple<scheduler::ExecutionPolicy::multiThreaded> sched{std::move(flowGraph)};
    expect(sched.changeStateTo(lifecycle::State::INITIALISED).has_value());
    expect(sched.changeStateTo(lifecycle::State::RUNNING).has_value());
    std::this_thread::sleep_for(testDuration);
    expect(sched.changeStateTo(lifecycle::State::REQUESTED_STOP).has_value());

    const auto measuredRate = static_cast<double>(sinkA._nSamplesProduced) / duration<double>(testDuration).count();
    fmt::println("Produced in worker: {}", ps._nSamplesPublished);
    fmt::println("Configured rate: {}, Measured rate: {} ({:.2f}%), Duration: {} ms", sampleRate, static_cast<std::size_t>(measuredRate), measuredRate / static_cast<double>(sampleRate) * 100., duration_cast<milliseconds>(testDuration).count());
    fmt::println("Total samples: {}", sinkA._nSamplesProduced);
    fmt::println("Total tags: {}", tagMonitor._tags.size());

    expect(ge(sinkA._nSamplesProduced, 80000UZ));
    expect(le(sinkA._nSamplesProduced, 170000UZ));
    expect(ge(tagMonitor._tags.size(), 1UZ));
    if (tagMonitor._tags.size() == 1UZ) {
        const auto& tag = tagMonitor._tags[0];
        expect(eq(tag.index, int64_t{0}));
        expect(eq(std::get<float>(tag.at(std::string(tag::SAMPLE_RATE.shortKey()))), sampleRate));
        expect(eq(std::get<std::string>(tag.at(std::string(tag::SIGNAL_NAME.shortKey()))), "Test signal"s));
        expect(eq(std::get<std::string>(tag.at(std::string(tag::SIGNAL_UNIT.shortKey()))), "Test unit"s));
        expect(eq(std::get<float>(tag.at(std::string(tag::SIGNAL_MIN.shortKey()))), 0.f));
        expect(eq(std::get<float>(tag.at(std::string(tag::SIGNAL_MAX.shortKey()))), 5.f));
    }
}

const boost::ut::suite PicoscopeTests = [] {
    using namespace boost::ut;
    using namespace gr;
    using namespace fair::helpers;
    using namespace fair::picoscope;

    "open and close"_test = [] {
        Graph flowGraph;
        auto& ps = flowGraph.emplaceBlock<PicoscopeT<DataSet<float>>>();

        // this takes time, so we do it a few times only
        for (auto i = 0; i < 3; i++) {
            expect(nothrow([&ps] { ps.initialize(); }));
            expect(neq(ps.driverVersion(), std::string()));
            expect(neq(ps.hardwareVersion(), std::string()));
            expect(nothrow([&ps] { ps.stop(); })) << "doesn't throw";
        }
    };

    "streaming basics"_test = [] {
        testStreamingBasics<int16_t>();
        testStreamingBasics<float>();
        testStreamingBasics<gr::UncertainValue<float>>();
    };

    "rapid block basics"_test = [] {
        testRapidBlockBasic<gr::DataSet<int16_t>>(1);
        testRapidBlockBasic<gr::DataSet<float>>(1);
        testRapidBlockBasic<gr::DataSet<gr::UncertainValue<float>>>(1);
    };

    "rapid block multiple captures"_test = [] { testRapidBlockBasic<DataSet<float>>(3); };

    "rapid block 3 channels"_test = [] {
        constexpr gr::Size_t preSamples   = 33;
        constexpr gr::Size_t postSamples  = 1000;
        constexpr gr::Size_t nCaptures    = 5;
        constexpr gr::Size_t totalSamples = preSamples + postSamples;
        using T                           = DataSet<float>;

        Graph flowGraph;
        auto& ps = flowGraph.emplaceBlock<PicoscopeT<T>>({{{"sample_rate", 10000.f},                              //
            {"pre_samples", preSamples}, {"post_samples", postSamples}, {"n_captures", nCaptures},                //
            {"auto_arm", true}, {"trigger_once", true}, {"channel_ids", std::vector<std::string>{"A", "B", "C"}}, //
            {"channel_ranges", std::vector<float>{{5.f, 5.f, 5.f}}}, {"channel_couplings", std::vector<std::string>{"AC_1M", "AC_1M", "AC_1M"}}}});

        auto& sinkA = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", true}, {"log_tags", false}}});
        auto& sinkB = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", true}, {"log_tags", false}}});
        auto& sinkC = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", true}, {"log_tags", false}}});
        auto& sinkD = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", true}, {"log_tags", false}}});

        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).template to<"in">(sinkA)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkD)));

        if constexpr (std::is_same_v<Picoscope4000a<T>, PicoscopeT<T>>) {
            auto& sinkE = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
            auto& sinkF = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
            auto& sinkG = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
            auto& sinkH = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});

            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 4>(ps).template to<"in">(sinkE)));
            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 5>(ps).template to<"in">(sinkF)));
            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 6>(ps).template to<"in">(sinkG)));
            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 7>(ps).template to<"in">(sinkH)));
        }

        scheduler::Simple sched{std::move(flowGraph)};
        sched.runAndWait();

        expect(eq(sinkA._nSamplesProduced, nCaptures));
        expect(eq(sinkB._nSamplesProduced, nCaptures));
        expect(eq(sinkC._nSamplesProduced, nCaptures));
        expect(eq(sinkD._nSamplesProduced, nCaptures)); // empty DataSets

        const bool allSameSize = sinkA._samples.size() == sinkB._samples.size() && sinkB._samples.size() == sinkC._samples.size() && sinkC._samples.size() == sinkD._samples.size();
        expect(allSameSize);

        if (sinkA._samples.size() >= nCaptures) {
            for (std::size_t iC = 0; iC < sinkA._samples.size(); iC++) {
                expect(eq(sinkA._samples[iC].signal_values.size(), totalSamples));
                expect(eq(sinkB._samples[iC].signal_values.size(), totalSamples));
                expect(eq(sinkC._samples[iC].signal_values.size(), totalSamples));
                expect(eq(sinkD._samples[iC].signal_values.size(), 0UZ));
            }
        }
    };

    "rapid block continuous"_test = [] {
        using namespace std::chrono_literals;

        using T = DataSet<float>;

        constexpr float      sampleRate   = 1000.f;
        constexpr gr::Size_t postSamples  = 100;
        constexpr gr::Size_t preSamples   = 15;
        constexpr gr::Size_t totalSamples = postSamples + preSamples;
        constexpr auto       testDuration = 3s;

        const double      timePerDataset   = static_cast<double>(totalSamples) / sampleRate;
        const double      testDurationSecs = std::chrono::duration<double>(testDuration).count();
        const std::size_t maxDatasets      = static_cast<size_t>(std::floor(testDurationSecs / timePerDataset));
        const std::size_t minDatasets      = 2UZ;

        Graph flowGraph;
        auto& ps = flowGraph.emplaceBlock<PicoscopeT<T>>({{{"sample_rate", sampleRate}, {"pre_samples", preSamples}, {"post_samples", postSamples}, {"n_captures", gr::Size_t{1}}, {"auto_arm", true}, //
            {"channel_ids", std::vector<std::string>{"A"}}, {"channel_ranges", std::vector<float>{5.f}}, {"channel_couplings", std::vector<std::string>{"AC_1M"}}}});

        auto& sinkA = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", true}, {"log_tags", false}}});
        auto& sinkB = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
        auto& sinkC = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
        auto& sinkD = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});

        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).to<"in">(sinkA)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkD)));

        if constexpr (std::is_same_v<Picoscope4000a<T>, PicoscopeT<T>>) {
            auto& sinkE = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
            auto& sinkF = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
            auto& sinkG = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
            auto& sinkH = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});

            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 4>(ps).template to<"in">(sinkE)));
            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 5>(ps).template to<"in">(sinkF)));
            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 6>(ps).template to<"in">(sinkG)));
            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 7>(ps).template to<"in">(sinkH)));
        }

        // Explicitly start unit because it takes quite some time
        expect(nothrow([&ps] { ps.start(); }));
        scheduler::Simple<scheduler::ExecutionPolicy::multiThreaded> sched{std::move(flowGraph)};
        expect(sched.changeStateTo(lifecycle::State::INITIALISED).has_value());
        expect(sched.changeStateTo(lifecycle::State::RUNNING).has_value());
        std::this_thread::sleep_for(testDuration);
        expect(sched.changeStateTo(lifecycle::State::REQUESTED_STOP).has_value());

        fmt::println("rapid block continuous: producedDatasets:{}, valid range: [{}, {}]", sinkA._nSamplesProduced, minDatasets, maxDatasets);
        expect(ge(sinkA._nSamplesProduced, minDatasets));
        expect(le(sinkA._nSamplesProduced, maxDatasets));

        for (std::size_t iC = 0; iC < sinkA._samples.size(); iC++) {
            expect(eq(sinkA._samples[iC].signal_values.size(), totalSamples));
        }
    };
};

} // namespace fair::picoscope::test

int main() { /* tests are statically executed */ }
