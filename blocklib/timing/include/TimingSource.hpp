#ifndef GR_DIGITIZERS_TIMINGSOURCE_HPP
#define GR_DIGITIZERS_TIMINGSOURCE_HPP

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <optional>
#include <queue>
#include <random>
#include <thread>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/Tag.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>

#include "gnuradio-4.0/TriggerMatcher.hpp"

#include "./event_definitions.hpp"
#include "./timing.hpp"

namespace gr::timing {

// static std::uint64_t taiNsToUtcNs(uint64_t input) { return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(taiNsToUtc(input).time_since_epoch()).count()); }
//  should use some library function after validating the correct timestamp
static std::uint64_t taiNsToUtcNs(uint64_t input) { return input - std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(37u)).count(); }

struct TimingSource : public gr::Block<TimingSource, BlockingIO<true>> {
    template<typename T, gr::meta::fixed_string description = "", typename... Arguments>
    using A               = gr::Annotated<T, description, Arguments...>;
    using Description     = Doc<R""(A source block that generates clock signals as they are received from a White Rabbit timing receiver.

The `event_actions` parameter is a list of strings, where each defines a matcher and a list of actions, separated by an arrow `->` character sequence.
The matcher has the format `<GID>[:<EVT>[:BPC-START=<1|0>[:BEAM-IN=<1|0>[:<SID>[:BPID]]]]]`, where GID and EVT can be either Names or numeric values for
the timing group and event-number, BPC-START and BEAM-IN are boolean flags and SID and BPID are numeric values for the sequence id and the beam-process id.
Due to Hardware constraints it is not possible to only specify a value in the right part of the matcher without specifying all of the values left of it,
as they get mapped to prefix bit-mask.
The comma separated list of action can either contain the special value `PUBLISH()`, which will cause the matched events to be published as tags to the
output port, or the name of a port and a list of alternating delays and port-states in the parentheses: `IO1(<delay us>,<on|off>,...)` which allows to
e.g. trigger complex on/off patterns on one or multiple ports with on a single matching event or having one event switch an output on and another one
switching it off again.

The following settings will generate a 8ms pulse on the DIO1 output of the timing receiver and publish a stream that outputs one sample with the timing
tag for every "BP_START" timing event in the SIS100 timing group.
```
sample_rate: 0.0
event_actions:
  - "SIS100_RING:CMD_BP_START->IO1(400,on,8000,off)"
  - "SIS100_RING->PUBLISH()"
```
will lead to output in the following form:
```
samples:  [1, 3, ....]
tags: [
  0 -> { BEAM-IN: true, BPC-START: false, BPCID: 0, BPCTS: 0, BPID: 0, EVENT-NAME: CMD_BP_START, EVENT-NO: 256, GID: 310, SID: 0, TIMING-GROUP: UNKNOWN-TIMING-GROUP, TIMING-ID: 1240196373932933120, TIMING-PARAM: 0, gr:trigger_offset: 0, trigger_name: CMD_BP_START/FAIR-TIMING:B=0.P=0.C=0.T=310, trigger_time: 633103525799216 }
  1 -> { IO-LEVEL: true, IO-NAME: IO1, TIMING-ID: 18446181123756130306, TIMING-PARAM: 0, gr:trigger_offset: 0, trigger_name: IO2_FALLING, trigger_time: 633103024372976 }
  ...
]
```
If you need a continuous stream of samples synchronised to the timing tags, you can set a non-zero sample rate, e.g. `sample_rate: 250.0` in Hz, which will cause the block to output
data at that average sampling rate. If no events are received, there is a configured delay at which the samples will be published anyway.
```
samples:  [0, 1, 0, 0, 3, 2, 2, ...]
tags: [
  1 -> { BEAM-IN: true, BPC-START: false, BPCID: 0, BPCTS: 0, BPID: 0, EVENT-NAME: CMD_BP_START, EVENT-NO: 256, GID: 310, SID: 0, TIMING-GROUP: UNKNOWN-TIMING-GROUP, TIMING-ID: 1240196373932933120, TIMING-PARAM: 0, gr:trigger_offset: 0, trigger_name: CMD_BP_START/FAIR-TIMING:B=0.P=0.C=0.T=310, trigger_time: 633103525799216 }
  4 -> { IO-LEVEL: true, IO-NAME: IO1, TIMING-ID: 18446181123756130306, TIMING-PARAM: 0, gr:trigger_offset: 0, trigger_name: IO2_FALLING, trigger_time: 633103024372976 }
  ...
]
 ```

Also signal changes on the digital IOs will be added as tags to the output ports and change the value that will be output on the output port's bits left of the LSB(which indicates the presence of a tag):
```
timingSource.out [uint8t]:     0b   0    0    0    0    1    1    1    1
Timing Receiver IO port state:                         IO3  IO2  IO1  tag_present
```

This block does not change the configuration of the ports as input or output ports, so if you want to use a port as an output port you will have to configure
it accordingly using the `saft-io-ctl` utility.
)"">;
    using ClockSourceType = std::chrono::tai_clock;
    using TimePoint       = std::chrono::time_point<ClockSourceType>;

    PortOut<std::uint8_t> out;
    // TODO: PortIn< std::uint8_t, Async> DIO1..3 // Follow-up-PR
    MsgPortIn ctxInfo;

    A<std::vector<std::string>, "event actions", Doc<"Configure which timing events should trigger IO port changes and/or get forwarded as timing tags. Syntax description and examples in the block documentation.">, Visible> event_actions;
    A<float, "avg. sample rate", Doc<"Controls the sample rate at which to publish the digital output state. A value of 0.0f means samples are published only when there's an event.">, Visible>                                sample_rate     = 1000.f;
    A<std::string, "timing device name", Doc<"Specifies the timing device to use. In case it is left empty, the first timing device that is found is used.">>                                                                   timing_device   = "";
    A<std::uint64_t, "max delay", Doc<"Maximum delay for messages from the timing hardware. Only used for sample_rate != 0.0f">, Unit<"ns">>                                                                                    max_delay       = 10'000'000; // 10 ms // todo: uint64t ns
    A<bool, "verbose console", Doc<"For debugging">>                                                                                                                                                                            verbose_console = false;
    // TODO: enable/disable publishing on input port changes -> For now everything is published, add in follow-up PR

    GR_MAKE_REFLECTABLE(TimingSource, out, ctxInfo, event_actions, sample_rate, timing_device, max_delay, verbose_console);

    using ConditionsType = std::map<std::string, std::map<std::string, std::variant<std::shared_ptr<saftlib::OutputCondition_Proxy>, std::shared_ptr<saftlib::SoftwareCondition_Proxy>>>>;
    Timing _timing;
    using EventReaderType         = decltype(_timing.snooped.new_reader());
    EventReaderType _event_reader = _timing.snooped.new_reader();
    ConditionsType  _conditionProxies;
    TimePoint       _startTime;
    std::size_t     _publishedSamples = 0;
    std::uint8_t    _lastOutputState  = 0;
    std::uint8_t    _outputState      = 0;
    std::uint8_t    _nextOutputState  = 0;

    void start() {
        if (verbose_console) {
            fmt::println("starting {}", this->name);
        }
        _timing.saftAppName = fmt::format("{}_{}", this->unique_name | std::views::filter([](auto c) { return std::isalnum(c); }) | std::ranges::to<std::string>(), getpid());
        _timing.snoopID     = 0xffffffffffffffffull; // Workaround: make the default condition of _timing not listen to anything
        _timing.snoopMask   = 0xffffffffffffffffull;
        _timing.initialize();
        updateEventTriggers();
        _startTime = TimePoint(std::chrono::nanoseconds(_timing.currentTimeTAI()));
    }

    void stop() {
        if (verbose_console) {
            fmt::println("stop {}", this->name);
        }
        this->requestStop();
    }

    static std::pair<std::uint64_t, std::uint64_t> parseFilter(const std::string& newFilter) {
        std::vector<std::string_view> filterTokens = std::views::split(newFilter, ":"sv) | std::views::transform([](auto r) { return std::string_view(r.begin(), r.end()); }) | std::ranges::to<std::vector>();
        // initialize with the fixed format identifier
        uint64_t filter = 0x1000000000000000UL;
        uint64_t mask   = 0xf000000000000000UL;
        // timing group ID, given as string or numeric value
        if (filterTokens.empty()) {
            return {filter, mask};
        }
        if (!filterTokens[0].empty()) {
            uint64_t numericGroupId;
            if (std::from_chars_result result = std::from_chars(filterTokens[0].begin(), filterTokens[0].end(), numericGroupId); result.ptr == filterTokens[0].end()) {
                filter |= numericGroupId << 48;
            } else if (auto gidFromName = timingGroupTable | std::views::filter([&e = filterTokens[0]](auto& m) { return e == m.second.first; }) | std::views::transform([](auto& m) { return m.first; }) | std::views::take(1) | std::ranges::to<std::vector>(); !gidFromName.empty()) {
                filter |= static_cast<uint64_t>(gidFromName[0]) << 48;
            } else {
                throw gr::exception(fmt::format("Illegal Timing Group Name/Number: {}", filterTokens[0]));
            }
            mask |= 0xfffull << 48;
        }
        // event by name or numeric ID
        if (filterTokens.size() > 1) {
            uint64_t numericEventId;
            if (std::from_chars_result result = std::from_chars(filterTokens[1].begin(), filterTokens[1].end(), numericEventId); result.ptr == filterTokens[1].end() && !filterTokens[1].empty()) {
                filter |= numericEventId << 36;
            } else if (auto evtNrFromName = eventNrTable | std::views::filter([&e = filterTokens[1]](auto& m) { return e == m.second.first; }) | std::views::transform([](auto& m) { return m.first; }) | std::views::take(1) | std::ranges::to<std::vector>(); !evtNrFromName.empty()) {
                filter |= static_cast<std::uint64_t>(evtNrFromName[0]) << 36;
            } else {
                throw gr::exception(fmt::format("Illegal EventName/Number: {}", filterTokens[1]));
            }
            mask |= 0xfffull << 36;
        }
        // FLAGS BEAM-IN
        if (filterTokens.size() > 2) {
            if (filterTokens[2] == "BEAM-IN=1") {
                filter |= 0x1ULL << 35;
            } else if (filterTokens[2] == "BEAM-IN=0") {
                filter |= 0x0ULL << 35;
            } else {
                throw gr::exception(fmt::format("BEAM-IN flag has to be 1 or 0, was {}", filterTokens[2]));
            }
            mask |= 0x1ULL << 35;
        }
        // FLAGS BPC-START
        if (filterTokens.size() > 3) {
            if (filterTokens[3] == "BPC-START=1") {
                filter |= 0x1ULL << 34;
            } else if (filterTokens[3] == "BPC-START=0") {
                filter |= 0x0ULL << 34;
            } else {
                throw gr::exception(fmt::format("BPC-START flag has to be 1 or 0, was {}", filterTokens[3]));
            }
            mask |= 0x1ULL << 34;
        }
        // numeric Sequence id (to be extended to LSA pattern)
        if (filterTokens.size() > 4) {
            mask |= 0x3ULL << 32; // unused flag bits have to be set in mask for filter to be a prefix
            uint64_t numericSid;
            if (std::from_chars_result result = std::from_chars(filterTokens[4].begin(), filterTokens[4].end(), numericSid); result.ptr == filterTokens[4].end() && !filterTokens[1].empty()) {
                filter |= numericSid << 20u;
                mask |= 0xfffull << 20u;
            } else {
                throw gr::exception(fmt::format("Illegal Sequence ID: {}", filterTokens[4]));
            }
        }
        // numeric beam process id (to be extended to LSA pattern)
        if (filterTokens.size() > 5) {
            uint64_t numericBP;
            if (std::from_chars_result result = std::from_chars(filterTokens[5].begin(), filterTokens[5].end(), numericBP); result.ptr == filterTokens[5].end() && !filterTokens[1].empty()) {
                filter |= numericBP << 6u;
                mask |= 0x3fffull << 6u;
            } else {
                throw gr::exception(fmt::format("Illegal Beam Process ID: {}", filterTokens[5]));
            }
        }
        if (filterTokens.size() > 6) {
            throw gr::exception(fmt::format("Filter Pattern contains too many tokens: {}", std::span(filterTokens.begin() + 6, filterTokens.end())));
        }
        return {filter, mask};
    }

    static auto parseTriggerAction(const std::string& actions) {
        std::vector<std::pair<std::string, std::vector<std::pair<std::uint64_t, std::string>>>> parsedActions;
        std::size_t                                                                             actionPos = 0;
        while (true) {
            unsigned long actionStart = actions.find('(', actionPos);
            if (actionStart == std::string::npos) {
                throw gr::exception(fmt::format("Invalid action format, missing opening parenthesis, definition: {}, pos: {}", actions, actionPos));
            }
            std::size_t actionEnd = actions.find(')', actionStart);
            if (actionEnd == std::string::npos) {
                throw gr::exception(fmt::format("Invalid action format, missing closing parenthesis, definition: {}, pos: {}", actions, actionPos));
            }
            auto parsedAction = std::string_view(actions.begin() + static_cast<long>(actionStart) + 1uz, actions.begin() + static_cast<long>(actionEnd)) //
                                | std::views::split(","sv) | std::views::pairwise | std::views::stride(2) | std::views::transform([](auto t) {
                                      auto [tt, a] = t;
                                      std::uint64_t delay;
                                      if (std::from_chars_result result = std::from_chars(tt.begin(), tt.end(), delay); result.ptr != tt.end() || tt.empty()) {
                                          throw gr::exception(fmt::format("Invalid action format, cannot parse delay to value: {}, error-code: {}", tt, make_error_code(result.ec).value()));
                                      }
                                      return std::pair{delay, std::string(a.begin(), a.end())};
                                  }) |
                                std::ranges::to<std::vector>();
            parsedActions.push_back(std::pair{actions.substr(actionPos, actionStart - actionPos), std::move(parsedAction)});
            if (actionEnd + 1 >= actions.size()) {
                break;
            } else if (actions[actionEnd + 1] != ',') {
                throw gr::exception(fmt::format("Invalid action format, actions should be separated by comma, definition: {}, pos: {}", actions, actionEnd));
            } else {
                actionPos = actionEnd + 2;
            }
        }
        return parsedActions;
    }

    static auto splitTriggerActions(const std::string& newTrigger) {
        const std::size_t pos = newTrigger.find("->");
        if (pos == std::string::npos) {
            throw gr::exception(fmt::format("Invalid trigger definition (must contain '->' separator): {}", newTrigger));
        }
        auto trigger = newTrigger.substr(0, pos);
        auto actions = newTrigger.substr(pos + 2);
        return std::pair{trigger, actions};
    }

    void updateEventTriggers() {
        auto outputs = _timing.receiver->getOutputs() | std::views::transform([&sigGroup = _timing.saftSigGroup](auto kvp) {
            auto& [name, path] = kvp;
            return std::pair(name, saftlib::Output_Proxy::create(path, sigGroup));
        }) | std::ranges::to<std::map>();

        std::set<std::string> keepMatcher;
        for (auto& trigger : event_actions.value) {
            if (verbose_console) {
                fmt::print("adding new trigger: {}\n", trigger);
            }
            auto [triggerMatcher, triggerActions] = splitTriggerActions(trigger);
            keepMatcher.insert(triggerMatcher);
            if (!_conditionProxies.contains(triggerMatcher)) {
                std::set<std::string> keepAction;
                auto [filter, mask] = parseFilter(triggerMatcher);
                auto parsedActions  = parseTriggerAction(triggerActions);
                for (auto& [ioName, changes] : parsedActions) {
                    if (ioName == "PUBLISH") {
                        std::string triggerAction = ioName;
                        keepAction.insert(triggerAction);
                        if (!_conditionProxies[triggerMatcher].contains(triggerAction)) {
                            if (verbose_console) {
                                fmt::print("adding software condition for trigger {:#x}:{:#x}\n", filter, mask);
                            }
                            if (!changes.empty()) {
                                throw gr::exception(fmt::format("Illegal software condition specification: cannot have arguments: {}", parsedActions));
                            }
                            auto condition = saftlib::SoftwareCondition_Proxy::create(_timing.sink->NewCondition(false, filter, mask, 0), _timing.saftSigGroup);
                            condition->setAcceptLate(true);
                            condition->setAcceptEarly(true);
                            condition->setAcceptConflict(true);
                            condition->setAcceptDelayed(true);
                            condition->SigAction.connect([this](uint64_t id, uint64_t param, const saftlib::Time& deadline, const saftlib::Time& executed, uint16_t flags) {
                                auto data = _timing.snoop_writer.reserve<gr::SpanReleasePolicy::ProcessAll>(1);
                                data[0]   = Timing::Event{deadline.getTAI(), id, param, flags, executed.getTAI()};
                            });
                            condition->setActive(true);
                            _conditionProxies[triggerMatcher].insert({triggerAction, std::move(condition)});
                            continue;
                        }
                    }
                    if (!outputs.contains(ioName)) {
                        if (verbose_console) {
                            fmt::print("ignoring non-existent output {}, available output ports: {}\n", ioName, outputs | std::views::keys);
                        }
                        continue;
                    }
                    for (auto& [delay, state] : changes) {
                        std::string triggerAction = fmt::format("{}({},{})", ioName, delay, state);
                        keepAction.insert(triggerAction);
                        if (!_conditionProxies[triggerMatcher].contains(triggerAction)) {
                            if (verbose_console) {
                                fmt::print("adding output condition for trigger {:#x}:{:#x}, setting {} to {} after {}ns\n", filter, mask, ioName, state, delay * 1000ull);
                            }
                            std::uint64_t io_edge = state == "on" ? 1ull : 0ull;
                            try {
                                _conditionProxies[triggerMatcher].insert({triggerAction, saftlib::OutputCondition_Proxy::create(outputs[ioName]->NewCondition(true, filter, mask, static_cast<int64_t>(delay * 1000ull), io_edge), _timing.saftSigGroup)});
                            } catch (...) {
                                throw gr::exception(fmt::format("failed to add output condition for trigger {:#x}:{:#x}, setting {} to {} after {}ns\n", filter, mask, ioName, state, delay * 1000ull));
                                if (verbose_console) {
                                    fmt::print("failed to add output condition for trigger {:#x}:{:#x}, setting {} to {} after {}ns\n", filter, mask, ioName, state, delay * 1000ull);
                                }
                            }
                        }
                    }
                }
                std::erase_if(_conditionProxies[triggerMatcher], [&keepAction](const auto& item) { return !keepAction.contains(item.first); });
            }
        }
        std::erase_if(_conditionProxies, [&keepMatcher](const auto& item) { return !keepMatcher.contains(item.first); });
    }

    void settingsChanged(const property_map& oldSettings, const property_map& newSettings) {
        using std::operator""sv;
        // configure io ports based on settings
        if (newSettings.contains("timing_device") && timing_device != std::get<std::string>(oldSettings.at("timing_device"))) {
            if (_timing.receiver) {
                timing_device = std::get<std::string>(oldSettings.at("timing_device"));
                throw gr::exception(std::format("Changing the timing receiver device name while the timing source is running is not supported."));
            }
            _timing.deviceName = timing_device;
        } // end io triggers
        if (newSettings.contains("event_actions") && event_actions != std::get<std::vector<std::string>>(oldSettings.at("event_actions"))) {
            if (_timing.receiver) {
                updateEventTriggers();
            }
        } // end io triggers
    }

    Tag eventToTag(const Timing::Event& event) {
        Tag              tag;
        gr::property_map meta;
        std::uint64_t    id = event.id();
        tag.map.emplace(tag::TRIGGER_TIME.shortKey(), taiNsToUtcNs(event.time));
        meta.emplace("TIMING-ID", id);
        meta.emplace("TIMING-PARAM", event.param());
        if (event.isIo) {
            bool        level = static_cast<bool>(id & 0x1ull);
            std::string name  = _timing.idToIoName(id);
            meta.emplace("IO-NAME", name);
            meta.emplace("IO-LEVEL", level);
            tag.map.emplace(tag::TRIGGER_NAME.shortKey(), fmt::format("{}_{}", name, level ? "RISING" : "FALLING"));
            tag.map.emplace(tag::TRIGGER_OFFSET.shortKey(), 0.0f);
        } else {
            meta.emplace("GID", event.gid);
            if (timingGroupTable.contains(event.gid)) {
                meta.emplace("TIMING-GROUP", timingGroupTable.at(event.gid).first);
            } else {
                meta.emplace("TIMING-GROUP", "UNKNOWN-TIMING-GROUP");
            }
            meta.emplace("EVENT-NO", event.eventNo);
            const std::string eventName = [&event]() -> std::string {
                if (eventNrTable.contains(event.eventNo)) {
                    return eventNrTable.at(event.eventNo).first;
                } else {
                    return "UNKNOWN-EVENT"s;
                }
            }();
            meta.emplace("EVENT-NAME", eventName);
            tag.map.emplace(tag::TRIGGER_NAME.shortKey(), eventName);
            tag.map.emplace(tag::CONTEXT.shortKey(), fmt::format("FAIR-TIMING:C={}.S={}.P={}.T={}", event.bpcid, event.sid, event.bpid, event.gid));
            meta.emplace("BPCTS", event.bpcts); // chain execution time-stamp (i.e. unique chain identifier)
            meta.emplace("BPCID", event.bpcid); // chain ID (can contain multiple sequences)
            meta.emplace("SID", event.sid);     // chain ID -> sequence ID (can contain multiple beam-processes)
            meta.emplace("BPID", event.bpid);   // beam process ID (PID)
            meta.emplace("BEAM-IN", event.flagBeamin);
            meta.emplace("BPC-START", event.flagBpcStart);
            tag.map.emplace(tag::TRIGGER_OFFSET.shortKey(), 0.0f); // The trigger offset has to be set either when publishing at fixed sample rate or when adding the tag to a sample e.g. in the picoscope block
        }
        tag.map.emplace(tag::TRIGGER_META_INFO.shortKey(), meta);
        return tag;
    }

    void updateOutputState(const Timing::Event& event) {
        // check if the event changed the output state and update it accordingly
        // this logic makes sure that a bit is only changed once per published sample -> even short pulses or drops will at least trigger one sample flank
        std::uint8_t inputNr = static_cast<std::uint8_t>(event.id() & ~Timing::ECA_EVENT_MASK_LATCH) >> 1;
        std::uint8_t mask    = static_cast<std::uint8_t>(0x1 << (inputNr + 1));
        _nextOutputState     = (_nextOutputState & ~mask) | static_cast<std::uint8_t>((event.id() & 0x1ull) << (inputNr + 1));
        if ((_outputState & mask) == (_lastOutputState & mask)) { // if the bit was already changed since the last sample
            _outputState = (_nextOutputState & mask) | (_outputState & ~mask);
        }
    }

    work::Status processBulk(OutputSpanLike auto& outSpan) noexcept {
        // update context information
        _timing.saftSigGroup.wait_for_signal(static_cast<int>(max_delay >> 22ul)); // process saftlib events, max_delay >> 22 is ~ 1/4 of max delay in ms
        auto timingEvents = _event_reader.template get<SpanReleasePolicy::ProcessAll>();
        // publish to output port and message port
        std::size_t toPublish = 0;
        for (const Timing::Event& event : timingEvents) {
            gr::Tag timingTag = eventToTag(event);
            // TODO: make stable against rounding errors by using global time difference instead of just between 2 events
            std::size_t samplesUntilCurrentEvent = 0;
            if (sample_rate != 0.0f) {
                long publishedNs         = static_cast<long>(std::ceil(static_cast<double>(_publishedSamples) / static_cast<double>(sample_rate) * 1e9));
                samplesUntilCurrentEvent = static_cast<size_t>(std::floor(static_cast<double>((TimePoint(std::chrono::nanoseconds(event.time)) - _startTime).count() - publishedNs) * static_cast<double>(sample_rate) * 1e-9));
                if (samplesUntilCurrentEvent > 0) {
                    if (_nextOutputState != _outputState) {
                        _lastOutputState = _nextOutputState;
                        _outputState     = _nextOutputState;
                    }
                    std::ranges::fill(std::span(outSpan).subspan(toPublish, samplesUntilCurrentEvent), _outputState);
                    toPublish += samplesUntilCurrentEvent;
                }
                std::uint64_t offset                              = event.time - static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(_startTime.time_since_epoch()).count()) - static_cast<uint64_t>(static_cast<double>(_publishedSamples + samplesUntilCurrentEvent) / static_cast<double>(sample_rate) * 1e9);
                timingTag.map[gr::tag::TRIGGER_OFFSET.shortKey()] = offset;
            } else { // sample_rate == 0.0f -> publish one sample per timing tag
                samplesUntilCurrentEvent = 1;
                if (_nextOutputState != _outputState) {
                    _lastOutputState = _nextOutputState;
                    _outputState     = _nextOutputState;
                }
                outSpan[toPublish] = _outputState + 1;
                toPublish++;
            }
            if (event.isIo) {
                updateOutputState(event);
                outSpan[toPublish - 1] = _outputState + 1;
            }
            outSpan.publishTag(timingTag.map, static_cast<long>(toPublish - 1));
            if (verbose_console) {
                fmt::print("publishing tag, {} samples before, then sample with tag {}\n", samplesUntilCurrentEvent, timingTag.map);
            }
        }

        // publish more samples if last sample is further in the past than the maximum allowed latency
        if (sample_rate != 0.0f) {
            auto now         = TimePoint(std::chrono::nanoseconds(_timing.currentTimeTAI()));
            long publishedNs = static_cast<long>(std::ceil(static_cast<double>(_publishedSamples + toPublish) / static_cast<double>(sample_rate) * 1e9));
            auto catchUpNs   = (now - _startTime).count() - static_cast<long>(max_delay) - publishedNs;
            if (catchUpNs > 0) {
                std::size_t samplesUntilCurrentEvent = static_cast<size_t>(std::floor(static_cast<double>(catchUpNs) * static_cast<double>(sample_rate) * 1e-9));
                if (samplesUntilCurrentEvent > 0) {
                    if (_nextOutputState != _outputState) {
                        _lastOutputState = _nextOutputState;
                        _outputState     = _nextOutputState;
                    }
                    std::ranges::fill(std::span(outSpan).subspan(toPublish, samplesUntilCurrentEvent), _outputState);
                    toPublish += samplesUntilCurrentEvent;
                }
            }
        }
        outSpan.publish(toPublish);
        _publishedSamples += toPublish;

        return work::Status::OK;
    }

    void processMessages(gr::MsgPortIn& portIn, std::span<const gr::Message> messages) {
        if (portIn.name == ctxInfo.name) { // TODO: check why portIn cannot be directly compared
            for (auto& msg : messages) {
                if (msg.data.has_value()) {
                    // these messages may contain context information and should be used to update some lookup table inside the block and possibly update timing filters
                    // the exact format and content of messages and cache has yet to be determined
                }
            }
        }
    }
};

} // namespace gr::timing

auto registerTimingSource = gr::registerBlock<gr::timing::TimingSource>(gr::globalBlockRegistry());
static_assert(gr::HasProcessBulkFunction<gr::timing::TimingSource>);

#endif // GR_DIGITIZERS_TIMINGSOURCE_HPP
