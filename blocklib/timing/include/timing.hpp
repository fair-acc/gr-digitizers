#ifndef GR_DIGITIZERS_TIMING_HPP
#define GR_DIGITIZERS_TIMING_HPP
#include <string_view>
// gr
#include <gnuradio-4.0/CircularBuffer.hpp>
// timing
#include <SAFTd.h>
#include <TimingReceiver.h>
#include <SoftwareActionSink.h>
#include <SoftwareCondition.h>
#include <CommonFunctions.h>
#include <etherbone.h>
#include <Output_Proxy.hpp>
#include <Input_Proxy.hpp>
#include <Output.hpp>
#include <Input.hpp>
#include <OutputCondition_Proxy.hpp>
#include <OutputCondition.hpp>

// gr
#include <gnuradio-4.0/CircularBuffer.hpp>
#include <gnuradio-4.0/HistoryBuffer.hpp>

using saftlib::SAFTd_Proxy;
using saftlib::TimingReceiver_Proxy;
using saftlib::SoftwareActionSink_Proxy;
using saftlib::SoftwareCondition_Proxy;

static auto taiNsToUtc(auto input) {
    return std::chrono::utc_clock::to_sys(std::chrono::tai_clock::to_utc(std::chrono::tai_clock::time_point{} + std::chrono::nanoseconds(input)));
}

class Timing {
public:
    static const int milliToNano = 1000000;
    static const int minTriggerOffset = 100;
    struct Event {
        // eventid - 64
        uint8_t fid = 1;   // 4
        uint16_t gid;  // 12
        uint16_t eventNo; // 12
        bool flagBeamin;
        bool flagBpcStart;
        bool flagReserved1;
        bool flagReserved2;
        uint16_t sid; //12
        uint16_t bpid; //14
        bool reserved;
        bool reqNoBeam;
        uint8_t virtAcc; // 4
        // param - 64
        uint32_t bpcid; // 22
        uint64_t bpcts; // 42
        // timing system
        uint64_t time = 0;
        uint64_t executed = 0;
        uint16_t flags = 0x0;

        Event(const Event&) = default;
        Event(Event&&) = default;
        Event& operator=(const Event&) = default;
        Event& operator=(Event&&) = default;

        explicit Event(uint64_t timestamp = 0, uint64_t id = 1UL << 60, uint64_t param= 0, uint16_t _flags = 0, uint64_t _executed = 0) :
            // id
            fid           { static_cast<uint8_t>((id >> 60) & ((1UL <<  4) - 1))},
            gid           { static_cast<uint16_t>((id >> 48) & ((1UL << 12) - 1))},
            eventNo       { static_cast<uint16_t>((id >> 36) & ((1UL << 12) - 1))},
            flagBeamin    { static_cast<bool>((id >> 35) & ((1UL << 1) - 1))},
            flagBpcStart  { static_cast<bool>((id >> 34) & ((1UL << 1) - 1))},
            flagReserved1 { static_cast<bool>((id >> 33) & ((1UL << 1) - 1))},
            flagReserved2 { static_cast<bool>((id >> 32) & ((1UL << 1) - 1))},
            sid           { static_cast<uint16_t>((id >> 20) & ((1UL << 12) - 1))},
            bpid          { static_cast<uint16_t>((id >>  6) & ((1UL << 14) - 1))},
            reserved      { static_cast<bool>((id >>  5) & ((1UL <<  1) - 1))},
            reqNoBeam     { static_cast<bool>((id >> 4) & ((1UL << 1) - 1))},
            virtAcc       { static_cast<uint8_t>((id >>  0) & ((1UL <<  4) - 1))},
            // param
            bpcid         { static_cast<uint32_t>((param  >> 42) & ((1UL << 22) - 1))},
            bpcts         { (param  >>  0) & ((1UL << 42) - 1)},
            time { timestamp},
            executed { _executed},
            flags { _flags} { }

        [[nodiscard]] uint64_t id() const {
            // clang-format:off
            //       field             width        position
            return ((virtAcc & ((1UL <<  4) - 1)) <<  0)
                 + ((reqNoBeam + 0UL) << 4)
                 + ((reserved + 0UL)              <<  5)
                 + ((bpid    & ((1UL << 14) - 1)) <<  6)
                 + ((sid     & ((1UL << 12) - 1)) << 20)
                 + ((flagReserved2 + 0UL) << 32)
                 + ((flagReserved1 + 0UL) << 33)
                 + ((flagBpcStart + 0UL) << 34)
                 + ((flagBeamin + 0UL) << 35)
                 + ((eventNo & ((1UL << 12) - 1)) << 36)
                 + ((gid     & ((1UL << 12) - 1)) << 48)
                 + ((fid     & ((1UL <<  4) - 1)) << 60);
            // clang-format:on
        }

        [[nodiscard]] uint64_t param() const {
            // clang-format:off
            //       field             width        position
            return ((bpcts & ((1UL << 42) - 1)) <<  0)
                 + ((bpcid & ((1UL << 22) - 1)) << 42);
            // clang-format:on
        }

        static std::optional<Event> fromString(std::string_view line) {
            using std::operator""sv;
#if defined(__clang__)
            std::array<uint64_t, 3> elements{};
            std::size_t found = 0;
            std::size_t startingIndex = 0;
            for (std::size_t i = 0; i <= line.size() && found < elements.size(); i++) {
                if (i == line.size() || line[i] == ' ') {
                    if (startingIndex < i) {
                        auto parse = [](auto el) { return std::stoul(std::string(std::string_view(el)), nullptr, 0); };
                        elements[found++] = parse(line.substr(startingIndex, i - startingIndex));
                    }
                    startingIndex = i;
                }
            }
            if (found >= 3) {
                return Event{elements[2], elements[0], elements[1]};
            }
#else
            auto event = std::views::split(line, " "sv) | std::views::take(3)
                         | std::views::transform([](auto n) { return std::stoul(std::string(std::string_view(n)), nullptr, 0); })
                         | std::views::adjacent_transform<3>([](auto a, auto b, auto c) {return Timing::Event{c, a, b};});
            if (!event.empty()) {
                return event.front();
            }
#endif
            return {};
        }

        static void loadEventsFromString(std::vector<Timing::Event> &events, std::string_view string) {
            events.clear();
            using std::operator""sv;
            try {
                for (auto line : std::views::split(string, "\n"sv)) {
                    auto event = Timing::Event::fromString(std::string_view{line});
                    if (event) {
                        events.push_back(*event);
                    }
                }
            } catch (std::exception &e) {
                events.clear();
                fmt::print("Error parsing data: {}", string);
            }
        }

    };

    struct Trigger {
        std::array<bool, 20> outputs;
        uint64_t id;
        double delay; // [ms]
        double flattop; // [ms]

        bool operator<=>(const Trigger&) const = default;
    };

    gr::CircularBuffer<Event, 10000> snooped{10000};
    std::vector<std::tuple<uint, std::string, std::string>> outputs;
    std::map<uint64_t, Trigger> triggers;
    std::vector<Timing::Event> events = {};
private:
    decltype(snooped.new_writer()) snoop_writer = snooped.new_writer();
    bool tried = false;
    std::shared_ptr<SAFTd_Proxy> saftd;
    std::shared_ptr<SoftwareActionSink_Proxy> sink;
    std::shared_ptr<SoftwareCondition_Proxy> condition;
public:
    bool initialized = false;
    bool simulate = false;
    uint64_t snoopID = 0x0;
    uint64_t snoopMask = 0x0;
    std::shared_ptr<TimingReceiver_Proxy> receiver;

    void updateSnoopFilter() {
        if (simulate) return;
        if (condition) {
            condition->Destroy();
        }
        condition = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 0));
        condition->setAcceptLate(true);
        condition->setAcceptEarly(true);
        condition->setAcceptConflict(true);
        condition->setAcceptDelayed(true);
        condition->SigAction.connect(
                [this](uint64_t id, uint64_t param, const saftlib::Time& deadline, const saftlib::Time& executed,
                       uint16_t flags) {
                    this->snoop_writer.publish(
                            [id, param, &deadline, &executed, flags](std::span<Event> buffer) {
                                buffer[0] = Timing::Event{deadline.getTAI(), id, param, flags, executed.getTAI()};
                            }, 1);
                });
        condition->setActive(true);
    }

    void initialize() {
        if (simulate) {
            initialized = true;
        } else {
            try {
                saftd = SAFTd_Proxy::create();
                // get a specific device
                std::map<std::string, std::string> devices = saftd->getDevices();
                if (devices.empty()) {
                    std::cerr << "No devices attached to saftd" << std::endl;
                }
                receiver = TimingReceiver_Proxy::create(devices.begin()->second);
                sink = SoftwareActionSink_Proxy::create(receiver->NewSoftwareActionSink("gr_timing_example"));
                updateSnoopFilter();
                for (const auto & [i, output]: receiver->getOutputs() | std::views::enumerate ) {
                    const auto &[name, port] = output;
                    outputs.emplace_back(i, name, port);
                }
                initialized = true;
            } catch (...) {}
        }
    }

    void process() {
        if (!initialized && !tried) {
            tried = true;
            initialize();
        } else if (initialized & !simulate) {
            const auto startTime = std::chrono::system_clock::now();
            while(true) {
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(startTime - std::chrono::system_clock::now() + std::chrono::milliseconds(5)).count();
                if (duration > 0) {
                    saftlib::wait_for_signal(static_cast<int>(std::clamp(duration, 50L, std::numeric_limits<int>::max()+0L)));
                } else {
                    break;
                }
            }
        }
    }

    void injectEvent(const Event &ev, uint64_t time_offset) {
        if (simulate && ((ev.id() & snoopMask) == (snoopID & snoopMask)) ) {
            this->snoop_writer.publish(
                    [ev, time_offset](std::span<Event> buffer) {
                        buffer[0] = ev;
                        buffer[0].time += time_offset;
                        buffer[0].executed = buffer[0].time;
                    }, 1);
        } else if (!simulate) {
            receiver->InjectEvent(ev.id(), ev.param(), saftlib::makeTimeTAI(ev.time + time_offset));
        }
    }

    unsigned long getTAI() {
        if (simulate) {
            return static_cast<unsigned long>(std::max(0L, duration_cast<std::chrono::nanoseconds>(std::chrono::utc_clock::now().time_since_epoch()).count()));
        }
        return receiver->CurrentTime().getTAI();
    }

    void updateTrigger(Trigger &trigger) {
        auto existing = triggers.find(trigger.id);
        if (existing != triggers.end() && existing->second == trigger) {
            return; // nothing changed
        }
        if (!simulate) {
#if defined(__clang__)
            std::size_t i = 0;
            for (const auto &output : outputs) {
                bool enabled = trigger.outputs[i++];
#else
            for (const auto [i, output, enabled] : std::views::zip(std::views::iota(0), outputs, trigger.outputs)) {
#endif
                if (enabled && (existing == triggers.end() || !existing->second.outputs[static_cast<unsigned long>(i)])) { // newly enabled
                    auto proxy = saftlib::Output_Proxy::create(std::get<2>(output));
                    proxy->NewCondition(true, trigger.id, std::numeric_limits<uint64_t>::max(), static_cast<int64_t>(trigger.delay) * milliToNano + minTriggerOffset, true);
                    proxy->NewCondition(true, trigger.id, std::numeric_limits<uint64_t>::max(), static_cast<int64_t>(trigger.delay + trigger.flattop) * milliToNano + minTriggerOffset, false);
                    auto [inserted, _] = triggers.try_emplace(trigger.id, trigger);
                    existing = inserted;
                } else if (!enabled && existing != triggers.end() && existing->second.outputs[static_cast<unsigned long>(i)]) { // newly disabled
                    auto proxy = saftlib::Output_Proxy::create(std::get<2>(output));
                    auto matchingConditions = proxy->getAllConditions()
                            | std::views::transform([](const auto &cond) { return saftlib::OutputCondition_Proxy::create(cond); })
                            | std::views::filter([&trigger](const auto &cond) { return cond->getID() == trigger.id && cond->getMask() == std::numeric_limits<uint64_t>::max(); });
                    std::ranges::for_each(matchingConditions, [](const auto &cond) {
                        cond->Destroy();
                    });
                } else if (existing != triggers.end()) {
                    auto proxy = saftlib::Output_Proxy::create(std::get<2>(output));
                    if (trigger.delay != existing->second.delay || trigger.flattop != existing->second.flattop) { // update condition for rising edge
                        auto matchingConditions = proxy->getAllConditions()
                                                    | std::views::transform([](const auto &cond) { return saftlib::OutputCondition_Proxy::create(cond); })
                                                    | std::views::filter([&trigger](const auto &cond) { return cond->getID() == trigger.id && cond->getMask() == std::numeric_limits<uint64_t>::max(); });
                        std::ranges::for_each(matchingConditions, [&trigger](const auto &cond) {
                            if (cond->getOn()) {
                                cond->setOffset(static_cast<int64_t>(trigger.delay) * milliToNano + minTriggerOffset);
                            } else {
                                cond->setOffset(static_cast<int64_t>(trigger.delay + trigger.flattop) * milliToNano + minTriggerOffset);
                            }
                        });
                    }
                }
            }
        }
        triggers.insert_or_assign(trigger.id, trigger);
    }
};

#endif
