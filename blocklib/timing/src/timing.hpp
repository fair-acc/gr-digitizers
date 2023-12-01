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
#include <OutputCondition.hpp>

// gr
#include <gnuradio-4.0/CircularBuffer.hpp>
#include <gnuradio-4.0/HistoryBuffer.hpp>
#include "plot.hpp"

using saftlib::SAFTd_Proxy;
using saftlib::TimingReceiver_Proxy;
using saftlib::SoftwareActionSink_Proxy;
using saftlib::SoftwareCondition_Proxy;

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

        explicit Event(uint64_t timestamp = 0, uint64_t id = 1ul << 60, uint64_t param= 0, uint16_t _flags = 0, uint64_t _executed = 0) {
            time = timestamp;
            flags = _flags;
            executed = _executed;
            // id
            virtAcc        = (id >>  0) & ((1ul <<  4) - 1);
            reqNoBeam      = (id >> 4) & ((1ul << 1) - 1);
            reserved       = (id >>  5) & ((1ul <<  1) - 1);
            bpid           = (id >>  6) & ((1ul << 14) - 1);
            sid            = (id >> 20) & ((1ul << 12) - 1);
            flagReserved2 = (id >> 32) & ((1ul << 1) - 1);
            flagReserved1 = (id >> 33) & ((1ul << 1) - 1);
            flagBpcStart = (id >> 34) & ((1ul << 1) - 1);
            flagBeamin    = (id >> 35) & ((1ul << 1) - 1);
            eventNo        = (id >> 36) & ((1ul << 12) - 1);
            gid            = (id >> 48) & ((1ul << 12) - 1);
            fid            = (id >> 60) & ((1ul <<  4) - 1);
            // param
            bpcts          = (param  >>  0) & ((1ul << 42) - 1);
            bpcid          = (param  >> 42) & ((1ul << 22) - 1);
        }

        [[nodiscard]] uint64_t id() const {
            // clang-format:off
            //       field             width        position
            return ((virtAcc & ((1ul <<  4) - 1)) <<  0)
                 + ((reqNoBeam + 0ul) << 4)
                 + ((reserved + 0ul)              <<  5)
                 + ((bpid    & ((1ul << 14) - 1)) <<  6)
                 + ((sid     & ((1ul << 12) - 1)) << 20)
                 + ((flagReserved2 + 0ul) << 32)
                 + ((flagReserved1 + 0ul) << 33)
                 + ((flagBpcStart + 0ul) << 34)
                 + ((flagBeamin + 0ul) << 35)
                 + ((eventNo & ((1ul << 12) - 1)) << 36)
                 + ((gid     & ((1ul << 12) - 1)) << 48)
                 + ((fid     & ((1ul <<  4) - 1)) << 60);
            // clang-format:on
        }

        [[nodiscard]] uint64_t param() const {
            // clang-format:off
            //       field             width        position
            return ((bpcts & ((1ul << 42) - 1)) <<  0)
                 + ((bpcid & ((1ul << 22) - 1)) << 42);
            // clang-format:on
        }
    };

    struct Trigger {
        std::array<bool, 20> outputs;
        uint64_t id;
        uint64_t delay; // [ms]
        uint64_t flattop; // [ms]

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
                    saftlib::wait_for_signal(static_cast<int>(std::clamp(duration, 50l, std::numeric_limits<int>::max()+0l)));
                } else {
                    break;
                }
            }
        }
    }

    void injectEvent(Event ev, uint64_t time_offset) {
        if (simulate && ((ev.id() | snoopMask) == (snoopID | snoopMask)) ) {
            this->snoop_writer.publish(
                    [ev, time_offset](std::span<Event> buffer) {
                        buffer[0] = ev;
                        buffer[0].time += time_offset;
                        buffer[0].executed = buffer[0].time;
                    }, 1);
        } else {
            receiver->InjectEvent(ev.id(), ev.param(), saftlib::makeTimeTAI(ev.time + time_offset));
        }
    }

    unsigned long getTAI() {
        if (simulate) {
            return static_cast<unsigned long>(std::max(0l, duration_cast<std::chrono::nanoseconds>(
                    std::chrono::utc_clock::now().time_since_epoch()).count()));
        }
        return receiver->CurrentTime().getTAI();
    }

    void updateTrigger(Trigger &trigger) {
        auto existing = triggers.find(trigger.id);
        if (existing != triggers.end() && existing->second == trigger) {
            return; // nothing changed
        }
        if (!simulate) {
            std::size_t i = 0;
            for (const auto &output : outputs) { // clang does not support `(auto [i, output, enabled] : std::views::zip(std::views::iota(0), outputs, trigger.outputs))`
                bool enabled = trigger.outputs[i++];
                if (enabled && (existing == triggers.end() || !existing->second.outputs[static_cast<unsigned long>(i)])) { // newly enabled
                    auto proxy = saftlib::Output_Proxy::create(std::get<2>(output));
                    proxy->NewCondition(true, trigger.id, std::numeric_limits<uint64_t>::max(), static_cast<int64_t>(trigger.delay) * milliToNano + minTriggerOffset, true);
                    proxy->NewCondition(true, trigger.id, std::numeric_limits<uint64_t>::max(), static_cast<int64_t>(trigger.delay + trigger.flattop) * milliToNano + minTriggerOffset, false);
                    auto [inserted, _] = triggers.insert({trigger.id, trigger});
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
