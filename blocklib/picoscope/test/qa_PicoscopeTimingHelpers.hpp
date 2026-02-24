#ifndef QA_PICOSCOPETIMINGHELPERS_HPP
#define QA_PICOSCOPETIMINGHELPERS_HPP
#include <boost/ut.hpp>
#include <gnuradio-4.0/Tag.hpp>

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

#include <fair/picoscope/Picoscope.hpp>
#include <fair/timing/TimingSource.hpp>
#include <gnuradio-4.0/Scheduler.hpp>

#include <gnuradio-4.0/testing/TagMonitors.hpp>

namespace fair::picoscope::test {

inline auto createTimingEventThread(const std::vector<std::pair<std::uint64_t, std::variant<Timing::Event, std::uint8_t>>>& events, std::size_t schedule_offset) {
    using namespace std::chrono_literals;
    return std::jthread([events, schedule_offset]() {
        std::println("start replay of test timing data");
        Timing             timing;
        static std::size_t instance = 0;
        timing.saftAppName          = std::format("qa_picoscope_timing_dispatcher_{}_{}", getpid(), instance++);
        timing.initialize();
        const auto    outputs     = timing.receiver->getOutputs() | std::views::transform([&timing](auto kvp) {
            auto [outputName, path] = kvp;
            auto outputProxy        = saftlib::Output_Proxy::create(path, timing.saftSigGroup);
            outputProxy->setOutputEnable(true);
            outputProxy->WriteOutput(false);
            return outputProxy;
        }) | std::ranges::to<std::vector>();
        std::uint64_t outputState = 0x0ull;
        const auto    start       = timing.currentTimeTAI() + schedule_offset;
        std::this_thread::sleep_for(200ms);
        for (auto& [schedule_dt, action] : events) {
            if (std::holds_alternative<std::uint8_t>(action)) { // toggle outputs
                while (start + schedule_dt > timing.currentTimeTAI()) {
                    timing.saftSigGroup.wait_for_signal(0);
                }
                const auto newState = std::get<std::uint8_t>(action);
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
            }
        }
        timing.stop();
        std::println("finished replay of timing data");
    });
};

template<PicoscopeImplementationLike TPSImpl>
void testStreamingWithTiming(const float kSampleRate = 1000.f, const std::chrono::seconds kDuration = 7s, const std::string triggerName = "D") {
    using namespace std::chrono;
    using namespace std::chrono_literals;
    using namespace boost::ut;
    using namespace fair::picoscope;
    using namespace fair::picoscope::test;
    using namespace gr;

    std::println("\n================== testStreamingBasicsWithTiming: {} =================\n    sampleRate={}, trigger={}\n", gr::meta::type_name<TPSImpl>(), kSampleRate, triggerName);

    const std::size_t pulseOnTime  = 100UZ;                                                            // [us]
    const std::size_t pulseOffTime = pulseOnTime + 3UZ * static_cast<std::size_t>(1e6f / kSampleRate); // use a pulse that is the length of 3 samples

    gr::Graph flowGraph;
    auto&     timingSrc = flowGraph.emplaceBlock<gr::timing::TimingSource>({
        {"event_actions", std::vector<std::string>({std::format("SIS100_RING:CMD_CUSTOM_DIAG_1->IO3({},on,{},off),PUBLISH()", pulseOnTime, pulseOffTime)})},
        {"io_events", true},
        {"sample_rate", 0.0f},
        {"verbose_console", true},
    });

    const std::size_t matcher_timeout     = 30'000'000UZ; // * 10'000 / static_cast<std::size_t>(kSampleRate);
    const bool        digital_port_enable = TPSImpl::N_DIGITAL_CHANNELS > 0;

    auto& ps = flowGraph.emplaceBlock<Picoscope<float, TPSImpl>>({{
        {"sample_rate", kSampleRate},
        {"auto_arm", true},
        {"channel_ids", std::vector<std::string>{"A", "B", TPSImpl::N_ANALOG_CHANNELS > 4 ? "H" : "D"}},
        {"signal_names", std::vector<std::string>{"IO1", "IO2", "Trigger"}},
        {"signal_units", std::vector<std::string>{"V", "V", "V"}},
        {"channel_ranges", std::vector<float>{5.f, 5.f, 5.f}},
        {"signal_offsets", std::vector<float>{0.f, 0.f, 0.f}},
        {"channel_couplings", std::vector<std::string>{"DC", "DC", "DC"}},
        {"trigger_source", triggerName},
        {"trigger_threshold", 1.7f},
        {"trigger_direction", "Rising"},
        {"matcher_timeout", matcher_timeout},
        {"digital_port_enable", digital_port_enable},
        {"verbose_console", true},
    }});

    auto& sinkA = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", true}});
    auto& sinkB = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", false}});
    auto& sinkC = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", false}});
    auto& sinkD = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", false}});

    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out">(timingSrc).template to<"timingIn">(ps)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).template to<"in">(sinkA)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkD)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkC)));
    if constexpr (TPSImpl::N_ANALOG_CHANNELS > 4) {
        auto& sinkE = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkF = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkG = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkH = flowGraph.emplaceBlock<testing::TagSink<float, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 4>(ps).template to<"in">(sinkE)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 5>(ps).template to<"in">(sinkF)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 6>(ps).template to<"in">(sinkG)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 7>(ps).template to<"in">(sinkH)));
    }

    auto& sinkDigital = flowGraph.emplaceBlock<testing::TagSink<uint16_t, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", false}});
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"digitalOut">(ps).template to<"in">(sinkDigital)));

    std::jthread                                                 publishEvents = createTimingEventThread({
                                                             //   A    B   D/DI4            t                event
                                                             /*  │    │    │   */ {500'000'000, std::uint8_t{0b000}},
                                                             /*  │    │    ├── */ {800'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 286, 1)}},
                                                             /*  │    └─┐  │   */ {1'000'000'000, std::uint8_t{0b010}},
                                                             /*  │      │  ├── */ {1'400'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 286, 2)}},
                                                             /*  └─┐  ┌─┘  │   */ {1'500'000'000, std::uint8_t{0b001}},
                                                             /*    │  │    ├── */ {1'700'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 286, 3)}},
                                                             /*    │  └─┐  │   */ {2'000'000'000, std::uint8_t{0b011}},
                                                             /*  ┌─┘  ┌─┘  │   */ {2'500'000'000, std::uint8_t{0b000}},
                                                         },
        4'000'000'000);
    scheduler::Simple<scheduler::ExecutionPolicy::multiThreaded> sched{};
    std::ignore = sched.exchange(std::move(flowGraph));
    expect(sched.changeStateTo(lifecycle::State::INITIALISED).has_value());
    expect(sched.changeStateTo(lifecycle::State::RUNNING).has_value());
    gr::MsgPortOut _toScheduler;
    expect(_toScheduler.connect(sched.msgIn) == gr::ConnectionResult::SUCCESS) << fatal;
    std::this_thread::sleep_for(kDuration);
    gr::sendMessage<gr::message::Command::Set>(_toScheduler, sched.unique_name, gr::block::property::kLifeCycleState, {{"state", std::string(magic_enum::enum_name(gr::lifecycle::State::REQUESTED_STOP))}}, "test");
    std::this_thread::sleep_for(10ms); // wait for the scheduler to actually stop processing -> otherwise process work will be called when the scheduler and graph have already been destroyed

    const auto measuredRate = static_cast<double>(sinkA._nSamplesProduced) / duration<double>(kDuration).count();
    std::println("Produced in worker: {}", ps._nSamplesPublished);
    std::println("Configured rate: {}, Measured rate: {} ({:.2f}%), Duration: {} ms", kSampleRate, static_cast<std::size_t>(measuredRate), measuredRate / static_cast<double>(kSampleRate) * 100., duration_cast<milliseconds>(kDuration).count());
    std::println("Total: {}", sinkA._nSamplesProduced);
    // verify that all ports got the same number of samples
    expect(eq(sinkA._nSamplesProduced, sinkB._nSamplesProduced));
    expect(eq(sinkA._nSamplesProduced, sinkC._nSamplesProduced));
    expect(eq(sinkA._nSamplesProduced, sinkD._nSamplesProduced));
    expect(eq(sinkA._nSamplesProduced, sinkDigital._nSamplesProduced));
    for (const auto& [index, map] : sinkA._tags) {
        std::println("  - {}: {}", index, map);
    }

    expect(ge(sinkA._nSamplesProduced, std::chrono::duration_cast<nanoseconds>(kDuration - 4s).count() * static_cast<long>(kSampleRate * 1e-9f))); // assume that the picoscope takes at most 4s to start streaming data
    if (!sinkA._tags.empty()) {
        const auto& tag = sinkA._tags[0];
        expect(eq(tag.index, 0UZ));
        expect(eq(tag.at(std::string(tag::SAMPLE_RATE.shortKey())).value_or(INFINITY), kSampleRate));
        expect(eq(tag.at(std::string(tag::SIGNAL_NAME.shortKey())).value_or(std::string{}), "IO1"s));
        expect(eq(tag.at(std::string(tag::SIGNAL_UNIT.shortKey())).value_or(std::string{}), "V"s));
        expect(eq(tag.at(std::string(tag::SIGNAL_MIN.shortKey())).value_or(INFINITY), -5.f));
        expect(eq(tag.at(std::string(tag::SIGNAL_MAX.shortKey())).value_or(INFINITY), 5.f));
    }
    expect(std::ranges::equal(sinkA._tags | std::views::filter([](auto& t) { return t.map.contains(gr::tag::CONTEXT.shortKey()); })                                                                                       // only consider timing events
                                  | std::views::transform([](auto& t) { return t.map[gr::tag::TRIGGER_META_INFO.shortKey()].value_or(gr::property_map{})["BPID"].value_or(std::numeric_limits<std::uint16_t>::max()); }), // get bpid (which is unique in this test)
        std::vector<std::uint16_t>{1, 2, 3}))
        << "expected to get timing events with bpid 1, 2 and 3";

    auto chunks = sinkA._tags | std::views::filter([](auto& t) { return t.map.contains("chunk-start-time"); })                                                                                                                                                                                                          //
                  | std::views::transform([kSampleRate](const auto& t) { return std::tuple(t.index, static_cast<float>(t.index) * 1e9f / kSampleRate, t.map.at("chunk-start-time").value_or(std::numeric_limits<long>::max())); })                                                                                      //
                  | std::views::pairwise_transform([](const auto& a, const auto& b) { return std::tuple(std::get<0>(b), std::get<0>(b) - std::get<0>(a), std::get<1>(b) - std::get<1>(a), std::get<2>(b) - std::get<2>(a), static_cast<float>(std::get<2>(b) - std::get<2>(a)) - (std::get<1>(b) - std::get<1>(a))); }) // compute chunk durations [index, indexdiff, delta t samples ns, delta t acq ns]
                  | std::ranges::to<std::vector>();
    std::println("Chunks:");
    for (const auto& chunk : chunks) {
        std::println("  - {}: {} samples, {} ns based on sample rate, {} ns between updates", std::get<0>(chunk), std::get<1>(chunk), std::get<2>(chunk), std::get<3>(chunk));
    }

    auto timingEventSamplesFromTags = sinkA._tags | std::views::filter([](auto& t) { return t.map.contains(gr::tag::CONTEXT.shortKey()); }) // only consider timing events
                                      | std::views::transform([](auto& t) { return t.index; })                                              // get tag index
                                      | std::ranges::to<std::vector>();
    const auto detectedEdges        = std::views::zip(std::views::iota(0U), sinkD._samples) | std::views::filter([](const auto& p) { return std::get<1>(p) > 1.7f; }) | std::views::transform([](const auto& p) { return std::get<0>(p); }) | std::ranges::to<std::vector>();
    const auto detectedEdgesDigital = std::views::zip(std::views::iota(0U), sinkDigital._samples) | std::views::filter([](const auto& p) { return (std::get<1>(p) & (1u << 4)); }) | std::views::transform([](const auto& p) { return std::get<0>(p); }) | std::ranges::to<std::vector>();
    std::println("Trigger channel: detected triggers: {}, detected digital triggers: {}, timing idxes: {}", detectedEdges, detectedEdgesDigital, timingEventSamplesFromTags);
    expect(eq(timingEventSamplesFromTags.size(), 3UZ)) << "expected to get exactly 3 timing tags" << fatal;
    expect(approx(static_cast<double>(timingEventSamplesFromTags[1] - timingEventSamplesFromTags[0]), (1'400'000'000 - 800'000'000) * 1e-9 * static_cast<double>(kSampleRate), 30.0)) << "sample distance between first and second tag does not match sample rate";
    expect(approx(static_cast<double>(timingEventSamplesFromTags[2] - timingEventSamplesFromTags[0]), (1'700'000'000 - 800'000'000) * 1e-9 * static_cast<double>(kSampleRate), 30.0)) << "sample distance between first and third tag does not match sample rate";
    expect(approx(sinkD._samples[timingEventSamplesFromTags[0] - 1], 0.0f, 1e-1f)) << "trigger channel should be LOW ahead of timing event 0";
    expect(ge(sinkD._samples[timingEventSamplesFromTags[0]], 1.7f)) << "trigger channel should be HIGH after of timing event 0";
    expect(approx(sinkA._samples[timingEventSamplesFromTags[0]], 0.0f, 35e-2f)) << "state of IO1=inA should be LOW at timing event 0";
    expect(approx(sinkB._samples[timingEventSamplesFromTags[0]], 0.0f, 35e-2f)) << "state of IO2=inB should be LOW at timing event 0";
    expect(approx(sinkD._samples[timingEventSamplesFromTags[1] - 1], 0.0f, 35e-2f)) << "trigger channel should be LOW ahead of timing event 1";
    expect(ge(sinkD._samples[timingEventSamplesFromTags[1]], 1.7f)) << "trigger channel should be HIGH after of timing event 1";
    expect(approx(sinkA._samples[timingEventSamplesFromTags[1]], 0.0f, 35e-1f)) << "state of IO1=inA should be LOW at timing event 1";
    expect(approx(sinkB._samples[timingEventSamplesFromTags[1]], 3.0f, 35e-2f)) << "state of IO2=inB should be HIGH at timing event 1";
    expect(approx(sinkD._samples[timingEventSamplesFromTags[2] - 1], 0.0f, 35e-2f)) << "trigger channel should be LOW ahead of timing event 2";
    expect(ge(sinkD._samples[timingEventSamplesFromTags[2]], 1.7f)) << "trigger channel should be HIGH after of timing event 2";
    expect(approx(sinkA._samples[timingEventSamplesFromTags[2]], 3.0f, 35e-2f)) << "state of IO1=inA should be HIGH at timing event 2";
    expect(approx(sinkB._samples[timingEventSamplesFromTags[2]], 0.0f, 35e-2f)) << "state of IO2=inB should be LOW at timing event 2";
};

template<PicoscopeImplementationLike TPSImpl>
void testTriggeredAcquisitionWithTiming(const float kSampleRate = 1e5f, const std::chrono::seconds kDuration = 7s, const std::string triggerName = "D", std::chrono::nanoseconds acquisitionWindow = 100ms) {
    using namespace std::chrono;
    using namespace std::chrono_literals;
    using namespace boost::ut;
    using namespace fair::picoscope;
    using namespace fair::picoscope::test;
    using namespace gr;

    std::println("triggeredAcquisitionWithTiming");

    const std::size_t pulseOnTime  = 100UZ;                                                                           // [us]
    const std::size_t pulseOffTime = pulseOnTime + std::max(3UZ * static_cast<std::size_t>(1e6f / kSampleRate), 1UZ); // use a pulse that is the length of 3 samples, but at least 1 ns

    gr::Graph flowGraph;
    auto&     timingSrc = flowGraph.emplaceBlock<gr::timing::TimingSource>({
        {"event_actions", std::vector<std::string>({
                              "SIS100_RING->PUBLISH()", // monitor all events for the sis100 timing group
                              std::format("SIS100_RING:CMD_BP_START->IO3({},on,{},off)", pulseOnTime, pulseOffTime),
                          })},
        {"io_events", true},
        {"sample_rate", 0.0f}, // produce one sample per tag
        {"verbose_console", true},
    });

    const std::size_t samplesPre  = 100;
    const std::size_t samplesPost = static_cast<std::size_t>(static_cast<float>(acquisitionWindow.count()) * 1e-9f * kSampleRate);

    auto& ps = flowGraph.emplaceBlock<Picoscope<gr::DataSet<float>, TPSImpl>>({{
        {"sample_rate", kSampleRate},
        {"auto_arm", true},
        {"channel_ids", std::vector<std::string>{"A", "B", TPSImpl::N_ANALOG_CHANNELS > 4 ? "H" : "D"}},
        {"signal_names", std::vector<std::string>{"IO1", "IO2", "Trigger"}},
        {"signal_units", std::vector<std::string>{"V", "V", "V"}},
        {"channel_ranges", std::vector<float>{5.f, 5.f, 5.f}},
        {"signal_offsets", std::vector<float>{0.f, 0.f, 0.f}},
        {"channel_couplings", std::vector<std::string>{"DC", "DC", "DC"}},
        {"trigger_source", triggerName},
        {"trigger_threshold", 1.7f},
        {"trigger_direction", "Rising"},
        {"pre_samples", samplesPre},
        {"post_samples", samplesPost},
        {"n_captures", 1},
        {"auto_arm", true},
        // {"digital_port_invert_output", digitalPortInvertOutput},
        {"trigger_once", false},
        {"matcher_timeout", 20'000'000}, // for triggered acquisition, there is no matcher state, so it doesn't make sense to have a timeout for more tags or samples to arrive
        {"verbose_console", true},
    }});

    auto& sinkA = flowGraph.emplaceBlock<testing::TagSink<gr::DataSet<float>, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", true}, {"verbose_console", true}});
    auto& sinkB = flowGraph.emplaceBlock<testing::TagSink<gr::DataSet<float>, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", true}});
    auto& sinkC = flowGraph.emplaceBlock<testing::TagSink<gr::DataSet<float>, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", true}, {"log_tags", true}});
    auto& sinkD = flowGraph.emplaceBlock<testing::TagSink<gr::DataSet<float>, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});

    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out">(timingSrc).template to<"timingIn">(ps)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 0>(ps).template to<"in">(sinkA)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 1>(ps).template to<"in">(sinkB)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 2>(ps).template to<"in">(sinkC)));
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 3>(ps).template to<"in">(sinkD)));
    if constexpr (TPSImpl::N_ANALOG_CHANNELS > 4) {
        auto& sinkE = flowGraph.emplaceBlock<testing::TagSink<gr::DataSet<float>, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkF = flowGraph.emplaceBlock<testing::TagSink<gr::DataSet<float>, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkG = flowGraph.emplaceBlock<testing::TagSink<gr::DataSet<float>, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        auto& sinkH = flowGraph.emplaceBlock<testing::TagSink<gr::DataSet<float>, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 4>(ps).template to<"in">(sinkE)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 5>(ps).template to<"in">(sinkF)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 6>(ps).template to<"in">(sinkG)));
        expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"out", 7>(ps).template to<"in">(sinkH)));
    }

    auto& sinkDigital = flowGraph.emplaceBlock<testing::TagSink<gr::DataSet<uint16_t>, testing::ProcessFunction::USE_PROCESS_BULK>>({{"log_samples", false}, {"log_tags", false}});
    expect(eq(ConnectionResult::SUCCESS, flowGraph.connect<"digitalOut">(ps).template to<"in">(sinkDigital)));

    std::this_thread::sleep_for(1s);

    const double factor = static_cast<double>(acquisitionWindow.count()) / 0.1e9;
    // TODO: improve dead-time between consecutive acquisitions
    std::jthread publishEvents = createTimingEventThread(
        {
            {factor * 780'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 286, 2)}}, // CMD_CUSTOM_DIAG (tag with hw-trigger=false and no previous sample)
            // 790'000'000 acq 1 starts
            {factor * 800'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 256, 3)}}, // CMD_BP_START
            {factor * 800'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 286, 4)}}, // CMD_CUSTOM_DIAG (on the exact same timestamp)
            // 900'000'000 acq 1 ends
            // 2'899'000'000 acq 2 starts
            {factor * 2'899'500'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 286, 5)}}, // CMD_CUSTOM_DIAG (tag with hw-trigger=false)
            {factor * 2'930'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 256, 6)}}, // CMD_BP_START
            {factor * 2'930'000'010, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 526, 7)}}, // CMD_TARGET_ON (claims there should be a hw event, but there isn't. Also on the same sample but with small offset to previous sample)
            {factor * 2'960'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 286, 8)}}, // CMD_CUSTOM_DIAG (tag with hw-trigger=false)
            // 3'000'000'000 acq 2 ends
            {factor * 3'100'000'000, {Timing::Event(Timing::Event::fromGidEventnoBpidTag{}, 310, 286, 9)}}, // CMD_CUSTOM_DIAG (tag with hw-trigger=false)
        },
        3'500'000'000);
    scheduler::Simple<scheduler::ExecutionPolicy::multiThreaded> sched{};
    std::ignore = sched.exchange(std::move(flowGraph));
    expect(sched.changeStateTo(lifecycle::State::INITIALISED).has_value());
    expect(sched.changeStateTo(lifecycle::State::RUNNING).has_value());
    gr::MsgPortOut _toScheduler;
    expect(_toScheduler.connect(sched.msgIn) == gr::ConnectionResult::SUCCESS) << fatal;
    std::this_thread::sleep_for(kDuration);
    std::print("stopping scheduler");
    gr::sendMessage<gr::message::Command::Set>(_toScheduler, sched.unique_name, gr::block::property::kLifeCycleState, {{"state", std::string(magic_enum::enum_name(gr::lifecycle::State::REQUESTED_STOP))}}, "test");
    std::this_thread::sleep_for(10ms); // wait for the scheduler to actually stop processing -> otherwise process work will be called when the scheduler and graph have already been destroyed
    std::println(" and assume it is finished after 10ms");

    expect(eq(sinkA._nSamplesProduced, 2UZ));
    if (sinkA._nSamplesProduced > 0) {
        gr::DataSet dataA = sinkA._samples[0];
        expect(eq(dataA.size(), 1UZ));
        expect(eq(dataA.signalValues(0).size(), samplesPre + samplesPost));
        expect(eq(4UZ, dataA.timingEvents(0UZ).size())); // 2 timing events + 2 hardware edges
        std::println("tags: {}", dataA.timingEvents(0UZ));
    }
    if (sinkA._nSamplesProduced > 1) {
        gr::DataSet dataA2 = sinkA._samples[1];
        expect(eq(dataA2.size(), 1UZ));
        expect(eq(dataA2.signalValues(0).size(), samplesPre + samplesPost));
        // expect(eq(6UZ, dataA2.timingEvents(0UZ).size())); // 4 timing events + 2 hardware edges
        expect(ge(5UZ, dataA2.timingEvents(0UZ).size())); // 4 timing events + 2 hardware edges // TODO: tag ahead of trigger should also be matched
        std::println("tags2: {}", dataA2.timingEvents(0UZ));
    }
}
} // namespace fair::picoscope::test

#endif // QA_PICOSCOPETIMINGHELPERS_HPP
