#include <boost/ut.hpp>
#include <fair/timing/TimingSource.hpp>
#include <format>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/basic/ConverterBlocks.hpp>
#include <gnuradio-4.0/testing/TagMonitors.hpp>
#include <latch>

using namespace std::string_literals;
using namespace std::chrono_literals;
using namespace boost::ut;

namespace fair::timing::test {

const suite TimingBlockHelpers = [] {
    "filter-parsing"_test = []() {
        auto expectTiming = [](auto a, auto b) {
            auto aa = gr::timing::TimingSource::parseFilter(a);
            expect(aa == b) << std::format("checking: {}\ngot: {:#x}, {:#x}\nexp: {:#x}, {:#x}\n", a, aa.first, aa.second, b.first, b.second);
        };

        expectTiming("", std::pair{0x1000000000000000ul, 0xf000000000000000ul});
        expectTiming("SIS100_RING:CMD_BP_START", std::pair{0x1136100000000000ul, 0xfffffff000000000ul});
        expectTiming("SIS100_RING:CMD_BP_START:BEAM-IN=1", std::pair{0x1136100800000000ul, 0xfffffff800000000ul});
        expectTiming("SIS100_RING:CMD_BP_START:BEAM-IN=1:BPC-START=0", std::pair{0x1136100800000000ul, 0xfffffffc00000000ul});
        expectTiming("SIS100_RING:CMD_BP_START:BEAM-IN=1:BPC-START=0:0", std::pair{0x1136100800000000ul, 0xfffffffffff00000ul});
        expectTiming("SIS100_RING:CMD_BP_START:BEAM-IN=1:BPC-START=0:0:2", std::pair{0x1136100800000080ul, 0xffffffffffffffc0ul});
        expectTiming("301:256", std::pair{0x112d100000000000ul, 0xfffffff000000000ul});
        expect(throws([]() { gr::timing::TimingSource::parseFilter("INVALID_TIMING_GROUP"); })) << "Unknown TimingGroup should throw";
        expect(throws([]() { gr::timing::TimingSource::parseFilter("SIS100_RING:UNKNOWN_EVENT"); })) << "Unknown Event should throw";
        expect(throws([]() { gr::timing::TimingSource::parseFilter("SIS100_RING:CMD_CUSTOM_DIAG1:5"); })) << "Illegal Flags should throw";
        expect(throws([]() { gr::timing::TimingSource::parseFilter("SIS100_RING:CMD_CUSTOM_DIAG1:5"); })) << "Illegal Flags should throw";
        expect(throws([]() { gr::timing::TimingSource::parseFilter("SIS100_RING:CMD_CUSTOM_DIAG1:BEAM-IN=1:BPC-START=4"); })) << "Illegal Flags should throw";
        expect(throws([]() { gr::timing::TimingSource::parseFilter("SIS100_RING:CMD_CUSTOM_DIAG1:BEAM-IN=0:BPC-START=1:x"); })) << "Illegal sequence id should throw";
        expect(throws([]() { gr::timing::TimingSource::parseFilter("SIS100_RING:CMD_CUSTOM_DIAG1:BEAM-IN=0:BPC-START=1:5:test"); })) << "Illegal peam process id should throw";
        expect(throws([]() { gr::timing::TimingSource::parseFilter("SIS100_RING::BEAM-IN=0:BPC-START=1:5:7"); })) << "Zero length filter token should throw";
    };

    "trigger-action-parsing"_test = []() {
        auto expectTrigger = [](const auto a, const auto& b) {
            const auto [trigger, actions] = gr::timing::TimingSource::splitTriggerActions(a);
            const auto aa                 = gr::timing::TimingSource::parseTriggerAction(actions);
            expect(std::ranges::equal(aa, b)) << std::format("checking: {}\ngot: {}\nexp: {}\n", a, aa, b);
        };

        expectTrigger("SIS100_RING->DIO1(20,on,5,off),DIO5(5,off)",                                      //
            std::vector{std::pair("DIO1"s, std::vector{std::pair(20ul, "on"s), std::pair(5ul, "off"s)}), //
                std::pair("DIO5"s, std::vector{std::pair(5ul, "off"s)})});
        expectTrigger("SIS100_RING:CMD_BP_START:BEAM-IN=1:BPC-START=0:0:2->IO1(20000,on,30000,off),IO2(5000,on,6000,off)", //
            std::vector{std::pair("IO1"s, std::vector{std::pair(20000ul, "on"s), std::pair(30000ul, "off"s)}),             //
                std::pair("IO2"s, std::vector{std::pair(5000ul, "on"s), std::pair{6000ul, "off"s}})});
        expectTrigger("301:256->IO3(10,on,11,off)", std::vector{std::pair("IO3"s, std::vector{std::pair(10ul, "on"s), std::pair(11ul, "off"s)})});
        expectTrigger("301:256->PUBLISH()", std::vector{std::pair("PUBLISH"s, std::vector<std::pair<std::uint64_t, std::string>>())});
        expect(throws([]() { gr::timing::TimingSource::splitTriggerActions("RING:EVENT--<PUBLISH()"); })) << "should throw if there is no arrow separator";
        expect(throws([]() { gr::timing::TimingSource::parseTriggerAction("PUBLISH"); })) << "missing parentheses should throw";
        expect(throws([]() { gr::timing::TimingSource::parseTriggerAction("IO(notADelay,on)"); })) << "if delay is not a parsable number parsing should throw";
        expect(throws([]() { gr::timing::TimingSource::parseTriggerAction("IO(50,on,40,off),"); })) << "Additional comma without additional action should throw";
        expect(throws([]() { gr::timing::TimingSource::parseTriggerAction("IO(50,on,40,off"); })) << "Missing closing bracket should throw";
    };

    "add metadata to tag"_test = []() {
        std::vector<std::tuple<std::uint64_t, std::uint64_t>> actionTrigger{
            {0x1136100000000000ul, 0xfffffff000000000ul},
            {0x112d100000000000ul, 0xfffffff000000000ul},
        };
        { // id does not match any filters -> HW-TRIGGER: false
            gr::Tag tag{.index = 0uz, .map = {{"existingKey", "test"}}};
            gr::timing::TimingSource::addHwTriggerInfo(0x1136200000000000ul, tag, actionTrigger);
            gr::property_map expected{{"existingKey", "test"}, {gr::tag::TRIGGER_META_INFO.shortKey(), gr::property_map{{"HW-TRIGGER", false}}}};
            expect(std::ranges::equal(expected, tag.map)) << [&tag, &expected]() { return std::format("got: {} exp: {}", tag.map, expected); };
        }
        { // id matches first filter in the filter list -> HW-TRIGGER: true
            gr::Tag tag{.index = 0uz, .map = {{"existingKey", "test"}}};
            gr::timing::TimingSource::addHwTriggerInfo(0x1136100000000000ul, tag, actionTrigger);
            gr::property_map expected{{"existingKey", "test"}, {gr::tag::TRIGGER_META_INFO.shortKey(), gr::property_map{{"HW-TRIGGER", true}}}};
            expect(std::ranges::equal(expected, tag.map)) << [&tag, &expected]() { return std::format("got: {} exp: {}", tag.map, expected); };
        }
    };
};

std::thread dispatchSimulatedTiming(std::size_t n = 30, std::size_t schedule_dt = 200'000'000UL, std::size_t schedule_offset = 500'000'000UL) {
    return std::thread([=]() {
        static std::size_t uniqueId;
        std::print("start replay of test timing data\n");
        Timing timing;
        timing.saftAppName = std::format("qa_timingSource_dispatcher_{}_{}", getpid(), uniqueId++);
        timing.initialize();
        auto          outputs     = timing.receiver->getOutputs() | std::views::transform([&timing](auto kvp) {
            auto [name, path] = kvp;
            auto result       = saftlib::Output_Proxy::create(path, timing.saftSigGroup);
            result->setOutputEnable(true);
            result->WriteOutput(false);
            return result;
        }) | std::ranges::to<std::vector>();
        std::uint64_t outputState = 0x0ull;
        std::uint8_t  i           = 0;
        auto          start       = timing.currentTimeTAI() + schedule_offset;
        std::this_thread::sleep_for(50ms);
        while (i < n) {
            while (start + i * schedule_dt < timing.currentTimeTAI() + schedule_offset) {
                // publish event
                Timing::Event ev;
                ev.gid        = (i % 2 == 0) ? 310 : 301;
                ev.eventNo    = (i % 3 == 0) ? 256 : 16;
                ev.bpid       = i;
                ev.flagBeamin = true;
                ev.time       = i * schedule_dt;
                timing.injectEvent(ev, start);
                // change output level for all IOs except IO1 which will be triggered by events
                for (std::size_t j = 0; j < outputs.size() - 1; j++) {
                    if ((outputState & (0x1ull << (j + 1))) != ((outputState + 1) & (0x1ull << (j + 1)))) {
                        const bool state = ((outputState + 1) & (0x1ull << (j + 1))) > 0;
                        outputs[j + 1]->WriteOutput(state);
                        // std::print("iteration {}, setting output {}({}) to {}\n", i, j + 1, outputs[j + 1]->getInput(), state);
                    }
                }
                outputState = (outputState + 1) & ((0x1ull << (outputs.size() - 1)) - 1);

                i++;
                if (i >= n) {
                    break;
                }
            }
            timing.saftSigGroup.wait_for_signal(1);
        }
        std::print("finished replay of timing data\n");
    });
}

const suite TimingBlock = [] {
    // boost::ext::ut::cfg<override> = {.tag = {"timing-hardware"}};

    constexpr static auto getBPID = [](gr::Tag& tag) -> std::optional<uint16_t> {
        auto BPIDValue = tag.map[gr::tag::TRIGGER_META_INFO.shortKey()].value_or(gr::property_map{})["BPID"];
        assert(BPIDValue.template holds<uint16_t>());
        return BPIDValue.value_or(uint16_t{});
    };

    constexpr static auto verboseUpdateMessages = [](auto& sink) {
        std::print("received{} timing tags and {} samples!\noutput: {}\n", sink._tags.size(), sink._samples.size(), sink._samples);
        if (!sink._tags.empty()) {
            const auto firstTimestampValue = sink._tags[0].map[gr::tag::TRIGGER_TIME.shortKey()];
            if (!firstTimestampValue.template holds<uint64_t>()) {
                std::println("WARNING: bad type ({}, {}) for timestamp at index 0, expected {}", firstTimestampValue.value_type(), firstTimestampValue.container_type(), gr::meta::type_name<std::uint64_t>());
            }
            const auto firstTimestamp = firstTimestampValue.value_or(std::uint64_t{});
            for (auto& tag : sink._tags) {
                const auto triggerTimeValue = tag.map[gr::tag::TRIGGER_TIME.shortKey()];
                if (!triggerTimeValue.template holds<std::uint64_t>()) {
                    std::println("WARNING: bad type ({}, {}) for timestamp, expected {}", firstTimestampValue.value_type(), firstTimestampValue.container_type(), gr::meta::type_name<std::uint64_t>());
                }
                std::print("  {} - {}s: {}\n", tag.index, (static_cast<double>(triggerTimeValue.value_or(std::uint64_t{}) - firstTimestamp)) * 1e-9, tag.map);
            }
        }
    };

    tag("timing-hardware") / "test_events_and_samples"_test = [] {
        using namespace gr;
        using namespace gr::testing;

        // publish timing events from a separate forked process because saftlib does not like us to spawn multiple threads
        const bool      verbose     = false;
        constexpr float sample_rate = 10000.f;
        Graph           testGraph;
        auto&           timingSrc = testGraph.emplaceBlock<gr::timing::TimingSource>({
            {"event_actions", gr::Tensor<pmt::Value>({"SIS100_RING:CMD_BP_START:BEAM-IN=1:BPC-START=0:0->IO1(400,on,8000,off)", //
                                            "301:256->IO1(100,on,110,off,140,on,150,off)",                                                //
                                            "SIS100_RING:CMD_BP_START->PUBLISH()"})},                                                     //
            {"io_events", true},                                                                                                //
            {"sample_rate", sample_rate},                                                                                       //
            {"verbose_console", verbose}                                                                                        //
        });
        auto&           sink      = testGraph.emplaceBlock<TagSink<uint8_t, ProcessFunction::USE_PROCESS_ONE>>({{"name", "TagSink"}, {"verbose_console", verbose}});

        expect(eq(ConnectionResult::SUCCESS, testGraph.connect<"out">(timingSrc).to<"in">(sink)));

        scheduler::Simple<gr::scheduler::ExecutionPolicy::multiThreaded> sched{};
        std::ignore = sched.exchange(std::move(testGraph));
        std::print("starting flowgraph\n");
        expect(sched.changeStateTo(gr::lifecycle::State::INITIALISED).has_value());
        auto res = sched.changeStateTo(gr::lifecycle::State::RUNNING);
        if (!res) {
            std::print("failed to start: {}\n", res.error());
        }
        expect(res.has_value());

        std::thread testEventDispatcherThread = dispatchSimulatedTiming();
        std::this_thread::sleep_for(6s);
        expect(sched.changeStateTo(gr::lifecycle::State::REQUESTED_STOP).has_value());
        std::this_thread::sleep_for(500ms);
        std::print("stopped flowgraph\n");

        testEventDispatcherThread.join();

        expect(approx(sink._samples.size(), 60000UZ, 1000UZ)) << "samples do not approximately correspond to the configured sample rate";
        expect(approx(sink._tags.size(), 50UZ, 10UZ)) << "expected approximately 50 tags";
        expect(std::ranges::equal(sink._tags | std::views::filter([](auto& tag) { return tag.map.contains(gr::tag::TRIGGER_META_INFO.shortKey()) && tag.map.at(gr::tag::TRIGGER_META_INFO.shortKey()).value_or(gr::property_map{}).contains("BPID"); }) | std::views::transform(getBPID), std::array{0, 6, 12, 18, 24})) << "Did not receive the correct timing tags with the correct BP indices";

        expect(std::ranges::equal(sink._samples | std::views::transform([](auto v) { return v >> 2; })                                    // drop irrelevant bits (LSB: tagPresent, LSB+1:output which toggles on timing events)
                                      | std::views::chunk_by(std::equal_to{})                                                             // merge and count identical samples
                                      | std::views::transform([](auto subrange) { return std::make_pair(subrange[0], subrange.size()); }) // get sample value and count as a pair
                                      | std::views::filter([](auto pair) { return pair.second > 3900UL && pair.second < 4100UL; })        // output changes are 200ms * 2 * 10000S/s = 4000 Samples
                                      | std::views::transform([](auto pair) { return pair.first; }),                                      // get sample values
            std::vector<int>{1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2}))
            << "expected almost 4 iterations of the pattern. the first sample is dropped, because it has less than 4000 samples";

        if (verbose) {
            verboseUpdateMessages(sink);
        }
    };

    tag("timing-hardware") / "test_events_only"_test = [] {
        using namespace gr;
        using namespace gr::testing;

        // publish timing events from a separate forked process because saftlib does not like us to spawn multiple threads
        const bool verbose = false;
        Graph      testGraph;
        auto&      timingSrc = testGraph.emplaceBlock<gr::timing::TimingSource>({
            {"sample_rate", 0.0f},                                                                                              //
            {"event_actions", gr::Tensor<pmt::Value>({"SIS100_RING:CMD_BP_START:BEAM-IN=1:BPC-START=0:0->IO1(400,on,8000,off)", //
                                       "301:256->IO1(100,on,110,off,140,on,150,off)",                                                //
                                       "SIS100_RING:CMD_BP_START->PUBLISH()"})},                                                     //
            {"io_events", true},                                                                                                //
            {"verbose_console", verbose}                                                                                        //
        });
        auto&      sink      = testGraph.emplaceBlock<TagSink<uint8_t, ProcessFunction::USE_PROCESS_ONE>>({{"name", "TagSink"}, {"verbose_console", verbose}});

        expect(eq(ConnectionResult::SUCCESS, testGraph.connect<"out">(timingSrc).to<"in">(sink)));

        scheduler::Simple<gr::scheduler::ExecutionPolicy::multiThreaded> sched{};
        std::ignore = sched.exchange(std::move(testGraph));
        std::print("starting flowgraph\n");
        expect(sched.changeStateTo(gr::lifecycle::State::INITIALISED).has_value());
        auto res = sched.changeStateTo(gr::lifecycle::State::RUNNING);
        if (!res && verbose) {
            std::print("failed to start: {}\n", res.error());
        }
        expect(res.has_value());

        std::thread testEventDispatcherThread = dispatchSimulatedTiming();
        std::this_thread::sleep_for(10s);
        expect(sched.changeStateTo(gr::lifecycle::State::REQUESTED_STOP).has_value());
        std::this_thread::sleep_for(500ms);
        std::print("stopped flowgraph\n");

        testEventDispatcherThread.join();

        expect(eq(sink._samples.size(), sink._tags.size())) << "For sample_rate=0.0f the number of samples and tags should be identical";
        expect(approx(sink._tags.size(), 60UZ, 10UZ)) << "Expected approximately 60 tags";
        expect(std::ranges::equal(sink._tags | std::views::filter([](auto& tag) { return tag.map.contains(gr::tag::TRIGGER_META_INFO.shortKey()) && tag.map.at(gr::tag::TRIGGER_META_INFO.shortKey()).value_or(gr::property_map{}).contains("BPID"); }) | std::views::transform(getBPID), std::array{0, 6, 12, 18, 24})) << "Did not receive the correct timing tags with the correct BP indices";
        std::size_t nIO = 0;
        for (const auto& [sample, tag] : std::views::zip(sink._samples, sink._tags)) {
            if (tag.map.contains(tag::TRIGGER_META_INFO.shortKey())) {
                auto metaMap = tag.map.at(tag::TRIGGER_META_INFO.shortKey()).get_if<property_map>();
                if (metaMap && metaMap->contains("IO-NAME") && (metaMap->at("IO-NAME") == "IO2" || metaMap->at("IO-NAME") == "IO3")) {
                    ++nIO;
                }
            }
        }
        expect(approx(nIO, 24, 2)) << "Wrong number of IO tags";

        std::size_t nEventIO = 0;
        for (const auto& [sample, tag] : std::views::zip(sink._samples, sink._tags)) {
            if (tag.map.contains(tag::TRIGGER_META_INFO.shortKey())) {
                auto metaMap = tag.map.at(tag::TRIGGER_META_INFO.shortKey()).get_if<property_map>();
                if (metaMap && metaMap->contains("IO-NAME") && metaMap->at("IO-NAME") == "IO1") {
                    ++nEventIO;
                }
            }
        }
        expect(approx(nEventIO, 30, 2)) << "Wrong number of event triggered IO tags";

        if (verbose) {
            verboseUpdateMessages(sink);
        }
    };
};

} // namespace fair::timing::test

int main() { /* tests are statically executed */ }
