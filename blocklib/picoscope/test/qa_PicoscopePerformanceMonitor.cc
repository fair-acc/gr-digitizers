#include <HelperBlocks.hpp>
#include <Picoscope4000a.hpp>
#include <Picoscope5000a.hpp>
#include <boost/ut.hpp>
#include <fmt/format.h>
#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/testing/PerformanceMonitor.hpp>
#include <gnuradio-4.0/testing/TagMonitors.hpp>
#include <string>

namespace {
using namespace std::chrono_literals;
template<typename Scheduler>
auto createWatchdog(Scheduler& sched, std::chrono::seconds timeOut = 2s, std::chrono::milliseconds pollingPeriod = 40ms) {
    auto externalInterventionNeeded = std::make_shared<std::atomic_bool>(false);

    std::thread watchdogThread([&sched, externalInterventionNeeded, timeOut, pollingPeriod]() {
        auto timeout = std::chrono::steady_clock::now() + timeOut;
        while (std::chrono::steady_clock::now() < timeout) {
            if (sched.state() == gr::lifecycle::State::STOPPED) {
                return;
            }
            std::this_thread::sleep_for(pollingPeriod);
        }
        fmt::println("watchdog kicked in");
        externalInterventionNeeded->store(true, std::memory_order_relaxed);
        sched.requestStop();
        fmt::println("requested scheduler to stop");
    });

    return std::make_pair(std::move(watchdogThread), externalInterventionNeeded);
}
} // namespace

int main(int argc, char* argv[]) {
    using namespace boost::ut;
    using namespace gr;
    using namespace gr::testing;
    using namespace fair::helpers;
    using namespace fair::picoscope;

    int runTime = 60; // in seconds

    if (argc >= 2) {
        runTime = std::atoi(argv[1]);
    }

    using SampleType = float;

    // Replace with your connected Picoscope device
    using PicoscopeT = Picoscope5000a<SampleType>;

    constexpr float      kSampleRate      = 1'000'000.f;
    constexpr gr::Size_t evaluatePerfRate = 1'000'000;
    constexpr float      publishRate      = 1.f;

    auto threadPool = std::make_shared<gr::thread_pool::BasicThreadPool>("custom pool", gr::thread_pool::CPU_BOUND, 2, 2);

    Graph graph;

    std::vector<std::string> channelIds    = {"A", "B", "C", "D"};
    std::vector<float>       channelRanges = {5.f, 5.f, 5.f, 5.f};
    if constexpr (std::is_same_v<Picoscope4000a<SampleType>, PicoscopeT>) {
        channelIds.insert(channelIds.end(), {"E", "F", "G", "H"});
        channelRanges.insert(channelRanges.end(), {5.f, 5.f, 5.f, 5.f});
    }

    auto& ps = graph.emplaceBlock<PicoscopeT>({{{"sample_rate", kSampleRate}, {"auto_arm", true}, //
        {"channel_ids", channelIds}, {"channel_ranges", channelRanges}}});

    auto& perfMonitorA = graph.emplaceBlock<PerformanceMonitor<SampleType>>({{"name", "Perf A"}, {"evaluate_perf_rate", evaluatePerfRate}, {"publish_rate", publishRate}});
    auto& perfMonitorB = graph.emplaceBlock<PerformanceMonitor<SampleType>>({{"name", "Perf B"}, {"evaluate_perf_rate", evaluatePerfRate}, {"publish_rate", publishRate}});
    auto& perfMonitorC = graph.emplaceBlock<PerformanceMonitor<SampleType>>({{"name", "Perf C"}, {"evaluate_perf_rate", evaluatePerfRate}, {"publish_rate", publishRate}});
    auto& perfMonitorD = graph.emplaceBlock<PerformanceMonitor<SampleType>>({{"name", "Perf D"}, {"evaluate_perf_rate", evaluatePerfRate}, {"publish_rate", publishRate}});

    expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 0>(ps).template to<"in">(perfMonitorA)));
    expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 1>(ps).template to<"in">(perfMonitorB)));
    expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 2>(ps).template to<"in">(perfMonitorC)));
    expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 3>(ps).template to<"in">(perfMonitorD)));

    auto& sinkDigital = graph.emplaceBlock<testing::TagSink<uint16_t, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
    expect(eq(ConnectionResult::SUCCESS, graph.connect<"digitalOut">(ps).template to<"in">(sinkDigital)));

    if constexpr (std::is_same_v<Picoscope4000a<SampleType>, PicoscopeT>) {
        auto& perfMonitorE = graph.emplaceBlock<PerformanceMonitor<SampleType>>({{"name", "Perf E"}, {"evaluate_perf_rate", evaluatePerfRate}, {"publish_rate", publishRate}});
        auto& perfMonitorF = graph.emplaceBlock<PerformanceMonitor<SampleType>>({{"name", "Perf F"}, {"evaluate_perf_rate", evaluatePerfRate}, {"publish_rate", publishRate}});
        auto& sinkG        = graph.emplaceBlock<testing::TagSink<SampleType, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
        auto& sinkH        = graph.emplaceBlock<testing::TagSink<SampleType, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});

        expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 4>(ps).template to<"in">(perfMonitorE)));
        expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 5>(ps).template to<"in">(perfMonitorF)));
        expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 6>(ps).template to<"in">(sinkG)));
        expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 7>(ps).template to<"in">(sinkH)));
    }

    auto sched                                        = scheduler::Simple{std::move(graph), threadPool};
    auto [watchdogThread, externalInterventionNeeded] = createWatchdog(sched, runTime > 0 ? std::chrono::seconds(runTime) : 20s);
    expect(sched.runAndWait().has_value());

    if (watchdogThread.joinable()) {
        watchdogThread.join();
    }
}
