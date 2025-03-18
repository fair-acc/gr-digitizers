#include <boost/ut.hpp>

#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/basic/ClockSource.hpp>
#include <gnuradio-4.0/testing/TagMonitors.hpp>

#include <Picoscope3000a.hpp>
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
void testRapidBlockBasic(std::size_t nCaptures, float sampleRate = 1234567.f, bool testDigitalOutput = false) {
    using namespace boost::ut;
    using namespace gr;
    using namespace fair::picoscope;

    fmt::println("testRapidBlockBasic({}) - {}", nCaptures, gr::meta::type_name<T>());

    constexpr gr::Size_t preSamples   = 33;
    constexpr gr::Size_t postSamples  = 1000;
    const gr::Size_t     totalSamples = preSamples + postSamples;

    int16_t        digitalPortThreshold     = static_cast<int16_t>(32767 / 5);
    bool           digitalPortInvertOutput  = false;
    const bool     digitalPortUseExactValue = true;
    const uint16_t digitalPortExactValue    = 8; // 2^n, n = connected port index (assuming that only one port is connected)

    Graph flowGraph;
    auto& ps = flowGraph.emplaceBlock<PicoscopeT<T>>({{"disconnect_on_done", true}, {"sample_rate", sampleRate}, {"pre_samples", preSamples}, {"post_samples", postSamples}, {"n_captures", nCaptures}, //
        {"auto_arm", true}, {"trigger_once", true}, {"channel_ids", std::vector<std::string>{"A"}}, {"channel_ranges", std::vector<float>{5.f}},                                                        //
        //{"trigger_source", "D"}, {"trigger_threshold", 0.5f},                                                                                                                                           //
        {"channel_couplings", std::vector<std::string>{"AC"}}, {"digital_port_threshold", digitalPortThreshold}, {"digital_port_invert_output", digitalPortInvertOutput}});

    auto& sinkA = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", false}});
    auto& sinkB = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", false}});
    auto& sinkC = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
    auto& sinkD = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});

    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).template to<"in">(sinkA)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkD)));

    auto& sinkDigital = flowGraph.emplaceBlock<testing::TagSink<gr::DataSet<uint16_t>, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", false}});
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"digitalOut">(ps).template to<"in">(sinkDigital)));

    if constexpr (std::is_same_v<Picoscope4000a<T>, PicoscopeT<T>>) {
        auto& sinkE = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkF = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkG = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkH = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});

        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 4>(ps).template to<"in">(sinkE)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 5>(ps).template to<"in">(sinkF)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 6>(ps).template to<"in">(sinkG)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 7>(ps).template to<"in">(sinkH)));
    }

    scheduler::Simple sched{std::move(flowGraph)};
    expect(sched.runAndWait().has_value());

    if constexpr (std::is_same_v<Picoscope4000a<T>, PicoscopeT<T>>) {
        if (sampleRate == 1234567.f) {
            expect(eq(ps._actualSampleRate, 1250000.f));
        }
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

    expect(eq(sinkDigital._nSamplesProduced, nCaptures)); // number of DataSets
    expect(eq(sinkDigital._samples.size(), nCaptures));   // number of DataSets
    if (sinkDigital._samples.size() >= nCaptures) {
        for (std::size_t iC = 0; iC < sinkDigital._samples.size(); iC++) {
            expect(eq(sinkDigital._samples[iC].signal_values.size(), totalSamples));

            // Digital output testing relies on the actual test setup.
            // We assume that we have only one input, the input signal is a `square wave` (Ampl = 1V, offset = 1V) at 1kHz, generated using picoscope's function generator.
            // With a sample rate near 10kHz, the occurrence of zeros and non-zeros should be roughly equal.
            if (testDigitalOutput) {
                const auto countZeros    = std::ranges::count(sinkDigital._samples[iC].signal_values, 0);
                const auto countNonZeros = (digitalPortUseExactValue) ? std::ranges::count(sinkDigital._samples[iC].signal_values, digitalPortExactValue) : std::ranges::count_if(sinkDigital._samples[iC].signal_values, [](int x) { return x != 0; });
                const bool isEqual       = static_cast<double>(std::abs(countZeros - countNonZeros)) / static_cast<double>(countNonZeros) < 0.05;
                fmt::println("Digital output rapid block testing countZeros:{}, countNonZeros:{}, isEqual:{}", countZeros, countNonZeros, isEqual);
                expect(isEqual);
            }
        }
    }
}

template<typename T>
void testStreamingBasics(float sampleRate = 83000.f, bool testDigitalOutput = false) {
    using namespace std::chrono;
    using namespace std::chrono_literals;
    using namespace boost::ut;
    using namespace gr;
    using namespace fair::picoscope;
    Graph flowGraph;

    fmt::println("testStreamingBasics - {}", gr::meta::type_name<T>());

    constexpr auto testDuration = 2s;

    int16_t        digitalPortThreshold     = static_cast<int16_t>(32767 / 5);
    bool           digitalPortInvertOutput  = false;
    const bool     digitalPortUseExactValue = true;
    const uint16_t digitalPortExactValue    = 8; // 2^n, n = connected port index (assuming that only one port is connected)

    auto& ps = flowGraph.emplaceBlock<PicoscopeT<T>>({{"sample_rate", sampleRate},                                                                                       //
        {"streaming_mode_poll_rate", 0.00001f}, {"auto_arm", true}, {"channel_ids", std::vector<std::string>{"A"}},                                                      //
        {"signal_names", std::vector<std::string>{"Test signal"}}, {"signal_units", std::vector<std::string>{"Test unit"}}, {"channel_ranges", std::vector<float>{5.f}}, //
        {"channel_couplings", std::vector<std::string>{"AC"}}, {"digital_port_threshold", digitalPortThreshold}, {"digital_port_invert_output", digitalPortInvertOutput}});

    auto& tagMonitor = flowGraph.emplaceBlock<testing::TagMonitor<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", true}});
    auto& sinkA      = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
    auto& sinkB      = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
    auto& sinkC      = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
    auto& sinkD      = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});

    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).template to<"in">(tagMonitor)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out">(tagMonitor).template to<"in">(sinkA)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkD)));

    auto& sinkDigital = flowGraph.emplaceBlock<testing::TagSink<uint16_t, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", testDigitalOutput}, {"log_tags", false}});
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"digitalOut">(ps).template to<"in">(sinkDigital)));

    if constexpr (std::is_same_v<Picoscope4000a<T>, PicoscopeT<T>>) {
        auto& sinkE = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkF = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkG = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkH = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});

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

    expect(ge(sinkA._nSamplesProduced, static_cast<std::size_t>(sampleRate)));
    expect(le(sinkA._nSamplesProduced, static_cast<std::size_t>(2.5f * sampleRate))); // 2 seconds
    expect(le(sinkA._nSamplesProduced, sinkDigital._nSamplesProduced));
    expect(ge(tagMonitor._tags.size(), 1UZ));
    if (tagMonitor._tags.size() == 1UZ) {
        const auto& tag = tagMonitor._tags[0];
        expect(eq(tag.index, int64_t{0}));
        expect(eq(std::get<float>(tag.at(std::string(tag::SAMPLE_RATE.shortKey()))), sampleRate));
        expect(eq(std::get<std::string>(tag.at(std::string(tag::SIGNAL_NAME.shortKey()))), "Test signal"s));
        expect(eq(std::get<std::string>(tag.at(std::string(tag::SIGNAL_UNIT.shortKey()))), "Test unit"s));
        expect(eq(std::get<float>(tag.at(std::string(tag::SIGNAL_MIN.shortKey()))), -5.f));
        expect(eq(std::get<float>(tag.at(std::string(tag::SIGNAL_MAX.shortKey()))), 5.f));
    }

    // Digital output testing relies on the actual test setup.
    // We assume that we have only one input, the input signal is a `square wave` (Ampl = 1V, offset = 1V) at 1kHz, generated using picoscope's function generator.
    // With a sample rate near 10kHz, the occurrence of zeros and non-zeros should be roughly equal.
    if (testDigitalOutput) {
        const auto countZeros    = std::ranges::count(sinkDigital._samples, 0);
        const auto countNonZeros = (digitalPortUseExactValue) ? std::ranges::count(sinkDigital._samples, digitalPortExactValue) : std::ranges::count_if(sinkDigital._samples, [](int x) { return x != 0; });
        const bool isEqual       = static_cast<double>(std::abs(countZeros - countNonZeros)) / static_cast<double>(countNonZeros) < 0.05;
        fmt::println("Digital output streaming testing countZeros:{}, countNonZeros:{}, isEqual:{}", countZeros, countNonZeros, isEqual);
        expect(isEqual);
    }
}

const boost::ut::suite PicoscopeTests = [] {
    using namespace boost::ut;
    using namespace gr;
    using namespace fair::picoscope;

    "open and close"_test = [] {
        Graph flowGraph;
        auto& ps = flowGraph.emplaceBlock<PicoscopeT<DataSet<float>>>();

        // this takes time, so we do it a few times only
        for (auto i = 0; i < 3; i++) {
            expect(nothrow([&ps] { ps.open(); }));
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
        //  testRapidBlockBasic<gr::DataSet<int16_t>>(1);
        testRapidBlockBasic<gr::DataSet<float>>(1);
        //   testRapidBlockBasic<gr::DataSet<gr::UncertainValue<float>>>(1);
    };

    skip / "streaming digital output"_test = [] { testStreamingBasics<float>(4000, true); };

    skip / "rapid block digital output"_test = [] { testRapidBlockBasic<gr::DataSet<float>>(1, 4000, true); };

    "rapid block multiple captures"_test = [] { testRapidBlockBasic<DataSet<float>>(3); };

    "rapid block 3 channels"_test = [] {
        fmt::println("rapid block 3 channels");
        constexpr gr::Size_t preSamples   = 33;
        constexpr gr::Size_t postSamples  = 1000;
        constexpr gr::Size_t nCaptures    = 5;
        constexpr gr::Size_t totalSamples = preSamples + postSamples;
        using T                           = DataSet<float>;

        Graph flowGraph;
        auto& ps = flowGraph.emplaceBlock<PicoscopeT<T>>({{"sample_rate", 10000.f},                               //
            {"pre_samples", preSamples}, {"post_samples", postSamples}, {"n_captures", nCaptures},                //
            {"auto_arm", true}, {"trigger_once", true}, {"channel_ids", std::vector<std::string>{"A", "B", "C"}}, //
            {"channel_ranges", std::vector<float>{{5.f, 5.f, 5.f}}}, {"channel_couplings", std::vector<std::string>{"AC", "AC", "AC"}}});

        auto& sinkA = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", false}});
        auto& sinkB = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", false}});
        auto& sinkC = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", false}});
        auto& sinkD = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", false}});

        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).template to<"in">(sinkA)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkD)));

        auto& sinkDigital = flowGraph.emplaceBlock<testing::TagSink<gr::DataSet<uint16_t>, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"digitalOut">(ps).template to<"in">(sinkDigital)));

        if constexpr (std::is_same_v<Picoscope4000a<T>, PicoscopeT<T>>) {
            auto& sinkE = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkF = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkG = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkH = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});

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
        fmt::println("rapid block continuous");
        using namespace std::chrono_literals;

        using T = DataSet<float>;

        constexpr float      sampleRate   = 1000.f;
        constexpr gr::Size_t postSamples  = 100;
        constexpr gr::Size_t preSamples   = 15;
        constexpr gr::Size_t totalSamples = postSamples + preSamples;
        constexpr auto       testDuration = 3s;

        const float       timePerDataset   = static_cast<float>(totalSamples) / sampleRate;
        const float       testDurationSecs = std::chrono::duration<float>(testDuration).count();
        const std::size_t maxDatasets      = static_cast<size_t>(std::floor(testDurationSecs / timePerDataset));
        const std::size_t minDatasets      = 2UZ;

        Graph flowGraph;
        auto& ps = flowGraph.emplaceBlock<PicoscopeT<T>>({{"sample_rate", sampleRate}, {"pre_samples", preSamples}, {"post_samples", postSamples}, {"n_captures", gr::Size_t{1}}, {"auto_arm", true}, //
            {"channel_ids", std::vector<std::string>{"A"}}, {"channel_ranges", std::vector<float>{5.f}}, {"channel_couplings", std::vector<std::string>{"AC"}}});

        auto& sinkA = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", false}});
        auto& sinkB = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkC = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkD = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});

        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).template to<"in">(sinkA)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkD)));

        auto& sinkDigital = flowGraph.emplaceBlock<testing::TagSink<gr::DataSet<uint16_t>, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"digitalOut">(ps).template to<"in">(sinkDigital)));

        if constexpr (std::is_same_v<Picoscope4000a<T>, PicoscopeT<T>>) {
            auto& sinkE = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkF = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkG = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkH = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});

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

    "tagContainsTrigger"_test = [] {
        fmt::println("tagContainsTrigger");
        using namespace std::chrono_literals;
        using namespace gr::tag;

        const auto testCase = [](bool expectedResult, const std::string& triggerNameAndCtx, const std::string& tagTriggerName, const std::string& tagCtx, bool includeCtx) { //
            const auto tag = Tag(0, includeCtx ? property_map{{TRIGGER_NAME.shortKey(), tagTriggerName}, {CONTEXT.shortKey(), tagCtx}}                                       //
                                               : property_map{{TRIGGER_NAME.shortKey(), tagTriggerName}});
            const bool res = detail::tagContainsTrigger(tag, detail::createTriggerNameAndCtx(triggerNameAndCtx));
            expect(eq(expectedResult, res)) << fmt::format("triggerNameAndCtx:{}, tag.map:{}", triggerNameAndCtx, tag.map);
        };

        // both trigger_name and ctx are set
        testCase(true, "trigger1/ctx1", "trigger1", "ctx1", true);
        testCase(false, "trigger1/ctx1", "trigger2", "ctx1", true); // incorrect trigger_name
        testCase(false, "trigger1/ctx1", "trigger1", "ctx2", true); // incorrect ctx
        testCase(false, "trigger1/ctx1", "trigger1", "", true);     // empty ctx
        testCase(false, "trigger1/ctx1", "trigger1", "", false);    // no ctx key in tag.map

        // both trigger_name and empty ctx are set
        testCase(true, "trigger1/", "trigger1", "", true);
        testCase(false, "trigger1/", "trigger2", "", true);     // incorrect trigger_name
        testCase(false, "trigger1/", "trigger1", "ctx2", true); // incorrect ctx
        testCase(false, "trigger1/", "trigger1", "", false);    // no ctx key in tag.map

        // only trigger_name is set
        testCase(false, "trigger1", "trigger2", "ctx1", true); // incorrect trigger_name
        testCase(true, "trigger1", "trigger1", "ctx2", true);  // incorrect ctx
        testCase(true, "trigger1", "trigger1", "", true);      // empty ctx
        testCase(true, "trigger1", "trigger1", "", false);     // no ctx key in tag.map
    };

    "rapid block arm/disarm trigger"_test = [] {
        fmt::println("rapid block arm/disarm trigger");
        using namespace std::chrono_literals;
        using namespace gr::tag;

        using T = DataSet<float>;

        constexpr float      sampleRate   = 1000.f;
        constexpr gr::Size_t postSamples  = 1000;
        constexpr gr::Size_t preSamples   = 5;
        constexpr gr::Size_t totalSamples = postSamples + preSamples;
        constexpr auto       testDuration = 12s;

        const auto createTriggerPropertyMap = [](const std::string& triggerName, const std::string& context, std::uint64_t time, float offset = 0.f) { //
            return property_map{{TRIGGER_NAME.shortKey(), triggerName}, {TRIGGER_TIME.shortKey(), std::uint64_t{time}}, {TRIGGER_OFFSET.shortKey(), offset}, {CONTEXT.shortKey(), context}};
        };

        Graph flowGraph;
        auto& clockSrc = flowGraph.emplaceBlock<gr::basic::ClockSource<std::uint8_t>>({{"sample_rate", sampleRate}, {"chunk_size", gr::Size_t(1)}, {"n_samples_max", gr::Size_t(0)}, {"name", "ClockSource"}, {"verbose_console", true}});

        clockSrc.tags = {
            Tag(1000, createTriggerPropertyMap("CMD_BP_START", "FAIR.SELECTOR.C=1:S=1:P=1", static_cast<std::uint64_t>(1000))), // OK
            Tag(3000, createTriggerPropertyMap("CMD_BP_START", "FAIR.SELECTOR.C=1:S=1:P=1", static_cast<std::uint64_t>(3000))), // OK
            Tag(3500, createTriggerPropertyMap("CMD_BP_START", "FAIR.SELECTOR.C=1:S=1:P=1", static_cast<std::uint64_t>(3500))), // This trigger will be ignored
            Tag(3700, createTriggerPropertyMap("CMD_BP_START", "FAIR.SELECTOR.C=1:S=1:P=1", static_cast<std::uint64_t>(3700))), // This trigger will be ignored
            Tag(5000, createTriggerPropertyMap("CMD_BP_START", "FAIR.SELECTOR.C=1:S=1:P=1", static_cast<std::uint64_t>(5000))), // This trigger will be disarmed
            Tag(5500, createTriggerPropertyMap("CMD_BP_STOP", "FAIR.SELECTOR.C=1:S=1:P=1", static_cast<std::uint64_t>(5500))),  // disarm trigger
            Tag(6000, createTriggerPropertyMap("CMD_BP_START", "FAIR.SELECTOR.C=1:S=1:P=1", static_cast<std::uint64_t>(6000))), // OK
            Tag(8000, createTriggerPropertyMap("CMD_BP_START", "FAIR.SELECTOR.C=1:S=1:P=1", static_cast<std::uint64_t>(8000))), // This trigger will be disarmed
            Tag(8500, createTriggerPropertyMap("CMD_BP_START", "FAIR.SELECTOR.C=1:S=1:P=1", static_cast<std::uint64_t>(8500))), // This trigger will be ignored
            Tag(8700, createTriggerPropertyMap("CMD_BP_STOP", "FAIR.SELECTOR.C=1:S=1:P=1", static_cast<std::uint64_t>(8700))),  // disarm trigger
            Tag(8800, createTriggerPropertyMap("CMD_BP_START", "FAIR.SELECTOR.C=1:S=1:P=1", static_cast<std::uint64_t>(8800)))  // OK
        }; // disarm trigger

        auto& ps = flowGraph.emplaceBlock<PicoscopeT<T>>({{"sample_rate", sampleRate}, {"pre_samples", preSamples}, {"post_samples", postSamples},                                      //
            {"trigger_arm", "CMD_BP_START/FAIR.SELECTOR.C=1:S=1:P=1"}, {"trigger_disarm", "CMD_BP_STOP/FAIR.SELECTOR.C=1:S=1:P=1"}, {"n_captures", gr::Size_t{1}}, {"auto_arm", false}, //
            {"channel_ids", std::vector<std::string>{"A"}}, {"channel_ranges", std::vector<float>{5.f}}, {"channel_couplings", std::vector<std::string>{"AC"}}});

        auto& sinkA = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", false}});
        auto& sinkB = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkC = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkD = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});

        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out">(clockSrc).template to<"timingIn">(ps)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).template to<"in">(sinkA)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkD)));

        auto& sinkDigital = flowGraph.emplaceBlock<testing::TagSink<gr::DataSet<uint16_t>, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"digitalOut">(ps).template to<"in">(sinkDigital)));

        if constexpr (std::is_same_v<Picoscope4000a<T>, PicoscopeT<T>>) {
            auto& sinkE = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkF = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkG = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkH = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});

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

        expect(eq(sinkA._nSamplesProduced, 4UZ));

        for (std::size_t iC = 0; iC < sinkA._samples.size(); iC++) {
            expect(eq(sinkA._samples[iC].signal_values.size(), totalSamples));
        }
    };

    "rapid partial captures"_test = [] {
        fmt::println("rapid partial captures");
        using namespace std::chrono_literals;
        using namespace gr::tag;

        using T = DataSet<float>;

        constexpr float      sampleRate   = 1000.f;
        constexpr gr::Size_t postSamples  = 1000;
        constexpr gr::Size_t preSamples   = 5;
        constexpr gr::Size_t totalSamples = postSamples + preSamples;
        constexpr auto       testDuration = 12s;
        constexpr gr::Size_t nCaptures    = 10;

        const auto createTriggerPropertyMap = [](const std::string& triggerName, const std::string& context, std::uint64_t time, float offset = 0.f) { //
            return property_map{{TRIGGER_NAME.shortKey(), triggerName}, {TRIGGER_TIME.shortKey(), std::uint64_t{time}}, {TRIGGER_OFFSET.shortKey(), offset}, {CONTEXT.shortKey(), context}};
        };

        Graph flowGraph;
        auto& clockSrc = flowGraph.emplaceBlock<gr::basic::ClockSource<std::uint8_t>>({{"sample_rate", sampleRate}, {"chunk_size", gr::Size_t(1)}, {"n_samples_max", gr::Size_t(0)}, {"name", "ClockSource"}, {"verbose_console", true}});

        clockSrc.tags = {Tag(1000, createTriggerPropertyMap("CMD_BP_START", "FAIR.SELECTOR.C=1:S=1:P=1", static_cast<std::uint64_t>(1000))), //
            Tag(5500, createTriggerPropertyMap("CMD_BP_STOP", "FAIR.SELECTOR.C=1:S=1:P=1", static_cast<std::uint64_t>(5500)))};              // disarm trigger

        auto& ps = flowGraph.emplaceBlock<PicoscopeT<T>>({{"sample_rate", sampleRate}, {"pre_samples", preSamples}, {"post_samples", postSamples},                                  //
            {"trigger_arm", "CMD_BP_START/FAIR.SELECTOR.C=1:S=1:P=1"}, {"trigger_disarm", "CMD_BP_STOP/FAIR.SELECTOR.C=1:S=1:P=1"}, {"n_captures", nCaptures}, {"auto_arm", false}, //
            {"channel_ids", std::vector<std::string>{"A"}}, {"channel_ranges", std::vector<float>{5.f}}, {"channel_couplings", std::vector<std::string>{"AC"}}});

        auto& sinkA = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", false}});
        auto& sinkB = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkC = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkD = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});

        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out">(clockSrc).to<"timingIn">(ps)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).to<"in">(sinkA)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkD)));

        auto& sinkDigital = flowGraph.emplaceBlock<testing::TagSink<gr::DataSet<uint16_t>, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"digitalOut">(ps).template to<"in">(sinkDigital)));

        if constexpr (std::is_same_v<Picoscope4000a<T>, PicoscopeT<T>>) {
            auto& sinkE = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkF = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkG = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkH = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});

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

        expect(eq(sinkA._nSamplesProduced, 4UZ));

        for (std::size_t iC = 0; iC < sinkA._samples.size(); iC++) {
            expect(eq(sinkA._samples[iC].signal_values.size(), totalSamples));
        }
    };
};

} // namespace fair::picoscope::test

int main() { /* tests are statically executed */ }
