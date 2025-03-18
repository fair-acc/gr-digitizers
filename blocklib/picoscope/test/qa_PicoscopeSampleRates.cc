#include <HelperBlocks.hpp>
#include <Picoscope4000a.hpp>
#include <boost/ut.hpp>
#include <fmt/format.h>
#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/testing/NullSources.hpp>
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

    auto runTime = 60s;
    if (argc >= 2) {
        runTime = std::chrono::seconds{std::atol(argv[1])};
    }

    float kSampleRate = 100'000.f;
    if (argc >= 3) {
        kSampleRate = static_cast<float>(std::atof(argv[2]));
    }


    using SampleType = float;

    // Replace with your connected Picoscope device
    using PicoscopeT = Picoscope4000a<SampleType>;

    auto threadPool = std::make_shared<gr::thread_pool::BasicThreadPool>("custom pool", gr::thread_pool::CPU_BOUND, 2, 2);

    Graph graph;

    auto& ps = graph.emplaceBlock<PicoscopeT>({{{"sample_rate", kSampleRate}, {"auto_arm", true}, //
        {"channel_ids", std::vector<std::string>{"A", "B", "C", "D", "E", "F", "G", "H"}},        //
        {"channel_ranges", std::vector<float>{5.f, 5.f, 5.f, 5.f, 5.f, 5.f, 5.f, 5.f}}}});

    auto& perfMonitorDigi = graph.emplaceBlock<NullSink<std::uint16_t>>();

    //auto& perfMonitorA = graph.emplaceBlock<ImChartMonitor<SampleType, false>>({{"name", "Pico A"}, {"plot_graph", true}, {"plot_timing", true}});
    //auto& perfMonitorB = graph.emplaceBlock<ImChartMonitor<SampleType, false>>({{"name", "Pico B"}, {"plot_graph", false}, {"plot_timing", false}});
    //auto& perfMonitorC = graph.emplaceBlock<ImChartMonitor<SampleType, false>>({{"name", "Pico C"}, {"plot_graph", false}, {"plot_timing", false}});
    //auto& perfMonitorD = graph.emplaceBlock<ImChartMonitor<SampleType, false>>({{"name", "Pico D"}, {"plot_graph", false}, {"plot_timing", false}});
    auto& perfMonitorA = graph.emplaceBlock<NullSink<SampleType>>();
    auto& perfMonitorB = graph.emplaceBlock<NullSink<SampleType>>();
    auto& perfMonitorC = graph.emplaceBlock<NullSink<SampleType>>();
    auto& perfMonitorD = graph.emplaceBlock<NullSink<SampleType>>();

    expect(eq(ConnectionResult::SUCCESS, graph.connect<"digitalOut">(ps).template to<"in">(perfMonitorDigi)));

    expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 0>(ps).template to<"in">(perfMonitorA)));
    expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 1>(ps).template to<"in">(perfMonitorB)));
    expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 2>(ps).template to<"in">(perfMonitorC)));
    expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 3>(ps).template to<"in">(perfMonitorD)));

    if constexpr (std::is_same_v<Picoscope4000a<SampleType>, PicoscopeT>) {
        auto& perfMonitorE = graph.emplaceBlock<NullSink<SampleType>>();
        auto& perfMonitorF = graph.emplaceBlock<NullSink<SampleType>>();
        auto& perfMonitorG = graph.emplaceBlock<NullSink<SampleType>>();
        auto& perfMonitorH = graph.emplaceBlock<NullSink<SampleType>>();
        //auto& perfMonitorE = graph.emplaceBlock<ImChartMonitor<SampleType, false>>({{"name", "Pico E"}, {"plot_graph", false}, {"plot_timing", false}});
        //auto& perfMonitorF = graph.emplaceBlock<ImChartMonitor<SampleType, false>>({{"name", "Pico F"}, {"plot_graph", false}, {"plot_timing", false}});
        //auto& perfMonitorG = graph.emplaceBlock<ImChartMonitor<SampleType, false>>({{"name", "Pico E"}, {"plot_graph", false}, {"plot_timing", false}});
        //auto& perfMonitorH = graph.emplaceBlock<ImChartMonitor<SampleType, false>>({{"name", "Pico F"}, {"plot_graph", false}, {"plot_timing", false}});

        expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 4>(ps).template to<"in">(perfMonitorE)));
        expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 5>(ps).template to<"in">(perfMonitorF)));
        expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 6>(ps).template to<"in">(perfMonitorG)));
        expect(eq(ConnectionResult::SUCCESS, graph.connect<"out", 7>(ps).template to<"in">(perfMonitorH)));
    }

    auto sched                                        = scheduler::Simple{std::move(graph), threadPool};
    auto [watchdogThread, externalInterventionNeeded] = createWatchdog(sched, runTime > 0s ? runTime : 20s);
    expect(sched.runAndWait().has_value());

    //fmt::print("ImplotSinkA processed {} samples\n", perfMonitorA._n_samples_total);

    if (watchdogThread.joinable()) {
        watchdogThread.join();
    }
}
