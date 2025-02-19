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
    using PicoscopeT = Picoscope4000a<SampleType, AcquisitionMode::Streaming>;

    constexpr float kSampleRate = 20'000'000.f;

    auto threadPool = std::make_shared<gr::thread_pool::BasicThreadPool>("custom pool", gr::thread_pool::CPU_BOUND, 2, 2);

    Graph graph;

    auto& ps = graph.emplaceBlock<PicoscopeT>({{{"sample_rate", kSampleRate},              //
        {"acquisition_mode", "Streaming"}, {"auto_arm", true},                             //
        {"channel_ids", std::vector<std::string>{"A", "B", "C", "D", "E", "F", "G", "H"}}, //
        {"channel_ranges", std::vector<float>{5.f, 5.f, 5.f, 5.f, 5.f, 5.f, 5.f, 5.f}}}});

    auto& perfMonitorA = graph.emplaceBlock<PerformanceMonitor<SampleType>>({{"name", "Perf A"}});
    auto& perfMonitorB = graph.emplaceBlock<PerformanceMonitor<SampleType>>({{"name", "Perf B"}});
    auto& perfMonitorC = graph.emplaceBlock<PerformanceMonitor<SampleType>>({{"name", "Perf C"}});
    auto& perfMonitorD = graph.emplaceBlock<PerformanceMonitor<SampleType>>({{"name", "Perf D"}});
    auto& perfMonitorE = graph.emplaceBlock<PerformanceMonitor<SampleType>>({{"name", "Perf E"}});
    auto& perfMonitorF = graph.emplaceBlock<PerformanceMonitor<SampleType>>({{"name", "Perf F"}});
    auto& sinkG        = graph.emplaceBlock<testing::TagSink<SampleType, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});
    auto& sinkH        = graph.emplaceBlock<testing::TagSink<SampleType, testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", false}, {"log_tags", false}}});

    expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 0>(ps).template to<"in">(perfMonitorA)));
    expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 1>(ps).template to<"in">(perfMonitorB)));
    expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 2>(ps).template to<"in">(perfMonitorC)));
    expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 3>(ps).template to<"in">(perfMonitorD)));
    expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 4>(ps).template to<"in">(perfMonitorE)));
    expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 5>(ps).template to<"in">(perfMonitorF)));
    expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 6>(ps).template to<"in">(sinkG)));
    expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 7>(ps).template to<"in">(sinkH)));

    auto sched                                        = scheduler::Simple{std::move(graph), threadPool};
    auto [watchdogThread, externalInterventionNeeded] = createWatchdog(sched, runTime > 0 ? std::chrono::seconds(runTime) : 20s);
    expect(sched.runAndWait().has_value());

    if (watchdogThread.joinable()) {
        watchdogThread.join();
    }
}
