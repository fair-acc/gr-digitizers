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

using saftlib::SAFTd_Proxy;
using saftlib::TimingReceiver_Proxy;
using saftlib::SoftwareActionSink_Proxy;
using saftlib::SoftwareCondition_Proxy;

class Timing {
public:
    struct event {
        // eventid - 64
        uint8_t fid = 1;   // 4
        uint16_t gid;  // 12
        uint16_t eventno; // 12
        bool flag_beamin;
        bool flag_bpc_start;
        bool flag_reserved1;
        bool flag_reserved2;
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

        event(const event&) = default;
        event(event&&) = default;
        event& operator=(const event&) = default;
        event& operator=(event&&) = default;

        explicit event(uint64_t timestamp = 0, uint64_t id = 1ul << 60, uint64_t param= 0, uint16_t _flags = 0, uint64_t _executed = 0) {
            time = timestamp;
            flags = _flags;
            executed = _executed;
            // id
            virtAcc        = (id >>  0) & ((1ul <<  4) - 1);
            reqNoBeam      = (id >> 4) & ((1ul << 1) - 1);
            reserved       = (id >>  5) & ((1ul <<  1) - 1);
            bpid           = (id >>  6) & ((1ul << 14) - 1);
            sid            = (id >> 20) & ((1ul << 12) - 1);
            flag_reserved2 = (id >> 32) & ((1ul <<  1) - 1);
            flag_reserved1 = (id >> 33) & ((1ul <<  1) - 1);
            flag_bpc_start = (id >> 34) & ((1ul <<  1) - 1);
            flag_beamin    = (id >> 35) & ((1ul <<  1) - 1);
            eventno        = (id >> 36) & ((1ul << 12) - 1);
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
                 + ((flag_reserved2 + 0ul)        << 32)
                 + ((flag_reserved1 + 0ul)        << 33)
                 + ((flag_bpc_start + 0ul)        << 34)
                 + ((flag_beamin + 0ul)           << 35)
                 + ((eventno & ((1ul << 12) - 1)) << 36)
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
        uint64_t delay;
        uint64_t flattop;
    };

    bool initialized = false;
public:
    gr::CircularBuffer<event, 10000> snooped{10000};
    std::vector<std::tuple<uint, std::string, std::string>> outputs;
    std::map<uint64_t, Trigger> triggers;
private:
    decltype(snooped.new_writer()) snoop_writer = snooped.new_writer();
    bool tried = false;
public:
    bool simulate = false;
    bool ppsAlign= false;
    bool absoluteTime = false;
    bool UTC = false;
    bool UTCleap = false;
    uint64_t snoopID     = 0x0;
    uint64_t snoopMask   = 0x0;
    int64_t  snoopOffset = 0x0;

    std::shared_ptr<SAFTd_Proxy> saftd;
    std::shared_ptr<TimingReceiver_Proxy> receiver;
    std::shared_ptr<SoftwareActionSink_Proxy> sink;
    std::shared_ptr<SoftwareCondition_Proxy> condition;

    void initialize() {
        // open connection to saftlib
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
                condition = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, snoopOffset));
                condition->setAcceptLate(true);
                condition->setAcceptEarly(true);
                condition->setAcceptConflict(true);
                condition->setAcceptDelayed(true);
                condition->SigAction.connect(
                        [this](uint64_t id, uint64_t param, saftlib::Time deadline, saftlib::Time executed,
                               uint16_t flags) {
                            this->snoop_writer.publish(
                                    [this, id, param, deadline, executed, flags](std::span<event> buffer) {
                                        buffer[0] = Timing::event{deadline.getTAI(), id, param, flags, executed.getTAI()};
                                    }, 1);
                        });
                condition->setActive(true);
                uint i = 0;
                for (auto &[name, port] : receiver->getOutputs()) {
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
        } else if (initialized) {
            if (!simulate) {
                snoop();
            }
        }
    }

    static void snoop() {
        const auto startTime = std::chrono::system_clock::now();
        while(true) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(startTime - std::chrono::system_clock::now() + std::chrono::milliseconds(5)).count();
            if (duration > 0) {
                saftlib::wait_for_signal(std::clamp(duration, std::numeric_limits<int>::max()+0l, 50l));
            } else {
                break;
            }
        }
    }

    void injectEvent(event ev, uint64_t time_offset) {
        if (simulate) {
            this->snoop_writer.publish(
                    [ev, time_offset](std::span<event> buffer) {
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
            return duration_cast<std::chrono::nanoseconds>(std::chrono::utc_clock::now().time_since_epoch()).count();
        }
        return receiver->CurrentTime().getTAI();
    }

    void updateTrigger(Trigger &trigger) {
        triggers.insert({trigger.id, trigger});
        // // get condition information
        // static std::vector<std::pair<std::string, std::string>> ioNames{};
        // std::vector<std::pair<std::string, std::shared_ptr<saftlib::OutputCondition_Proxy>>> conditions{};
        // if (!timing.simulate){
        //     for (auto &[name, port] : timing.receiver->getOutputs()) {
        //         if (std::find_if(ioNames.begin(), ioNames.end(), [&name](auto const &e) {return e.first == name;}) == ioNames.end()) {
        //             ioNames.emplace_back(name, port);
        //         }
        //         auto port_proxy = saftlib::Output_Proxy::create(port);
        //         for (auto & condition : port_proxy->getAllConditions()) {
        //             auto condition_proxy = saftlib::OutputCondition_Proxy::create(condition);
        //             conditions.emplace_back(name, std::move(condition_proxy));
        //         }
        //     }
        // }
        //% std::string condIO{};
        //% std::int64_t delay = 0, flattop = 0;
        //% std::array<std::shared_ptr<saftlib::OutputCondition_Proxy>, 2> trigger_conditions{};
        //% bool condition_changed = false;
        //% for (auto &[ioName, cond]: conditions) {
        //%     if ((ev.id() & cond->getMask()) == cond->getID()) {
        //%         if (condIO.empty()) {
        //%             condIO = ioName;
        //%             if (cond->getOn()) {
        //%                 delay = cond->getOffset();
        //%                 trigger_conditions[0] = cond;
        //%             } else {
        //%                 flattop = cond->getOffset();
        //%                 trigger_conditions[1] = cond;
        //%             }
        //%         } else if (condIO == ioName) {
        //%             if (cond->getOn() && !trigger_conditions[0]) {
        //%                 delay = cond->getOffset();
        //%                 flattop -= delay;
        //%                 trigger_conditions[0] = cond;
        //%                 break;
        //%             } else if (!trigger_conditions[1]) {
        //%                 flattop = cond->getOffset() - delay;
        //%                 trigger_conditions[1] = cond;
        //%                 break;
        //%             }
        //%         }
        //%     }
        //% }
        //% if (!condIO.empty()) {
        //%     int current = std::distance(ioNames.begin(),
        //%                                 std::find_if(ioNames.begin(),
        //%                                              ioNames.end(),
        //%                                              [&condIO](auto e) {
        //%                                                  return e.first ==
        //%                                                         condIO;
        //%                                              }));
        //%     current = current >= ioNames.size() ? 0 : current;
        //%     ImGui::Combo("IO", &current,
        //%                  [](void *data, int i, const char **out) -> bool {
        //%                      *out = ((std::vector<std::pair<std::string, std::string>> *) data)->at(
        //%                              i).first.c_str();
        //%                      return true;
        //%                  }, &ioNames, ioNames.size(), 5);
        //%     if (delay != trigger_conditions[0]->getOffset() ||
        //%         flattop != trigger_conditions[1]->getOffset()) {
        //%         //condition_changed = true;
        //%     }
        //%     if (ImGui::Button("delete") ||
        //%         ioNames[current].first != condIO) {
        //%         // todo: remove both conditions
        //%         trigger_conditions[0]->Destroy();
        //%         trigger_conditions[1]->Destroy();
        //%         trigger_conditions[0] = {};
        //%         trigger_conditions[1] = {};
        //%         if (ioNames[current].first != condIO) {
        //%             condition_changed = true;
        //%         }
        //%     }
        //% } else {
        //%     ImGui::SameLine();
        //%     if (ImGui::Button("Add Trigger")) {
        //%         auto proxy = saftlib::Output_Proxy::create(
        //%                 ioNames[0].second);
        //%         proxy->NewCondition(true, ev.id(),
        //%                             std::numeric_limits<uint64_t>::max(),
        //%                             default_offset,
        //%                             true);
        //%         proxy->NewCondition(true, ev.id(),
        //%                             std::numeric_limits<uint64_t>::max(),
        //%                             2 * default_offset,
        //%                             false);
        //%     }
        //% }
        //% if (condition_changed) {
        //%     auto proxy = saftlib::Output_Proxy::create(
        //%             ioNames[current].second);
        //%     if (trigger_conditions[0]) {
        //%         trigger_conditions[0]->setOffset(delay);
        //%     } else {
        //%         proxy->NewCondition(true, ev.id(),
        //%                             std::numeric_limits<uint64_t>::max(),
        //%                             delay, true);
        //%     }
        //%     if (trigger_conditions[1]) {
        //%         trigger_conditions[1]->setOffset(flattop);
        //%     } else {
        //%         proxy->NewCondition(true, ev.id(),
        //%                             std::numeric_limits<uint64_t>::max(),
        //%                             flattop + delay,
        //%                             false);
        //%     }
        //% }
    }
};
