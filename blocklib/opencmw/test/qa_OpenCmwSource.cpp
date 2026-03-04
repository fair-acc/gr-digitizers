#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>

#include <fair/opencmw/OpenCmwSource.hpp>
#include <gnuradio-4.0/algorithm/ImChart.hpp>
#include <gnuradio-4.0/meta/UnitTestHelper.hpp>
#include <gnuradio-4.0/testing/TagMonitors.hpp>

#include <boost/ut.hpp>

#include <print>

#include "ProcessHelper.hpp"

const boost::ut::suite OpenCmwSourceTests = [] {
    using namespace boost::ut;
    using namespace std::literals;
    using namespace gr;
    using namespace gr::testing;

    "OpenCmwSourceBlockDev"_test = [](bool verbose = true) {
        /* This unit-test depends on the availability of internal infrastructure and will be skipped if the required environment variables are not set up */
        std::println("# CmwSourceBlock DEV - started");

        const char* env_nameserver = std::getenv("CMW_NAMESERVER");
        if (env_nameserver == nullptr) {
            std::println("skipping BasicCmwLight example test as it relies on the availability of network infrastructure. Define CMW_NAMESERVER environment variable to run this test.");
            return; // skip test
        }

        std::string DEVICE_NAME = "GSITemplateDevice";
        std::string deviceHost{""}; // by default, perform name lookup on the nameserver, explicitly setting this to localhost might be needed in case ssh tunnels are required
        if (const char* env_cmw_host = std::getenv("CMW_DEVICE_HOST"); env_cmw_host != nullptr) {
            deviceHost = std::string{env_cmw_host};
        }

        Graph testGraph;
        auto& src                 = testGraph.emplaceBlock<fair::opencmw::OpenCmwSource>({
            {"url", "rda3://" + deviceHost + "/GSITemplateDevice/Acquisition?ctx=FAIR.SELECTOR.ALL"},
            {"verbose_console", verbose},
        });
        auto& extractFromMapBlock = testGraph.emplaceBlock<fair::opencmw::ExtractFromMap<double>>({{"fieldname", "voltage"}});
        auto& sink                = testGraph.emplaceBlock<TagSink<double, ProcessFunction::USE_PROCESS_ONE>>({{"name", "TagSink1"}});
        expect(eq(ConnectionResult::SUCCESS, testGraph.connect<"out">(src).template to<"in">(extractFromMapBlock)));
        expect(eq(ConnectionResult::SUCCESS, testGraph.connect<"out">(extractFromMapBlock).template to<"in">(sink)));

        scheduler::Simple<scheduler::ExecutionPolicy::multiThreaded> sched{};
        std::ignore = sched.exchange(std::move(testGraph));
        MsgPortOut _toScheduler;
        expect(_toScheduler.connect(sched.msgIn) == ConnectionResult::SUCCESS) << fatal;
        expect(sched.changeStateTo(lifecycle::State::INITIALISED).has_value());
        expect(sched.changeStateTo(lifecycle::State::RUNNING).has_value());
        std::this_thread::sleep_for(5s);
        std::println("stopping scheduler");
        gr::sendMessage<message::Command::Set>(_toScheduler, sched.unique_name, block::property::kLifeCycleState, {{"state", std::string(magic_enum::enum_name(lifecycle::State::REQUESTED_STOP))}}, "test");

        expect(sink._nSamplesProduced == 4_i);
        std::println("sink received samples: {}", sink._samples);

        std::println("# CmwSourceBlock - finished");
    };

    "OpenCmwSourceBlockLocal"_test = [](bool verbose = true) {
        std::println("# CmwSourceBlock local mock rda3 server - started");

        Process rda3Server({"java", "-jar", "./DemoRda3Server.jar", "-cfg", "--cmw.rda3.transport.server.port=12345", "-sleep", "1400"});
        std::this_thread::sleep_for(10ms); // some time for rda3 server startup

        std::string DEVICE_NAME = "GSITemplateDevice";
        std::string deviceHost{"rda3://localhost:12345"};

        Graph testGraph;
        auto& src                 = testGraph.emplaceBlock<fair::opencmw::OpenCmwSource>({
            {"url", deviceHost + "/GSITemplateDevice/Acquisition?ctx=FAIR.SELECTOR.ALL"},
            {"verbose_console", verbose},
        });
        auto& extractFromMapBlock = testGraph.emplaceBlock<fair::opencmw::ExtractFromMap<double>>({{"fieldname", "voltage"}});
        auto& sink                = testGraph.emplaceBlock<TagSink<double, ProcessFunction::USE_PROCESS_ONE>>({{"name", "TagSink1"}});
        expect(eq(ConnectionResult::SUCCESS, testGraph.connect<"out">(src).template to<"in">(extractFromMapBlock)));
        expect(eq(ConnectionResult::SUCCESS, testGraph.connect<"out">(extractFromMapBlock).template to<"in">(sink)));

        scheduler::Simple<scheduler::ExecutionPolicy::multiThreaded> sched{};
        std::ignore = sched.exchange(std::move(testGraph));
        MsgPortOut _toScheduler;
        expect(_toScheduler.connect(sched.msgIn) == ConnectionResult::SUCCESS) << fatal;
        expect(sched.changeStateTo(lifecycle::State::INITIALISED).has_value());
        expect(sched.changeStateTo(lifecycle::State::RUNNING).has_value());
        std::this_thread::sleep_for(5s);
        std::println("stopping scheduler");
        gr::sendMessage<message::Command::Set>(_toScheduler, sched.unique_name, block::property::kLifeCycleState, {{"state", std::string(magic_enum::enum_name(lifecycle::State::REQUESTED_STOP))}}, "test");

        expect(sink._nSamplesProduced == 4_i);
        std::println("sink received samples: {}", sink._samples);

        std::println("# CmwSourceBlock - finished");
    };
};

int main() { /* tests are statically executed */ }
