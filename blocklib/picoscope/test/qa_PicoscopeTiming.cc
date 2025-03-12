#include <boost/ut.hpp>

#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/testing/TagMonitors.hpp>

#include <HelperBlocks.hpp>
#include <Picoscope4000a.hpp>
#include <TimingSource.hpp>

using namespace std::string_literals;

namespace fair::picoscope::test {

const boost::ut::suite PicoscopeTests = [] {
    using namespace boost::ut;
    using namespace gr;
    using namespace fair::helpers;
    using namespace fair::picoscope;

    /**
     * This is a special hardware test which needs a properly configured timing receiver with its IO1..3 connected to the Picoscopes input A..C.
     * It will generate a known pattern of timing events, configure the timing receiver to generate pules on IO1 for each event and output a known pattern on IO2..3.
     * The flowgraph in this test will then store the corresponding output samples and tags and check that the tags and output levels are correct.
     */
    "streamingWithTiming"_test = [] {
        using namespace std::chrono;
        using namespace std::chrono_literals;
        using namespace boost::ut;

        fmt::println("testStreamingBasicsWithTiming");

        constexpr float kSampleRate = 80000.f; // [Hz]
        constexpr auto  kDuration   = 2s;

        gr::Graph flowGraph;
        auto&     timingSrc = flowGraph.emplaceBlock<gr::timing::TimingSource>({
            {"event_actions", std::vector<std::string>({"SIS100_RING:CMD_CUSTOM_DIAG_1->IO1(100,on,150,off),PUBLISH()"})}, // create a 50us pulse 100us after the timing event
            {"sample_rate", 0.0f},                                                                                         //
            {"verbose_console", false}                                                                                     //
        });

        auto& ps = flowGraph.emplaceBlock<Picoscope4000a<float>>({{
            {"sample_rate", kSampleRate},                                        //
            {"auto_arm", true},                                                  //
            {"channel_ids", std::vector<std::string>{"A", "B", "C"}},            //
            {"signal_names", std::vector<std::string>{"Trigger", "IO2", "IO3"}}, //
            {"signal_units", std::vector<std::string>{"V", "V", "V"}},           //
            {"channel_ranges", std::vector<float>{5.f, 5.f, 5.f}},               //
            {"signal_offsets", std::vector<float>{0.f, 0.f, 0.f}},               //
            {"channel_couplings", std::vector<std::string>{"DC", "DC", "DC"}},   //
            {"trigger_source", "A"},                                             //
            {"trigger_threshold", 1.7f},                                         //
            {"trigger_direction", "Rising"}                                      //
        }});

        auto& sinkA = flowGraph.emplaceBlock<gr::testing::TagSink<float, gr::testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", true}, {"log_tags", true}}});
        auto& sinkB = flowGraph.emplaceBlock<gr::testing::TagSink<float, gr::testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", true}, {"log_tags", true}}});
        auto& sinkC = flowGraph.emplaceBlock<gr::testing::TagSink<float, gr::testing::ProcessFunction::USE_PROCESS_BULK>>({{{"log_samples", true}, {"log_tags", true}}});

        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out">(timingSrc).template to<"timingIn">(ps)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).template to<"in">(sinkA)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));

        // Explicitly start unit because it takes quite some time
        expect(nothrow([&ps] { ps.start(); }));

        std::jthread                                                 publishEvents([]() {
            std::vector<std::pair<std::uint64_t, std::variant<Timing::Event, std::uint8_t>>> events{
                                                                {500'000'000, std::uint8_t{0b000}},                                                    //
                                                                {800'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 286, 1)}},   //
                                                                {1'000'000'000, std::uint8_t{0b010}},                                                  //
                                                                {1'400'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 286, 2)}}, //
                                                                {1'500'000'000, std::uint8_t{0b100}},                                                  //
                                                                {1'700'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 286, 3)}}, //
                                                                {2'000'000'000, std::uint8_t{0b110}},                                                  //
                                                                {2'500'000'000, std::uint8_t{0b000}}                                                   //
            };
            std::size_t schedule_offset = 100'000'000;
            fmt::print("start replay of test timing data\n");
            Timing timing;
            timing.saftAppName = fmt::format("qa_picoscope_timing_dispatcher_{}", getpid());
            timing.initialize();
            auto          outputs     = timing.receiver->getOutputs() | std::views::transform([&timing](auto kvp) {
                auto [name, path] = kvp;
                auto result       = saftlib::Output_Proxy::create(path, timing.saftSigGroup);
                result->setOutputEnable(true);
                result->WriteOutput(false);
                return result;
            }) | std::ranges::to<std::vector>();
            std::uint64_t outputState = 0x0ull;
            auto          start       = timing.currentTimeTAI() + schedule_offset;
            std::this_thread::sleep_for(200ms);
            for (auto& [schedule_dt, action] : events) {
                while (start + schedule_dt > timing.currentTimeTAI() + schedule_offset) {
                    timing.saftSigGroup.wait_for_signal(0);
                }
                if (std::holds_alternative<std::uint8_t>(action)) { // toggle outputs
                    auto newState = std::get<std::uint8_t>(action);
                    for (std::size_t j = 0; j < outputs.size(); j++) {
                        if ((outputState & (0x1ull << j)) != ((newState) & (0x1ull << j))) {
                            const bool state = ((newState) & (0x1ull << (j))) > 0;
                            outputs[j]->WriteOutput(state);
                        }
                    }
                    outputState = newState;
                    // fmt::print("changed outputs: {:0b}\n", newState);
                } else if (std::holds_alternative<Timing::Event>(action)) { // publish event
                    auto event = std::get<Timing::Event>(action);
                    event.time = schedule_dt;
                    timing.injectEvent(event, start);
                    timing.saftSigGroup.wait_for_signal(0);
                    // fmt::print("sent event: id:{:#x}, gid:{}, evNo:{}, bpid:{}\n", event.id(), event.gid, event.eventNo, event.bpid);
                } else {
                    expect(false) << "unsupported event action";
                }
            }
            fmt::print("finished replay of timing data\n");
            return;
        });
        scheduler::Simple<scheduler::ExecutionPolicy::multiThreaded> sched{std::move(flowGraph)};
        expect(sched.changeStateTo(lifecycle::State::INITIALISED).has_value());
        expect(sched.changeStateTo(lifecycle::State::RUNNING).has_value());
        std::this_thread::sleep_for(kDuration);
        expect(sched.changeStateTo(lifecycle::State::REQUESTED_STOP).has_value());

        const auto measuredRate = static_cast<double>(sinkA._nSamplesProduced) / duration<double>(kDuration).count();
        fmt::println("Produced in worker: {}", ps._nSamplesPublished);
        fmt::println("Configured rate: {}, Measured rate: {} ({:.2f}%), Duration: {} ms", kSampleRate, static_cast<std::size_t>(measuredRate), measuredRate / static_cast<double>(kSampleRate) * 100., duration_cast<milliseconds>(kDuration).count());
        fmt::println("Total: {}", sinkA._nSamplesProduced);

        expect(ge(sinkA._nSamplesProduced, 80000UZ));
        expect(le(sinkA._nSamplesProduced, 170000UZ));
        if (sinkA._tags.size() == 1UZ) {
            const auto& tag = sinkA._tags[0];
            expect(eq(tag.index, 0UZ));
            expect(eq(std::get<float>(tag.at(std::string(tag::SAMPLE_RATE.shortKey()))), kSampleRate));
            expect(eq(std::get<std::string>(tag.at(std::string(tag::SIGNAL_NAME.shortKey()))), "Test signal"s));
            expect(eq(std::get<std::string>(tag.at(std::string(tag::SIGNAL_UNIT.shortKey()))), "Test unit"s));
            expect(eq(std::get<float>(tag.at(std::string(tag::SIGNAL_MIN.shortKey()))), 0.f));
            expect(eq(std::get<float>(tag.at(std::string(tag::SIGNAL_MAX.shortKey()))), 5.f));
        }
        expect(std::ranges::equal(sinkA._tags | std::views::filter([](auto& t) { return t.map.contains("BPID"); })        // only consider timing events
                                      | std::views::transform([](auto& t) { return std::get<uint16_t>(t.map["BPID"]); }), // get bpid (which is unique in this test)
            std::vector<std::uint16_t>{1, 2, 3}));
        auto timingEventSamplesFromTags = sinkA._tags | std::views::filter([](auto& t) { return t.map.contains("BPID"); }) // only consider timing events
                                          | std::views::transform([](auto& t) { return t.index; })                         // get tag index
                                          | std::ranges::to<std::vector>();
        expect(eq(timingEventSamplesFromTags.size(), 3UZ)) << "expected to get exactly 3 timing tags";
        expect(approx(static_cast<double>(timingEventSamplesFromTags[1] - timingEventSamplesFromTags[0]), (1'400'000'000 - 800'000'000) * 1e-9 * static_cast<double>(kSampleRate), 1.0)) << "sample distance between first and second tag does not match sample rate";
        expect(approx(static_cast<double>(timingEventSamplesFromTags[2] - timingEventSamplesFromTags[0]), (1'700'000'000 - 800'000'000) * 1e-9 * static_cast<double>(kSampleRate), 1.0)) << "sample distance between first and third tag does not match sample rate";
        expect(approx(sinkA._samples[timingEventSamplesFromTags[0] - 2], 0.0f, 1e-1f)) << "trigger channel should be zero ahead of timing event 0";
        expect(approx(sinkA._samples[timingEventSamplesFromTags[0] + 2], 3.0f, 1e-1f)) << "trigger channel should be zero after of timing event 0";
        expect(approx(sinkB._samples[timingEventSamplesFromTags[0]], 0.0f, 1e-1f)) << "state of IO2=inB should be LOW at timing event 0";
        expect(approx(sinkC._samples[timingEventSamplesFromTags[0]], 0.0f, 1e-1f)) << "state of IO2=inB should be LOW at timing event 0";
        expect(approx(sinkA._samples[timingEventSamplesFromTags[1] - 2], 0.0f, 1e-1f)) << "trigger channel should be zero ahead of timing event 1";
        expect(approx(sinkA._samples[timingEventSamplesFromTags[1] + 2], 3.0f, 1e-1f)) << "trigger channel should be zero after of timing event 1";
        expect(approx(sinkB._samples[timingEventSamplesFromTags[1]], 3.3f, 1e-1f)) << "state of IO2=inB should be HIGH at timing event 1";
        expect(approx(sinkC._samples[timingEventSamplesFromTags[1]], 0.0f, 1e-1f)) << "state of IO2=inB should be LOW at timing event 1";
        expect(approx(sinkA._samples[timingEventSamplesFromTags[2] - 2], 0.0f, 1e-1f)) << "trigger channel should be zero ahead of timing event 2";
        expect(approx(sinkA._samples[timingEventSamplesFromTags[2] + 2], 3.0f, 1e-1f)) << "trigger channel should be zero after of timing event 2";
        expect(approx(sinkB._samples[timingEventSamplesFromTags[2]], 0.0f, 1e-1f)) << "state of IO2=inB should be LOW at timing event 2";
        expect(approx(sinkC._samples[timingEventSamplesFromTags[2]], 3.3f, 1e-1f)) << "state of IO2=inB should be HIGH at timing event 2";
        fmt::print("timing events: {}\n", sinkA._tags | std::views::filter([](auto& t) { return t.map.contains("BPID"); }) | std::views::transform([](auto& t) { return std::pair(t.index, t.map); }));
    };
};

} // namespace fair::picoscope::test

int main() { /* tests are statically executed */ }
