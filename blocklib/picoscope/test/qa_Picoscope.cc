#include <boost/ut.hpp>

#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/basic/ClockSource.hpp>
#include <gnuradio-4.0/testing/TagMonitors.hpp>

#include <fair/picoscope/Picoscope.hpp>
#include <fair/picoscope/Picoscope3000a.hpp>
#include <fair/picoscope/Picoscope4000a.hpp>
#include <fair/picoscope/Picoscope5000a.hpp>
#include <fair/picoscope/Picoscope6000.hpp>

using namespace std::string_literals;

// Workaround for compile-time bottleneck in vir-simd, as evaluating struct_size for uncertain datasets is very expensive to compile and
// not used in the picoscope block with its processBulk function anyway.
// The block logic still checks if the block provides a simdized processOne and has to evaluate the simd size for all ports for that.
namespace vir::detail {
template<>
constexpr size_t struct_size<gr::DataSet<gr::UncertainValue<float>>, 0, sizeof(gr::DataSet<gr::UncertainValue<float>>) * CHAR_BIT, 8>() {
    // static_assert(false); // uncomment here to see in the compiler errors where the simd computations are triggered
    return 0UZ;
}
} // namespace vir::detail

namespace fair::picoscope::test {

// enumerate the picoscope types to be tested
using picoscopeTypes = std::tuple<Picoscope3000a, Picoscope4000a, Picoscope5000a, Picoscope6000>;

template<typename T>
using BulkTagSink = gr::testing::TagSink<T, gr::testing::ProcessFunction::USE_PROCESS_BULK>;

constexpr std::size_t minPicoBufferSize = 65536UZ; // this is needed because otherwise even streaming buffers for UncertainValue will only get 4k buffer entries

bool promptForTestCase(std::string_view testcase) {
    std::println("{}", testcase);
    static bool runAll = false;
    if (runAll) {
        return true;
    }
    static bool runNone = false;
    if (runNone) {
        return false;
    }
    std::print("Do you want to run this test? [Y]es, [s]kip, [a]ll, [n]one: ");
    while (true) {
        int ch;
        switch (ch = std::getchar()) {
        case 'a': runAll = true; return true;
        case 'y':
        case 'Y': return true;
        case 'n': runNone = true; return false;
        case 's': return false;
        case '\n': break; // do not consume an additional newline if we only got the newline
        default: std::ignore = std::getchar();
        }
        std::print("\ninvalid choice {}, valid choices: [Y]es, [s]kip, [a]ll, [n]one: ", static_cast<char>(ch));
        while ((ch = std::getchar()) != '\n') {
        } // remove superfluous characters
    }
    std::println("");
}

template<gr::DataSetLike T, PicoscopeImplementationLike PicoscopeT>
void testRapidBlockBasic(std::size_t nCaptures, float sampleRate = 1234567.f, std::string triggerSource = "", bool testDigitalOutput = false) {
    using namespace boost::ut;
    using namespace gr;
    using namespace fair::picoscope;

    if (!promptForTestCase(std::format("testRapidBlockBasic(n={}) - {} - {}", nCaptures, gr::meta::type_name<T>(), gr::meta::type_name<PicoscopeT>()))) {
        return;
    }

    constexpr gr::Size_t preSamples   = 33;
    constexpr gr::Size_t postSamples  = 1000;
    constexpr gr::Size_t totalSamples = preSamples + postSamples;

    bool           digitalPortInvertOutput  = false;
    const bool     digitalPortUseExactValue = true;
    const uint16_t digitalPortExactValue    = 8; // 2^n, n = connected port index (assuming that only one port is connected)

    Graph        flowGraph;
    property_map params = {
        {"sample_rate", sampleRate},
        {"pre_samples", preSamples},
        {"post_samples", postSamples},
        {"n_captures", nCaptures},
        {"auto_arm", true},
        {"trigger_once", true},
        {"channel_ids", std::vector<std::string>{"A"}},
        {"channel_ranges", std::vector<float>{5.f}},
        {"trigger_threshold", 0.0f},
        {"channel_couplings", std::vector<std::string>{"AC"}},
        {"digital_port_invert_output", digitalPortInvertOutput},
    };
    if (!triggerSource.empty()) {
        params.insert_or_assign("trigger_source", triggerSource);
    }
    auto& ps    = flowGraph.emplaceBlock<Picoscope<T, PicoscopeT>>(params);
    auto& sinkA = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", true}, {"log_tags", false}});
    auto& sinkB = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", true}, {"log_tags", false}});
    auto& sinkC = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
    auto& sinkD = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});

    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).template to<"in">(sinkA)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkD)));

    auto& sinkDigital = flowGraph.emplaceBlock<BulkTagSink<gr::DataSet<uint16_t>>>({{"log_samples", true}, {"log_tags", false}});
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"digitalOut">(ps).template to<"in">(sinkDigital)));

    if constexpr (std::is_same_v<Picoscope4000a, PicoscopeT>) {
        auto& sinkE = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkF = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkG = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkH = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});

        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 4>(ps).template to<"in">(sinkE)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 5>(ps).template to<"in">(sinkF)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 6>(ps).template to<"in">(sinkG)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 7>(ps).template to<"in">(sinkH)));
    }

    scheduler::Simple sched{};
    std::ignore = sched.exchange(std::move(flowGraph));
    expect(sched.runAndWait().has_value());

    expect(eq(sinkA._nSamplesProduced, nCaptures)); // number of DataSets
    expect(eq(sinkA._samples.size(), nCaptures));   // number of DataSets
    if (sinkA._samples.size() >= nCaptures) {
        for (std::size_t iC = 0; iC < sinkA._samples.size(); iC++) {
            const T& ds = sinkA._samples[iC];
            expect(eq(ds.signal_values.size(), totalSamples));

            // check axis_values (time axis)
            expect(eq(ds.axis_values.size(), 1UZ));
            if (ds.axis_values.size() >= 1) {
                expect(eq(ds.axis_values[0].size(), totalSamples));
                if (ds.axis_values[0].size() == totalSamples) {
                    expect(std::ranges::is_sorted(ds.axis_values[0]));
                    const float tolerance = 0.0005f;
                    if constexpr (std::is_same_v<typename T::value_type, float> || std::is_same_v<typename T::value_type, gr::UncertainValue<float>>) {
                        expect(approx(ds.axis_values[0][0], -static_cast<float>(preSamples) / sampleRate, tolerance));
                        expect(approx(ds.axis_values[0][preSamples], 0.f, tolerance));
                        expect(approx(ds.axis_values[0].back(), static_cast<float>(postSamples) / sampleRate, tolerance));
                    } else if constexpr (std::is_same_v<typename T::value_type, std::int16_t>) {
                        expect(eq(ds.axis_values[0][0], static_cast<std::int16_t>(-preSamples)));
                        expect(eq(ds.axis_values[0].back(), static_cast<std::int16_t>(postSamples - 1)));
                    }
                }
            }
        }
    }

    expect(eq(sinkB._nSamplesProduced, nCaptures)); // empty DataSets are published if channel is not active, TODO: this logic can be changed later
    expect(eq(sinkB._samples.size(), nCaptures));
    if (sinkB._samples.size() >= nCaptures) {
        for (std::size_t iC = 0; iC < sinkB._samples.size(); iC++) {
            expect(eq(sinkB._samples[iC].signal_values.size(), 0UZ));
        }
    }

    if constexpr (PicoscopeT::N_DIGITAL_CHANNELS > 0) {
        expect(eq(sinkDigital._nSamplesProduced, nCaptures)); // number of DataSets
        expect(eq(sinkDigital._samples.size(), nCaptures));   // number of DataSets
        if (sinkDigital._samples.size() >= nCaptures) {
            for (auto& _sample : sinkDigital._samples) {
                expect(eq(_sample.signal_values.size(), totalSamples));

                // Digital output testing relies on the actual test setup.
                // We assume that we have only one input and the input signal is a `square wave` (Ampl = 1 V, offset = 1 V) at 1 kHz, generated using picoscope's function generator.
                // With a sample rate near 10 kHz, the occurrence of zeros and non-zeros should be roughly equal.
                if (testDigitalOutput) {
                    const auto countZeros    = std::ranges::count(_sample.signal_values, 0);
                    const auto countNonZeros = (digitalPortUseExactValue) ? std::ranges::count(_sample.signal_values, digitalPortExactValue) : std::ranges::count_if(_sample.signal_values, [](int x) { return x != 0; });
                    const bool isEqual       = static_cast<double>(std::abs(countZeros - countNonZeros)) / static_cast<double>(countNonZeros) < 0.05;
                    std::println("Digital output rapid block testing countZeros:{}, countNonZeros:{}, isEqual:{}", countZeros, countNonZeros, isEqual);
                    expect(isEqual);
                }
            }
        }
    }
}

template<typename T, PicoscopeImplementationLike PicoscopeT>
void testStreamingBasics(float sampleRate = 83000.f, bool testDigitalOutput = false) {
    using namespace std::chrono;
    using namespace std::chrono_literals;
    using namespace boost::ut;
    using namespace gr;
    using namespace fair::picoscope;
    Graph flowGraph;

    if (!promptForTestCase(std::format("testStreamingBasics - {} - {}", gr::meta::type_name<T>(), gr::meta::type_name<PicoscopeT>()))) {
        return;
    }

    constexpr auto testDuration = 15s;

    bool           digitalPortInvertOutput  = false;
    const bool     digitalPortUseExactValue = true;
    const uint16_t digitalPortExactValue    = 8; // 2^n, n = connected port index (assuming that only one port is connected)

    auto& ps = flowGraph.emplaceBlock<Picoscope<T, PicoscopeT>>({
        {"sample_rate", sampleRate},
        {"auto_arm", true},
        {"channel_ids", std::vector<std::string>{"A"}},
        {"signal_names", std::vector<std::string>{"Test signal"}},
        {"signal_units", std::vector<std::string>{"Test unit"}},
        {"channel_ranges", std::vector<float>{5.f}},
        {"channel_couplings", std::vector<std::string>{"AC"}},
        {"digital_port_invert_output", digitalPortInvertOutput},
    });

    auto& tagMonitor = flowGraph.emplaceBlock<testing::TagMonitor<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", true}});
    auto& sinkA      = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
    auto& sinkB      = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
    auto& sinkC      = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
    auto& sinkD      = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});

    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps, minPicoBufferSize).template to<"in">(tagMonitor)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out">(tagMonitor, minPicoBufferSize).template to<"in">(sinkA)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps, minPicoBufferSize).template to<"in">(sinkB)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps, minPicoBufferSize).template to<"in">(sinkC)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps, minPicoBufferSize).template to<"in">(sinkD)));

    auto& sinkDigital = flowGraph.emplaceBlock<BulkTagSink<uint16_t>>({{"log_samples", testDigitalOutput}, {"log_tags", false}});
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"digitalOut">(ps, minPicoBufferSize).template to<"in">(sinkDigital)));

    if constexpr (std::is_same_v<Picoscope4000a, PicoscopeT>) {
        auto& sinkE = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkF = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkG = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkH = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});

        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 4>(ps, minPicoBufferSize).template to<"in">(sinkE)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 5>(ps, minPicoBufferSize).template to<"in">(sinkF)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 6>(ps, minPicoBufferSize).template to<"in">(sinkG)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 7>(ps, minPicoBufferSize).template to<"in">(sinkH)));
    }

    // Explicitly start the picoscope because it takes quite some time
    // expect(nothrow([&ps] { ps.start(); }));

    scheduler::Simple<scheduler::ExecutionPolicy::multiThreaded> sched{};
    std::ignore = sched.exchange(std::move(flowGraph));
    expect(sched.changeStateTo(lifecycle::State::INITIALISED).has_value());
    expect(sched.changeStateTo(lifecycle::State::RUNNING).has_value());
    std::this_thread::sleep_for(testDuration);
    const auto lifestateChangeResult = sched.changeStateTo(lifecycle::State::REQUESTED_STOP);
    expect(lifestateChangeResult.has_value()) << [&lifestateChangeResult]() { return std::format("failed to set REQUESTED_STOP: {} at {}", lifestateChangeResult.error().message, lifestateChangeResult.error().sourceLocation); };

    const auto measuredRate = static_cast<double>(sinkA._nSamplesProduced) / duration<double>(testDuration).count();
    std::println("Produced in worker: {}", ps._nSamplesPublished);
    std::println("Configured rate: {}, Measured rate: {} ({:.2f}%), Duration: {} ms", sampleRate, static_cast<std::size_t>(measuredRate), measuredRate / static_cast<double>(sampleRate) * 100., duration_cast<milliseconds>(testDuration).count());
    std::println("Total samples: {}", sinkA._nSamplesProduced);
    std::println("Total tags: {}", tagMonitor._tags.size());
    for (auto& [i, map] : tagMonitor._tags) {
        std::println("  - {:4}: {}", i, map);
    }

    expect(ge(sinkA._nSamplesProduced, static_cast<std::size_t>(sampleRate)));
    expect(le(sinkA._nSamplesProduced, static_cast<std::size_t>(15.0f * sampleRate))); // 2 seconds
    expect(le(sinkA._nSamplesProduced, sinkDigital._nSamplesProduced));
    expect(ge(tagMonitor._tags.size(), 1UZ));
    if (tagMonitor._tags.size() == 1UZ) {
        const auto& tag = tagMonitor._tags[0];
        expect(eq(tag.index, 0UZ));
        expect(eq(std::get<float>(tag.at(std::string(tag::SAMPLE_RATE.shortKey()))), sampleRate));
        expect(eq(std::get<std::string>(tag.at(std::string(tag::SIGNAL_NAME.shortKey()))), "Test signal"s));
        expect(eq(std::get<std::string>(tag.at(std::string(tag::SIGNAL_UNIT.shortKey()))), "Test unit"s));
        expect(eq(std::get<float>(tag.at(std::string(tag::SIGNAL_MIN.shortKey()))), -5.f));
        expect(eq(std::get<float>(tag.at(std::string(tag::SIGNAL_MAX.shortKey()))), 5.f));
    }

    // Digital output testing relies on the actual test setup.
    // We assume that we have only one input. The input signal is a `square wave` (Ampl = 1 V, offset = 1 V) at 1 kHz, generated using picoscope's function generator.
    // With a sample rate near 10 kHz, the occurrence of zeros and non-zeros should be roughly equal.
    if (testDigitalOutput) {
        const auto countZeros    = std::ranges::count(sinkDigital._samples, 0);
        const auto countNonZeros = (digitalPortUseExactValue) ? std::ranges::count(sinkDigital._samples, digitalPortExactValue) : std::ranges::count_if(sinkDigital._samples, [](int x) { return x != 0; });
        const bool isEqual       = static_cast<double>(std::abs(countZeros - countNonZeros)) / static_cast<double>(countNonZeros) < 0.05;
        std::println("Digital output streaming testing countZeros:{}, countNonZeros:{}, isEqual:{}", countZeros, countNonZeros, isEqual);
        expect(isEqual);
    }
}

const boost::ut::suite PicoscopeTests = [] {
    using namespace boost::ut;
    using namespace gr;
    using namespace fair::picoscope;

    //"open and close"_test = []<PicoscopeImplementationLike PicoscopeT> {
    //    //if (!promptForTestCase(std::format("open and close: {}", gr::meta::type_name<PicoscopeT>()))) { return; }
    //    Graph flowGraph;
    //    auto& ps = flowGraph.emplaceBlock<Picoscope<DataSet<float>, PicoscopeT>>();

    //    // this takes time, so we do it a few times only
    //    for (auto i = 0; i < 3; i++) {
    //        expect(nothrow([&ps] { ps.initialize(); }));
    //        // expect(neq(ps.driverVersion(), std::string()));
    //        // expect(neq(ps.hardwareVersion(), std::string()));
    //        expect(nothrow([&ps] { ps.stop(); })) << "doesn't throw";
    //    }
    //} | picoscopeTypes{};

    "streaming basics"_test = []<PicoscopeImplementationLike PicoscopeT> {
        testStreamingBasics<int16_t, PicoscopeT>();
        testStreamingBasics<float, PicoscopeT>();
        testStreamingBasics<gr::UncertainValue<float>, PicoscopeT>();
    } | picoscopeTypes{};

    "rapid block basics"_test = []<PicoscopeImplementationLike PicoscopeT> {
        testRapidBlockBasic<gr::DataSet<int16_t>, PicoscopeT>(1);
        testRapidBlockBasic<gr::DataSet<float>, PicoscopeT>(1);
        testRapidBlockBasic<gr::DataSet<gr::UncertainValue<float>>, PicoscopeT>(1);

        // static_assert(vir::detail::struct_size<gr::DataSet<gr::UncertainValue<float>>> == 0);
    } | picoscopeTypes{};

    "rapid block digital port trigger"_test = []<PicoscopeImplementationLike PicoscopeT> {
        if constexpr (PicoscopeT::N_DIGITAL_CHANNELS == 0UZ) {
            return;
        }
        // TODO: Skipped because it requires proper configuration and Timing card. Skipping does not work for parameterized tests: https://github.com/boost-ext/ut/issues/555
        // This requires proper setup, for example, using `test-timing` to generate external timing events,
        // configuring the timing receiver to output pulses on IO1(2,3) for each event,
        // and connecting it to a digital input defined in `trigger_source`.
        // Implementing timing event generation directly in the test, similar to `qa_PicoscopeTiming`.
        testRapidBlockBasic<gr::DataSet<int16_t>, PicoscopeT>(1, 1234567.f, "DI4", false);
    } | picoscopeTypes{};

    // todo: needs testing with hardware setup
    //    "streaming digital output"_test = []<PicoscopeImplementationLike PicoscopeT> { testStreamingBasics<float, PicoscopeT>(4000, true); } | picoscopeTypes{};

    // todo: needs testing with hardware setup
    //    "rapid block digital output"_test = []<PicoscopeImplementationLike PicoscopeT> { testRapidBlockBasic<gr::DataSet<float>, PicoscopeT>(1, 4000, "", true); } | picoscopeTypes{};

    "rapid block multiple captures"_test = []<PicoscopeImplementationLike PicoscopeT> { testRapidBlockBasic<DataSet<float>, PicoscopeT>(3); } | picoscopeTypes{};

    "rapid block 3 channels"_test = []<PicoscopeImplementationLike PicoscopeT> {
        if (!promptForTestCase(std::format("rapid block 3 channels: {}", gr::meta::type_name<PicoscopeT>()))) {
            return;
        }
        constexpr gr::Size_t preSamples   = 33;
        constexpr gr::Size_t postSamples  = 1000;
        constexpr gr::Size_t nCaptures    = 5;
        constexpr gr::Size_t totalSamples = preSamples + postSamples;
        using T                           = DataSet<float>;

        Graph flowGraph;
        auto& ps = flowGraph.emplaceBlock<Picoscope<T, PicoscopeT>>({
            {"sample_rate", 10000.f},
            {"pre_samples", preSamples},
            {"post_samples", postSamples},
            {"n_captures", nCaptures},
            {"auto_arm", true},
            {"trigger_once", true},
            {"channel_ids", std::vector<std::string>{"A", "B", "C"}},
            {"channel_ranges", std::vector<float>{{5.f, 5.f, 5.f}}},
            {"channel_couplings", std::vector<std::string>{"AC", "AC", "AC"}},
        });

        auto& sinkA = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", true}, {"log_tags", false}});
        auto& sinkB = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", true}, {"log_tags", false}});
        auto& sinkC = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", true}, {"log_tags", false}});
        auto& sinkD = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", true}, {"log_tags", false}});

        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).template to<"in">(sinkA)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkD)));

        auto& sinkDigital = flowGraph.emplaceBlock<BulkTagSink<gr::DataSet<uint16_t>>>({{"log_samples", false}, {"log_tags", false}});
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"digitalOut">(ps).template to<"in">(sinkDigital)));

        if constexpr (std::is_same_v<Picoscope4000a, PicoscopeT>) {
            auto& sinkE = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkF = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkG = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkH = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});

            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 4>(ps).template to<"in">(sinkE)));
            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 5>(ps).template to<"in">(sinkF)));
            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 6>(ps).template to<"in">(sinkG)));
            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 7>(ps).template to<"in">(sinkH)));
        }

        scheduler::Simple sched{};
        std::ignore = sched.exchange(std::move(flowGraph));
        expect(sched.runAndWait().has_value());

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
    } | picoscopeTypes{};

    "rapid block continuous"_test = []<PicoscopeImplementationLike PicoscopeT> {
        if (!promptForTestCase(std::format("rapid block continuous: {}", gr::meta::type_name<PicoscopeT>()))) {
            return;
        }
        using namespace std::chrono_literals;

        using T = DataSet<float>;

        constexpr float      sampleRate   = 1000.f;
        constexpr gr::Size_t postSamples  = 100;
        constexpr gr::Size_t preSamples   = 15;
        constexpr gr::Size_t totalSamples = postSamples + preSamples;
        constexpr auto       testDuration = 13s;

        constexpr float timePerDataset   = static_cast<float>(totalSamples) / sampleRate;
        constexpr float testDurationSecs = std::chrono::duration<float>(testDuration).count();
        const auto      expectedDatasets = static_cast<size_t>(std::floor(testDurationSecs / timePerDataset));

        Graph flowGraph;
        auto& ps = flowGraph.emplaceBlock<Picoscope<T, PicoscopeT>>({
            {"sample_rate", sampleRate},
            {"pre_samples", preSamples},
            {"post_samples", postSamples},
            {"n_captures", gr::Size_t{1}},
            {"auto_arm", true},
            {"channel_ids", std::vector<std::string>{"A"}},
            {"channel_ranges", std::vector<float>{5.f}},
            {"channel_couplings", std::vector<std::string>{"AC"}},
        });

        auto& sinkA = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", true}, {"log_tags", false}});
        auto& sinkB = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkC = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkD = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});

        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).template to<"in">(sinkA)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkD)));

        auto& sinkDigital = flowGraph.emplaceBlock<BulkTagSink<gr::DataSet<uint16_t>>>({{"log_samples", false}, {"log_tags", false}});
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"digitalOut">(ps).template to<"in">(sinkDigital)));

        if constexpr (std::is_same_v<Picoscope4000a, PicoscopeT>) {
            auto& sinkE = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkF = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkG = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkH = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});

            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 4>(ps).template to<"in">(sinkE)));
            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 5>(ps).template to<"in">(sinkF)));
            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 6>(ps).template to<"in">(sinkG)));
            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 7>(ps).template to<"in">(sinkH)));
        }

        // Explicitly start the picoscope block because it takes quite some time
        expect(nothrow([&ps] { ps.start(); }));
        scheduler::Simple<scheduler::ExecutionPolicy::multiThreaded> sched{};
        std::ignore = sched.exchange(std::move(flowGraph));
        expect(sched.changeStateTo(lifecycle::State::INITIALISED).has_value());
        expect(sched.changeStateTo(lifecycle::State::RUNNING).has_value());
        std::this_thread::sleep_for(testDuration);
        expect(sched.changeStateTo(lifecycle::State::REQUESTED_STOP).has_value());

        std::println("rapid block continuous: producedDatasets:{}, expectedDatasets: {}", sinkA._nSamplesProduced, expectedDatasets);
        expect(le(sinkA._nSamplesProduced, expectedDatasets)); // we can never get more datasets than if re-arming was instantaneous
        expect(ge(sinkA._nSamplesProduced, 2UZ));              // the time for rearming differs wildly between different models, make sure we get at least 2 captures

        for (auto& _sample : sinkA._samples) {
            expect(eq(_sample.signal_values.size(), totalSamples));
        }
    } | picoscopeTypes{};

    "tagContainsTrigger"_test = [] {
        std::println("tagContainsTrigger");
        using namespace std::chrono_literals;
        using namespace gr::tag;

        const auto testCase = [](bool expectedResult, const std::string& triggerNameAndCtx, const std::string& tagTriggerName, const std::string& tagCtx, bool includeCtx) { //
            const auto tag = Tag(0, includeCtx ? property_map{{TRIGGER_NAME.shortKey(), tagTriggerName}, {CONTEXT.shortKey(), tagCtx}}                                       //
                                               : property_map{{TRIGGER_NAME.shortKey(), tagTriggerName}});
            const bool res = detail::tagContainsTrigger(tag.map, detail::createTriggerNameAndCtx(triggerNameAndCtx));
            expect(eq(expectedResult, res)) << std::format("triggerNameAndCtx:{}, tag.map:{}", triggerNameAndCtx, tag.map);
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

    "rapid block arm/disarm trigger"_test = []<PicoscopeImplementationLike PicoscopeT> {
        if (!promptForTestCase(std::format("rapid block arm/disarm trigger: {}", gr::meta::type_name<PicoscopeT>()))) {
            return;
        }
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
        auto& clockSrc = flowGraph.emplaceBlock<gr::basic::ClockSource<std::uint8_t>>({{"sample_rate", sampleRate}, {"chunk_size", gr::Size_t{1}}, {"n_samples_max", gr::Size_t{0}}, {"name", "ClockSource"}, {"verbose_console", true}});

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
            Tag(8800, createTriggerPropertyMap("CMD_BP_START", "FAIR.SELECTOR.C=1:S=1:P=1", static_cast<std::uint64_t>(8800))), // OK
        }; // disarm trigger

        auto& ps = flowGraph.emplaceBlock<Picoscope<T, PicoscopeT>>({
            {"sample_rate", sampleRate},
            {"pre_samples", preSamples},
            {"post_samples", postSamples},
            {"trigger_arm", "CMD_BP_START/FAIR.SELECTOR.C=1:S=1:P=1"},
            {"trigger_disarm", "CMD_BP_STOP/FAIR.SELECTOR.C=1:S=1:P=1"},
            {"n_captures", gr::Size_t{1}},
            {"auto_arm", false},
            {"channel_ids", std::vector<std::string>{"A"}},
            {"channel_ranges", std::vector<float>{5.f}},
            {"channel_couplings", std::vector<std::string>{"AC"}},
        });

        auto& sinkA = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", true}, {"log_tags", false}});
        auto& sinkB = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkC = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkD = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});

        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out">(clockSrc).to<"timingIn">(ps)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).template to<"in">(sinkA)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkD)));

        auto& sinkDigital = flowGraph.emplaceBlock<BulkTagSink<gr::DataSet<uint16_t>>>({{"log_samples", false}, {"log_tags", false}});
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"digitalOut">(ps).template to<"in">(sinkDigital)));

        if constexpr (std::is_same_v<Picoscope4000a, PicoscopeT>) {
            auto& sinkE = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkF = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkG = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkH = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});

            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 4>(ps).template to<"in">(sinkE)));
            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 5>(ps).template to<"in">(sinkF)));
            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 6>(ps).template to<"in">(sinkG)));
            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 7>(ps).template to<"in">(sinkH)));
        }

        // Explicitly start the picoscope block because it takes quite some time
        expect(nothrow([&ps] { ps.start(); }));
        scheduler::Simple<scheduler::ExecutionPolicy::multiThreaded> sched{};
        std::ignore = sched.exchange(std::move(flowGraph));
        expect(sched.changeStateTo(lifecycle::State::INITIALISED).has_value());
        expect(sched.changeStateTo(lifecycle::State::RUNNING).has_value());
        std::this_thread::sleep_for(testDuration);
        expect(sched.changeStateTo(lifecycle::State::REQUESTED_STOP).has_value());

        expect(ge(sinkA._nSamplesProduced, 1UZ));
        for (auto& _sample : sinkA._samples) {
            expect(eq(_sample.signal_values.size(), totalSamples));
        }
    } | picoscopeTypes{};

    "rapid partial captures"_test = []<PicoscopeImplementationLike PicoscopeT> {
        // This test verifies that if a capture is disarmed before the nCaptures triggers have been processed, we will get an update with all the
        // captures that have been captured before the dis-arm
        // For this the test arms the scope for 4500 samples at 1 kS/s, so for 4 seconds and requests 10 captures of 1005 samples => around 1 second each
        // The expected outcome is, that only the 4 updates occurring in the armed window will be published.
        if (!promptForTestCase(std::format("rapid partial captures: {}", std::string(gr::meta::type_name<PicoscopeT>())))) {
            return;
        }
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
            return property_map{
                {TRIGGER_NAME.shortKey(), triggerName},
                {TRIGGER_TIME.shortKey(), std::uint64_t{time}},
                {TRIGGER_OFFSET.shortKey(), offset},
                {CONTEXT.shortKey(), context},
            };
        };

        Graph flowGraph;
        auto& clockSrc = flowGraph.emplaceBlock<gr::basic::ClockSource<std::uint8_t>>({{"sample_rate", sampleRate}, {"chunk_size", gr::Size_t{1}}, {"n_samples_max", gr::Size_t{0}}, {"name", "ClockSource"}, {"verbose_console", true}});

        clockSrc.tags = {
            Tag(1000, createTriggerPropertyMap("CMD_BP_START", "FAIR.SELECTOR.C=1:S=1:P=1", static_cast<std::uint64_t>(1000))), //
            Tag(6000, createTriggerPropertyMap("CMD_BP_STOP", "FAIR.SELECTOR.C=1:S=1:P=1", static_cast<std::uint64_t>(6000))),  // disarm trigger
        };

        auto& ps = flowGraph.emplaceBlock<Picoscope<T, PicoscopeT>>({
            {"sample_rate", sampleRate},
            {"pre_samples", preSamples},
            {"post_samples", postSamples},
            {"trigger_arm", "CMD_BP_START/FAIR.SELECTOR.C=1:S=1:P=1"},
            {"trigger_disarm", "CMD_BP_STOP/FAIR.SELECTOR.C=1:S=1:P=1"},
            {"n_captures", nCaptures},
            {"auto_arm", false},
            {"channel_ids", std::vector<std::string>{"A"}},
            {"channel_ranges", std::vector<float>{5.f}},
            {"channel_couplings", std::vector<std::string>{"AC"}},
        });

        auto& sinkA = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", true}, {"log_tags", false}});
        auto& sinkB = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkC = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkD = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});

        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out">(clockSrc).to<"timingIn">(ps)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).template to<"in">(sinkA)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkD)));

        auto& sinkDigital = flowGraph.emplaceBlock<BulkTagSink<gr::DataSet<uint16_t>>>({{"log_samples", false}, {"log_tags", false}});
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"digitalOut">(ps).template to<"in">(sinkDigital)));

        if constexpr (std::is_same_v<Picoscope4000a, PicoscopeT>) {
            auto& sinkE = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkF = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkG = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});
            auto& sinkH = flowGraph.emplaceBlock<BulkTagSink<T>>({{"log_samples", false}, {"log_tags", false}});

            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 4>(ps).template to<"in">(sinkE)));
            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 5>(ps).template to<"in">(sinkF)));
            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 6>(ps).template to<"in">(sinkG)));
            expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 7>(ps).template to<"in">(sinkH)));
        }

        // Explicitly start the picoscope block because it takes quite some time
        expect(nothrow([&ps] { ps.start(); }));
        scheduler::Simple<scheduler::ExecutionPolicy::multiThreaded> sched{};
        std::ignore = sched.exchange(std::move(flowGraph));
        gr::MsgPortOut _toScheduler;
        expect(_toScheduler.connect(sched.msgIn) == gr::ConnectionResult::SUCCESS) << fatal;
        expect(sched.changeStateTo(lifecycle::State::INITIALISED).has_value());
        expect(sched.changeStateTo(lifecycle::State::RUNNING).has_value());
        // gr::sendMessage<gr::message::Command::Set>(_toScheduler, sched.unique_name, gr::block::property::kLifeCycleState, {{"state", std::string(magic_enum::enum_name(gr::lifecycle::State::INITIALISED))}}, "test");
        // gr::sendMessage<gr::message::Command::Set>(_toScheduler, sched.unique_name, gr::block::property::kLifeCycleState, {{"state", std::string(magic_enum::enum_name(gr::lifecycle::State::RUNNING))}}, "test");
        std::this_thread::sleep_for(testDuration);
        std::println("stopping scheduler");
        gr::sendMessage<gr::message::Command::Set>(_toScheduler, sched.unique_name, gr::block::property::kLifeCycleState, {{"state", std::string(magic_enum::enum_name(gr::lifecycle::State::REQUESTED_STOP))}}, "test");
        std::println("stopped scheduler");

        expect(ge(sinkA._nSamplesProduced, 1UZ));
        for (auto& _sample : sinkA._samples) {
            expect(eq(_sample.signal_values.size(), totalSamples));
        }
    } | picoscopeTypes{}; //| std::tuple<Picoscope3000a>{}; //
};

} // namespace fair::picoscope::test

int main() { /* tests are statically executed */ }
