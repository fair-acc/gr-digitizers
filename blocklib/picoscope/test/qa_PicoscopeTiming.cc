#include <boost/ut.hpp>

#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/testing/TagMonitors.hpp>

#include <HelperBlocks.hpp>
#include <Picoscope4000a.hpp>
#include <TimingSource.hpp>

#include <format>
#include <print>

using namespace std::string_literals;

template<>
struct std::formatter<gr::Tag> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    constexpr auto format(const gr::Tag& tag, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "  {}->{{ {} }}\n", tag.index, tag.map);
    }
};

namespace fair::picoscope::test {

const boost::ut::suite<"PicoscopeTimingTests"> PicoscopeTimingTests = [] {
    using namespace boost::ut;
    using namespace gr;
    using namespace fair::helpers;
    using namespace fair::picoscope;

    // small helper to print the content of the ranges if there is a mismatch
    auto expectRangesEquals = [](const auto& r1, const auto& r2) { expect(std::ranges::equal(r1, r2)) << [&r1, &r2]() { return std::format("exp: {}\n got: {}", r1, r2); }; };

    auto createTimingEventThread = [](std::vector<std::pair<std::uint64_t, std::variant<Timing::Event, std::uint8_t>>> events, std::size_t schedule_offset) {
        return std::jthread([events, schedule_offset]() {
            std::println("start replay of test timing data");
            Timing             timing;
            static std::size_t instance = 0;
            timing.saftAppName          = std::format("qa_picoscope_timing_dispatcher_{}_{}", getpid(), instance++);
            timing.initialize();
            auto          outputs     = timing.receiver->getOutputs() | std::views::transform([&timing](auto kvp) {
                auto [outputName, path] = kvp;
                auto outputProxy        = saftlib::Output_Proxy::create(path, timing.saftSigGroup);
                outputProxy->setOutputEnable(true);
                outputProxy->WriteOutput(false);
                return outputProxy;
            }) | std::ranges::to<std::vector>();
            std::uint64_t outputState = 0x0ull;
            auto          start       = timing.currentTimeTAI() + schedule_offset;
            std::this_thread::sleep_for(200ms);
            for (auto& [schedule_dt, action] : events) {
                if (std::holds_alternative<std::uint8_t>(action)) { // toggle outputs
                    while (start + schedule_dt > timing.currentTimeTAI()) {
                        timing.saftSigGroup.wait_for_signal(0);
                    }
                    auto newState = std::get<std::uint8_t>(action);
                    for (std::size_t j = 0; j < outputs.size(); j++) {
                        if ((outputState & (0x1ull << j)) != ((newState) & (0x1ull << j))) {
                            const bool state = ((newState) & (0x1ull << (j))) > 0;
                            outputs[j]->WriteOutput(state);
                        }
                    }
                    outputState = newState;
                    // std::print("changed outputs: {:0b}\n", newState);
                } else if (std::holds_alternative<Timing::Event>(action)) { // publish event
                    while (start + schedule_dt > timing.currentTimeTAI() + schedule_offset) {
                        timing.saftSigGroup.wait_for_signal(0);
                    }
                    auto event = std::get<Timing::Event>(action);
                    event.time = schedule_dt;
                    timing.injectEvent(event, start);
                    timing.saftSigGroup.wait_for_signal(0);
                    // std::print("sent event: id:{:#x}, gid:{}, evNo:{}, bpid:{}\n", event.id(), event.gid, event.eventNo, event.bpid);
                } else {
                    expect(false) << "unsupported event action";
                }
            }
            timing.stop();
            std::println("finished replay of timing data");
            return;
        });
    };

    /**
     * This is a special hardware test which needs a properly configured timing receiver with its IO1..3 connected to the Picoscopes input A..C.
     * It will generate a known pattern of timing events, configure the timing receiver to generate pules on IO1 for each event and output a known pattern on IO2..3.
     * The flowgraph in this test will then store the corresponding output samples and tags and check that the tags and output levels are correct.
     */
    "streamingWithTiming"_test = [&] {
        using namespace std::chrono;
        using namespace std::chrono_literals;
        using namespace boost::ut;

        std::println("testStreamingBasicsWithTiming");

        constexpr float kSampleRate = 100000.f; // [Hz]
        constexpr auto  kDuration   = 2s;

        gr::Graph flowGraph;
        auto&     timingSrc = flowGraph.emplaceBlock<gr::timing::TimingSource>({
            {"event_actions", std::vector<std::string>({"SIS100_RING:CMD_CUSTOM_DIAG_1->IO1(100,on,150,off),PUBLISH()"})}, // create a 50us pulse 100us after the timing event
            {"event_hw_trigger", std::vector<std::string>({"SIS100_RING:CMD_CUSTOM_DIAG_1"})},                             // All diag events produce hw triggers, so set the HW-TRIGGER flag in their tags
            {"sample_rate", 0.0f},
            {"verbose_console", false},
        });

        auto& ps = flowGraph.emplaceBlock<Picoscope4000a<float>>({{
            {"sample_rate", kSampleRate},
            {"auto_arm", true},
            {"channel_ids", std::vector<std::string>{"A", "B", "C"}},
            {"signal_names", std::vector<std::string>{"Trigger", "IO2", "IO3"}},
            {"signal_units", std::vector<std::string>{"V", "V", "V"}},
            {"channel_ranges", std::vector<float>{5.f, 5.f, 5.f}},
            {"signal_offsets", std::vector<float>{0.f, 0.f, 0.f}},
            {"channel_couplings", std::vector<std::string>{"DC", "DC", "DC"}},
            {"trigger_source", "A"},
            {"trigger_threshold", 1.7f},
            {"trigger_direction", "Rising"},
        }});

        auto& sinkA = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", true}});
        auto& sinkB = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", true}});
        auto& sinkC = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", true}});
        auto& sinkD = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkE = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkF = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkG = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkH = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});

        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out">(timingSrc).template to<"timingIn">(ps)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).template to<"in">(sinkA)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkD)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 4>(ps).template to<"in">(sinkE)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 5>(ps).template to<"in">(sinkF)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 6>(ps).template to<"in">(sinkG)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 7>(ps).template to<"in">(sinkH)));

        auto& sinkDigital = flowGraph.emplaceBlock<testing::TagSink<uint16_t, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"digitalOut">(ps).template to<"in">(sinkDigital)));

        std::this_thread::sleep_for(1s);

        // Explicitly start unit because it takes quite some time
        expect(nothrow([&ps] { ps.start(); }));

        std::jthread publishEvents = createTimingEventThread(
            {
                {500'000'000, std::uint8_t{0b000}},
                {800'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 286, 1)}},
                {1'000'000'000, std::uint8_t{0b010}},
                {1'400'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 286, 2)}},
                {1'500'000'000, std::uint8_t{0b100}},
                {1'700'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 286, 3)}},
                {2'000'000'000, std::uint8_t{0b110}},
                {2'500'000'000, std::uint8_t{0b000}},
            },
            100'000'000);
        scheduler::Simple<scheduler::ExecutionPolicy::multiThreaded> sched{std::move(flowGraph)};
        expect(sched.changeStateTo(lifecycle::State::INITIALISED).has_value());
        expect(sched.changeStateTo(lifecycle::State::RUNNING).has_value());
        std::this_thread::sleep_for(kDuration);
        expect(sched.changeStateTo(lifecycle::State::REQUESTED_STOP).has_value());

        const auto measuredRate = static_cast<double>(sinkA._nSamplesProduced) / duration<double>(kDuration).count();
        std::println("Produced in worker: {}", ps._nSamplesPublished);
        std::println("Configured rate: {}, Measured rate: {} ({:.2f}%), Duration: {} ms", kSampleRate, static_cast<std::size_t>(measuredRate), measuredRate / static_cast<double>(kSampleRate) * 100., duration_cast<milliseconds>(kDuration).count());
        std::println("Total: {}", sinkA._nSamplesProduced);

        expect(ge(sinkA._nSamplesProduced, 100000UZ));
        expect(le(sinkA._nSamplesProduced, 220000UZ));
        if (!sinkA._tags.empty()) {
            const auto& tag = sinkA._tags[0];
            expect(eq(tag.index, 0UZ));
            expect(eq(std::get<float>(tag.at(std::string(tag::SAMPLE_RATE.shortKey()))), kSampleRate));
            expect(eq(std::get<std::string>(tag.at(std::string(tag::SIGNAL_NAME.shortKey()))), "Trigger"s));
            expect(eq(std::get<std::string>(tag.at(std::string(tag::SIGNAL_UNIT.shortKey()))), "V"s));
            expect(eq(std::get<float>(tag.at(std::string(tag::SIGNAL_MIN.shortKey()))), -5.f));
            expect(eq(std::get<float>(tag.at(std::string(tag::SIGNAL_MAX.shortKey()))), 5.f));
        }
        expect(std::ranges::equal(sinkA._tags | std::views::filter([](auto& t) { return t.map.contains(gr::tag::CONTEXT.shortKey()); })                                                      // only consider timing events
                                      | std::views::transform([](auto& t) { return std::get<uint16_t>(std::get<gr::property_map>(t.map[gr::tag::TRIGGER_META_INFO.shortKey()])["BPID"]); }), // get bpid (which is unique in this test)
            std::vector<std::uint16_t>{1, 2, 3}));
        auto timingEventSamplesFromTags = sinkA._tags | std::views::filter([](auto& t) { return t.map.contains(gr::tag::CONTEXT.shortKey()); }) // only consider timing events
                                          | std::views::transform([](auto& t) { return t.index; })                                              // get tag index
                                          | std::ranges::to<std::vector>();
        expect(eq(timingEventSamplesFromTags.size(), 3UZ)) << "expected to get exactly 3 timing tags" << fatal;
        expect(approx(static_cast<double>(timingEventSamplesFromTags[1] - timingEventSamplesFromTags[0]), (1'400'000'000 - 800'000'000) * 1e-9 * static_cast<double>(kSampleRate), 1.0)) << "sample distance between first and second tag does not match sample rate";
        expect(approx(static_cast<double>(timingEventSamplesFromTags[2] - timingEventSamplesFromTags[0]), (1'700'000'000 - 800'000'000) * 1e-9 * static_cast<double>(kSampleRate), 1.0)) << "sample distance between first and third tag does not match sample rate";
        expect(approx(sinkA._samples[timingEventSamplesFromTags[0] - 2], 0.0f, 1e-1f)) << "trigger channel should be zero ahead of timing event 0";
        expect(approx(sinkA._samples[timingEventSamplesFromTags[0] + 2], 3.0f, 1e-1f)) << "trigger channel should be zero after of timing event 0";
        expect(approx(sinkB._samples[timingEventSamplesFromTags[0]], 0.0f, 35e-2f)) << "state of IO2=inB should be LOW at timing event 0";
        // expect(approx(sinkC._samples[timingEventSamplesFromTags[0]], 0.0f, 35e-2f)) << "state of IO2=inB should be LOW at timing event 0";
        expect(approx(sinkA._samples[timingEventSamplesFromTags[1] - 2], 0.0f, 35e-2f)) << "trigger channel should be zero ahead of timing event 1";
        expect(approx(sinkA._samples[timingEventSamplesFromTags[1] + 2], 3.0f, 35e-2f)) << "trigger channel should be zero after of timing event 1";
        expect(approx(sinkB._samples[timingEventSamplesFromTags[1]], 3.0f, 35e-2f)) << "state of IO2=inB should be HIGH at timing event 1";
        // expect(approx(sinkC._samples[timingEventSamplesFromTags[1]], 0.0f, 35e-1f)) << "state of IO2=inB should be LOW at timing event 1";
        expect(approx(sinkA._samples[timingEventSamplesFromTags[2] - 2], 0.0f, 35e-2f)) << "trigger channel should be zero ahead of timing event 2";
        expect(approx(sinkA._samples[timingEventSamplesFromTags[2] + 2], 3.0f, 35e-2f)) << "trigger channel should be zero after of timing event 2";
        expect(approx(sinkB._samples[timingEventSamplesFromTags[2]], 0.0f, 35e-2f)) << "state of IO2=inB should be LOW at timing event 2";
        // expect(approx(sinkC._samples[timingEventSamplesFromTags[2]], 3.0f, 35e-2f)) << "state of IO2=inB should be HIGH at timing event 2";
        // std::println("timing events: {}", sinkA._tags | std::views::filter([](auto& t) { return t.map.contains(gr::tag::CONTEXT.shortKey()); }) | std::views::transform([](auto& t) { return std::pair(t.index, t.map); }));
    };

    /**
     * This test adds some special cases to the previous test e.g.
     * - tags on the same timestamp
     * - tags on neighbouring samples
     * - missing tags
     */
    "streamingWithTimingEdgeCases"_test = [&] {
        using namespace std::chrono;
        using namespace std::chrono_literals;
        using namespace boost::ut;

        std::println("testStreamingBasicsWithTimingEdgeCases");

        constexpr float kSampleRate = 100000.f; // [Hz]
        constexpr auto  kDuration   = 2s;

        gr::Graph flowGraph;
        auto&     timingSrc = flowGraph.emplaceBlock<gr::timing::TimingSource>({
            {"event_actions", std::vector<std::string>({
                                  "SIS100_RING->PUBLISH()",                      // monitor all events for the sis100 timing group
                                  "SIS100_RING:CMD_BP_START->IO1(10,on,60,off)", // create a 50us pulse 10us after bp start events
                              })},
            {"event_hw_trigger", std::vector<std::string>({
                                     "SIS100_RING:CMD_BP_START",  // set the hw-trigger tag for all bp start events
                                     "SIS100_RING:CMD_TARGET_ON", // let's claim this tag should also produce an edge, to simulate a missing timing event
                                 })},
            {"sample_rate", 0.0f}, // produce one sample per tag
            {"verbose_console", true},
        });

        auto& ps = flowGraph.emplaceBlock<Picoscope4000a<float>>({{
            {"sample_rate", kSampleRate},
            {"auto_arm", true},
            {"channel_ids", std::vector<std::string>{"A", "B", "C"}},
            {"signal_names", std::vector<std::string>{"Trigger", "IO2", "IO3"}},
            {"signal_units", std::vector<std::string>{"V", "V", "V"}},
            {"channel_ranges", std::vector<float>{5.f, 5.f, 5.f}},
            {"signal_offsets", std::vector<float>{0.f, 0.f, 0.f}},
            {"channel_couplings", std::vector<std::string>{"DC", "DC", "DC"}},
            {"trigger_source", "A"},
            {"trigger_threshold", 1.7f},
            {"trigger_direction", "Rising"},
            {"matcher_timeout", 5'000'000},
        }});

        auto& sinkA = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", true}, {"verbose_console", true}});
        auto& sinkB = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", true}});
        auto& sinkC = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", true}});
        auto& sinkD = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkE = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkF = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkG = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkH = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});

        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out">(timingSrc).template to<"timingIn">(ps)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).template to<"in">(sinkA)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkD)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 4>(ps).template to<"in">(sinkE)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 5>(ps).template to<"in">(sinkF)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 6>(ps).template to<"in">(sinkG)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 7>(ps).template to<"in">(sinkH)));

        auto& sinkDigital = flowGraph.emplaceBlock<testing::TagSink<uint16_t, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"digitalOut">(ps).template to<"in">(sinkDigital)));

        std::this_thread::sleep_for(1s);

        // Explicitly start unit because it takes quite some time
        expect(nothrow([&ps] { ps.start(); }));

        std::jthread publishEvents = createTimingEventThread(
            {
                {790'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 286, 2)}},   // CMD_CUSTOM_DIAG (tag with hw-trigger=false and no previous sample)
                {800'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 256, 3)}},   // CMD_BP_START
                {800'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 286, 4)}},   // CMD_CUSTOM_DIAG (on the exact same timestamp)
                {1'200'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 286, 5)}}, // CMD_CUSTOM_DIAG (tag with hw-trigger=false)
                {1'300'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 256, 6)}}, // CMD_BP_START
                {1'300'000'010, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 526, 7)}}, // CMD_TARGET_ON (claims there should be a hw event, but there isn't. Also on the same sample but with small offset to previous sample)
            },
            100'000'000);
        scheduler::Simple<scheduler::ExecutionPolicy::multiThreaded> sched{std::move(flowGraph)};
        expect(sched.changeStateTo(lifecycle::State::INITIALISED).has_value());
        expect(sched.changeStateTo(lifecycle::State::RUNNING).has_value());
        std::this_thread::sleep_for(kDuration);
        expect(sched.changeStateTo(lifecycle::State::REQUESTED_STOP).has_value());

        const auto measuredRate = static_cast<double>(sinkA._nSamplesProduced) / duration<double>(kDuration).count();
        std::println("Produced in worker: {}", ps._nSamplesPublished);
        std::println("Configured rate: {}, Measured rate: {} ({:.2f}%), Duration: {} ms", kSampleRate, static_cast<std::size_t>(measuredRate), measuredRate / static_cast<double>(kSampleRate) * 100., duration_cast<milliseconds>(kDuration).count());
        std::println("Total: {}", sinkA._nSamplesProduced);
        std::println("Tags: {}", sinkA._tags);

        expect(ge(sinkA._nSamplesProduced, 100000UZ));
        expect(le(sinkA._nSamplesProduced, 220000UZ));
        if (!sinkA._tags.empty()) { // check meta info propagation in first tag
            const auto& tag = sinkA._tags[0];
            expect(eq(tag.index, 0UZ));
            expect(eq(std::get<float>(tag.at(std::string(tag::SAMPLE_RATE.shortKey()))), kSampleRate));
            expect(eq(std::get<std::string>(tag.at(std::string(tag::SIGNAL_NAME.shortKey()))), "Trigger"s));
            expect(eq(std::get<std::string>(tag.at(std::string(tag::SIGNAL_UNIT.shortKey()))), "V"s));
            expect(eq(std::get<float>(tag.at(std::string(tag::SIGNAL_MIN.shortKey()))), -5.f));
            expect(eq(std::get<float>(tag.at(std::string(tag::SIGNAL_MAX.shortKey()))), 5.f));
        }
        auto timingEventSamplesFromTags = sinkA._tags | std::views::filter([](auto& t) { return t.map.contains(gr::tag::CONTEXT.shortKey()); }) // only consider timing events
                                          | std::views::transform([](auto& t) { return t.index; })                                              // get tag index
                                          | std::ranges::to<std::vector>();
        // TODO: actually we expect 6, not 4, but this is due to gnuradio swallowing tags on the same index. This needs to be fixed in gr4 tag handling
        expect(eq(timingEventSamplesFromTags.size(), 4UZ)) << [&]() { return std::format("expected to get exactly 4 timing tags, tags at samples: {}", timingEventSamplesFromTags); } << fatal;
        expect(std::ranges::equal(                                                                                                                                                                           // check that the tags are at the correct time points
            timingEventSamplesFromTags | std::views::transform([&](unsigned long v) -> double { return static_cast<double>(v - timingEventSamplesFromTags[0]) / static_cast<double>(kSampleRate) + 0.79; }), // remove non timing tags
            std::vector<double>{0.79, 0.8, /*0.8,*/ 1.2, 1.3 /*, 1.3*/},                                                                                                                                     //
            [](auto a, auto b) { return std::abs(a - b) < 1e-2 /*1e-9 this tolerance needs to be brought down after the picoscope refactoring */; }                                                          // ignore differences smaller than 1ns
            ))
            << "Timing tags at wrong indices not matching their relative time offsets";
        std::print("timestamps: {}\n", timingEventSamplesFromTags | std::views::transform([&](unsigned long v) -> double { return static_cast<double>(v - timingEventSamplesFromTags[0]) / static_cast<double>(kSampleRate) + 0.79; }));
    };

    "triggeredAcquisitionWithTiming"_test = [&] {
        using namespace std::chrono;
        using namespace std::chrono_literals;
        using namespace boost::ut;

        std::println("triggeredAcquisitionWithTiming");

        constexpr float kSampleRate = 100000.f; // [Hz]
        constexpr auto  kDuration   = 2s;

        gr::Graph flowGraph;
        auto&     timingSrc = flowGraph.emplaceBlock<gr::timing::TimingSource>({
            {"event_actions", std::vector<std::string>({
                                  "SIS100_RING->PUBLISH()",                        // monitor all events for the sis100 timing group
                                  "SIS100_RING:CMD_BP_START->IO1(100,on,150,off)", // create a 50us pulse 100us after bp start events
                              })},
            {"event_hw_trigger", std::vector<std::string>({
                                     "SIS100_RING:CMD_BP_START", // set the hw-trigger tag for all bp start events
                                 })},
            {"sample_rate", 0.0f}, // produce one sample per tag
            {"verbose_console", true},
        });

        auto& ps = flowGraph.emplaceBlock<Picoscope4000a<gr::DataSet<float>>>({{
            {"sample_rate", kSampleRate},
            {"auto_arm", true},
            {"channel_ids", std::vector<std::string>{"A", "B", "C"}},
            {"signal_names", std::vector<std::string>{"Trigger", "IO2", "IO3"}},
            {"signal_units", std::vector<std::string>{"V", "V", "V"}},
            {"channel_ranges", std::vector<float>{5.f, 5.f, 5.f}},
            {"signal_offsets", std::vector<float>{0.f, 0.f, 0.f}},
            {"channel_couplings", std::vector<std::string>{"DC", "DC", "DC"}},
            {"trigger_source", "A"},
            {"trigger_threshold", 1.7f},
            {"trigger_direction", "Rising"},
            {"pre_samples", 100},
            {"post_samples", 30'000},
            {"n_captures", 1},
            {"auto_arm", true},
            // {"digital_port_threshold", digitalPortThreshold},
            // {"digital_port_invert_output", digitalPortInvertOutput},
            {"trigger_once", false},
            {"matcher_timeout", 20'000'000},
        }});

        auto& sinkA = flowGraph.emplaceBlock<testing::TagSink<gr::DataSet<float>, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", true}, {"verbose_console", true}});
        auto& sinkB = flowGraph.emplaceBlock<testing::TagSink<gr::DataSet<float>, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", true}});
        auto& sinkC = flowGraph.emplaceBlock<testing::TagSink<gr::DataSet<float>, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", true}});
        auto& sinkD = flowGraph.emplaceBlock<testing::TagSink<gr::DataSet<float>, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkE = flowGraph.emplaceBlock<testing::TagSink<gr::DataSet<float>, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkF = flowGraph.emplaceBlock<testing::TagSink<gr::DataSet<float>, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkG = flowGraph.emplaceBlock<testing::TagSink<gr::DataSet<float>, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkH = flowGraph.emplaceBlock<testing::TagSink<gr::DataSet<float>, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});

        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out">(timingSrc).template to<"timingIn">(ps)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).template to<"in">(sinkA)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkD)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 4>(ps).template to<"in">(sinkE)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 5>(ps).template to<"in">(sinkF)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 6>(ps).template to<"in">(sinkG)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 7>(ps).template to<"in">(sinkH)));

        auto& sinkDigital = flowGraph.emplaceBlock<testing::TagSink<gr::DataSet<uint16_t>, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"digitalOut">(ps).template to<"in">(sinkDigital)));

        std::this_thread::sleep_for(1s);

        // Explicitly start unit because it takes quite some time
        expect(nothrow([&ps] { ps.start(); }));

        std::jthread publishEvents = createTimingEventThread(
            {
                {780'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 286, 2)}}, // CMD_CUSTOM_DIAG (tag with hw-trigger=false and no previous sample)
                // 790'000'000 acq 1 starts
                {800'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 256, 3)}}, // CMD_BP_START
                {800'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 286, 4)}}, // CMD_CUSTOM_DIAG (on the exact same timestamp)
                // 1'000'000'000 acq 1 ends
                // 1'299'000'000 acq 2 starts
                {1'299'500'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 286, 5)}}, // CMD_CUSTOM_DIAG (tag with hw-trigger=false)
                {1'300'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 256, 6)}}, // CMD_BP_START
                {1'300'000'010, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 526, 7)}}, // CMD_TARGET_ON (claims there should be a hw event, but there isn't. Also on the same sample but with small offset to previous sample)
                {1'400'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 286, 8)}}, // CMD_CUSTOM_DIAG (tag with hw-trigger=false)
                // 1'600'000'000 acq 2 ends
                {1'700'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 286, 9)}}, // CMD_CUSTOM_DIAG (tag with hw-trigger=false)
            },
            100'000'000);
        scheduler::Simple<scheduler::ExecutionPolicy::multiThreaded> sched{std::move(flowGraph)};
        expect(sched.changeStateTo(lifecycle::State::INITIALISED).has_value());
        expect(sched.changeStateTo(lifecycle::State::RUNNING).has_value());
        std::this_thread::sleep_for(kDuration);
        expect(sched.changeStateTo(lifecycle::State::REQUESTED_STOP).has_value());

        expect(eq(sinkA._nSamplesProduced, 2uz));
        gr::DataSet dataA = sinkA._samples[0];
        expect(eq(dataA.size(), 1uz));
        expect(eq(dataA.signalValues(0uz).size(), 30100uz));
        expect(eq(4UZ, dataA.timingEvents(0uz).size())); // 2 timing events + 2 hardware edges
        std::println("tags: {}", dataA.timingEvents(0uz));
        gr::DataSet dataA2 = sinkA._samples[1];
        expect(eq(6UZ, dataA2.timingEvents(0uz).size())); // 4 timing events + 2 hardware edges
        std::println("tags2: {}", dataA2.timingEvents(0uz));
    };
};

} // namespace fair::picoscope::test

int main() { /* tests are statically executed */ }
