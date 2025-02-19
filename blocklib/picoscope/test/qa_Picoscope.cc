#include <boost/ut.hpp>

#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/testing/TagMonitors.hpp>

#include <HelperBlocks.hpp>
#include <Picoscope4000a.hpp>
#include <Picoscope5000a.hpp>

using namespace std::string_literals;

namespace fair::picoscope::test {

// Replace with your connected Picoscope device
template<typename T, AcquisitionMode aMode>
using PicoscopeT = Picoscope4000a<T, aMode>;

static_assert(gr::HasProcessBulkFunction<PicoscopeT<float, AcquisitionMode::Streaming>>);
static_assert(gr::HasProcessBulkFunction<PicoscopeT<float, AcquisitionMode::RapidBlock>>);

template<typename T>
void testRapidBlockBasic(std::size_t nrCaptures) {
    using namespace boost::ut;
    using namespace gr;
    using namespace fair::helpers;
    using namespace fair::picoscope;

    fmt::println("testRapidBlockBasic - {}", gr::meta::type_name<T>());

    constexpr gr::Size_t kPreSamples  = 33;
    constexpr gr::Size_t kPostSamples = 1000;
    const gr::Size_t     totalSamples = nrCaptures * (kPreSamples + kPostSamples);

    Graph flowGraph;
    auto& ps = flowGraph.emplaceBlock<PicoscopeT<T, AcquisitionMode::RapidBlock>>({{{"sample_rate", 10000.f},                                      //
        {"pre_samples", kPreSamples}, {"post_samples", kPostSamples}, {"acquisition_mode", "RapidBlock"}, {"rapid_block_nr_captures", nrCaptures}, //
        {"auto_arm", true}, {"trigger_once", true}, {"channel_ids", std::vector<std::string>{"A"}}, {"channel_ranges", std::vector<float>{5.f}},   //
        {"channel_couplings", std::vector<std::string>{"AC_1M"}}}});

    auto& sinkA = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
    auto& sinkB = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
    auto& sinkC = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
    auto& sinkD = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
    auto& sinkE = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
    auto& sinkF = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
    auto& sinkG = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
    auto& sinkH = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});

    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).template to<"in">(sinkA)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkD)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 4>(ps).template to<"in">(sinkE)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 5>(ps).template to<"in">(sinkF)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 6>(ps).template to<"in">(sinkG)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 7>(ps).template to<"in">(sinkH)));

    scheduler::Simple sched{std::move(flowGraph)};
    expect(sched.runAndWait().has_value());

    expect(eq(sinkA._nSamplesProduced, totalSamples));
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

    constexpr float kSampleRate = 80000.f;
    constexpr auto  kDuration   = 2s;

    auto& ps = flowGraph.emplaceBlock<PicoscopeT<T, AcquisitionMode::Streaming>>({{{"sample_rate", kSampleRate},                                                           //
        {"acquisition_mode", "Streaming"}, {"streaming_mode_poll_rate", 0.00001f}, {"auto_arm", true}, {"channel_ids", std::vector<std::string>{"A"}},                     //
        {"channel_names", std::vector<std::string>{"Test signal"}}, {"channel_units", std::vector<std::string>{"Test unit"}}, {"channel_ranges", std::vector<float>{5.f}}, //
        {"channel_couplings", std::vector<std::string>{"AC_1M"}}}});

    auto& tagMonitor = flowGraph.emplaceBlock<testing::TagMonitor<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", true}}});
    auto& sinkA      = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
    auto& sinkB      = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
    auto& sinkC      = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
    auto& sinkD      = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
    auto& sinkE      = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
    auto& sinkF      = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
    auto& sinkG      = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
    auto& sinkH      = flowGraph.emplaceBlock<testing::TagSink<T, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});

    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).template to<"in">(tagMonitor)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out">(tagMonitor).template to<"in">(sinkA)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkD)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 4>(ps).template to<"in">(sinkE)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 5>(ps).template to<"in">(sinkF)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 6>(ps).template to<"in">(sinkG)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 7>(ps).template to<"in">(sinkH)));

    // Explicitly start unit because it takes quite some time
    expect(nothrow([&ps] { ps.start(); }));

    scheduler::Simple<scheduler::ExecutionPolicy::multiThreaded> sched{std::move(flowGraph)};
    expect(sched.changeStateTo(lifecycle::State::INITIALISED).has_value());
    expect(sched.changeStateTo(lifecycle::State::RUNNING).has_value());
    std::this_thread::sleep_for(kDuration);
    expect(sched.changeStateTo(lifecycle::State::REQUESTED_STOP).has_value());

    const auto measuredRate = static_cast<double>(sinkA._nSamplesProduced) / duration<double>(kDuration).count();
    fmt::println("Produced in worker: {}", ps.producedWorker());
    fmt::println("Configured rate: {}, Measured rate: {} ({:.2f}%), Duration: {} ms", kSampleRate, static_cast<std::size_t>(measuredRate), measuredRate / static_cast<double>(kSampleRate) * 100., duration_cast<milliseconds>(kDuration).count());
    fmt::println("Total samples: {}", sinkA._nSamplesProduced);
    fmt::println("Total tags: {}", tagMonitor._tags.size());

    expect(ge(sinkA._nSamplesProduced, 80000UZ));
    expect(le(sinkA._nSamplesProduced, 170000UZ));
    expect(ge(tagMonitor._tags.size(), 1UZ));
    if (tagMonitor._tags.size() == 1UZ) {
        const auto& tag = tagMonitor._tags[0];
        expect(eq(tag.index, int64_t{0}));
        expect(eq(std::get<float>(tag.at(std::string(tag::SAMPLE_RATE.shortKey()))), kSampleRate));
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
        auto& ps = flowGraph.emplaceBlock<PicoscopeT<float, AcquisitionMode::RapidBlock>>();

        // this takes time, so we do it a few times only
        for (auto i = 0; i < 3; i++) {
            expect(nothrow([&ps] { ps.initialize(); }));
            expect(neq(ps.driverVersion(), std::string()));
            expect(neq(ps.hardwareVersion(), std::string()));
            expect(nothrow([&ps] { ps.close(); })) << "doesn't throw";
        }
    };

    "streaming basics"_test = [] {
        testStreamingBasics<int16_t>();
        testStreamingBasics<float>();
        testStreamingBasics<gr::UncertainValue<float>>();
    };

    "rapid block basics"_test = [] {
        testRapidBlockBasic<int16_t>(1);
        testRapidBlockBasic<float>(1);
        testRapidBlockBasic<gr::UncertainValue<float>>(1);
    };

    "rapid block multiple captures"_test = [] { testRapidBlockBasic<float>(3); };

    "rapid block 4 channels"_test = [] {
        constexpr gr::Size_t kPreSamples   = 33;
        constexpr gr::Size_t kPostSamples  = 1000;
        constexpr gr::Size_t kNrCaptures   = 2;
        constexpr gr::Size_t kTotalSamples = kNrCaptures * (kPreSamples + kPostSamples);

        Graph flowGraph;
        auto& ps = flowGraph.emplaceBlock<PicoscopeT<float, AcquisitionMode::RapidBlock>>({{{"sample_rate", 10000.f},                                   //
            {"pre_samples", kPreSamples}, {"post_samples", kPostSamples}, {"acquisition_mode", "RapidBlock"}, {"rapid_block_nr_captures", kNrCaptures}, //
            {"auto_arm", true}, {"trigger_once", true}, {"channel_ids", std::vector<std::string>{"A", "B", "C", "D"}},                                  //
            {"channel_ranges", std::vector<float>{{5.f, 5.f, 5.f, 5.f}}}, {"channel_couplings", std::vector<std::string>{"AC_1M", "AC_1M", "AC_1M", "AC_1M"}}}});

        auto& sinkA = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
        auto& sinkB = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
        auto& sinkC = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
        auto& sinkD = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
        auto& sinkE = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
        auto& sinkF = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
        auto& sinkG = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
        auto& sinkH = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});

        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).template to<"in">(sinkA)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkD)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 4>(ps).template to<"in">(sinkE)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 5>(ps).template to<"in">(sinkF)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 6>(ps).template to<"in">(sinkG)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 7>(ps).template to<"in">(sinkH)));

        scheduler::Simple sched{std::move(flowGraph)};
        sched.runAndWait();

        expect(eq(sinkA._nSamplesProduced, kTotalSamples));
        expect(eq(sinkB._nSamplesProduced, kTotalSamples));
        expect(eq(sinkC._nSamplesProduced, kTotalSamples));
        expect(eq(sinkD._nSamplesProduced, kTotalSamples));
    };

    "rapid block continuous"_test = [] {
        using namespace std::chrono_literals;
        Graph flowGraph;
        auto& ps = flowGraph.emplaceBlock<PicoscopeT<float, AcquisitionMode::RapidBlock>>({{{"sample_rate", 10000.f},                               //
            {"post_samples", gr::Size_t{1000}}, {"acquisition_mode", "RapidBlock"}, {"rapid_block_nr_captures", gr::Size_t{1}}, {"auto_arm", true}, //
            {"channel_ids", std::vector<std::string>{"A"}}, {"channel_ranges", std::vector<float>{5.f}}, {"channel_couplings", std::vector<std::string>{"AC_1M"}}}});

        auto& sinkA = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
        auto& sinkB = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
        auto& sinkC = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
        auto& sinkD = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
        auto& sinkE = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
        auto& sinkF = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
        auto& sinkG = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
        auto& sinkH = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});

        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).to<"in">(sinkA)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkD)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 4>(ps).template to<"in">(sinkE)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 5>(ps).template to<"in">(sinkF)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 6>(ps).template to<"in">(sinkG)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 7>(ps).template to<"in">(sinkH)));

        scheduler::Simple<scheduler::ExecutionPolicy::multiThreaded> sched{std::move(flowGraph)};
        expect(sched.changeStateTo(lifecycle::State::INITIALISED).has_value());
        expect(sched.changeStateTo(lifecycle::State::RUNNING).has_value());
        std::this_thread::sleep_for(3s);
        expect(sched.changeStateTo(lifecycle::State::REQUESTED_STOP).has_value());

        expect(ge(sinkA._nSamplesProduced, std::size_t{2000}));
        expect(le(sinkA._nSamplesProduced, std::size_t{10000}));
    };
};

} // namespace fair::picoscope::test

int main() { /* tests are statically executed */ }
