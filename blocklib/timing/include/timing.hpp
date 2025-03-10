#ifndef GR_DIGITIZERS_TIMING_HPP
#define GR_DIGITIZERS_TIMING_HPP
#include <string_view>
// gr
#include <gnuradio-4.0/CircularBuffer.hpp>
// timing
#include <CommonFunctions.h>
#include <Input.hpp>
#include <Input_Proxy.hpp>
#include <Output.hpp>
#include <OutputCondition.hpp>
#include <OutputCondition_Proxy.hpp>
#include <Output_Proxy.hpp>
#include <SAFTd.h>
#include <SoftwareActionSink.h>
#include <SoftwareCondition.h>
#include <TimingReceiver.h>
#include <etherbone.h>

// gr
#include <gnuradio-4.0/CircularBuffer.hpp>

using saftlib::SAFTd_Proxy;
using saftlib::SoftwareActionSink_Proxy;
using saftlib::SoftwareCondition_Proxy;
using saftlib::TimingReceiver_Proxy;

static std::chrono::time_point<std::chrono::system_clock> taiNsToUtc(uint64_t input) { return std::chrono::utc_clock::to_sys(std::chrono::tai_clock::to_utc(std::chrono::tai_clock::time_point{} + std::chrono::nanoseconds(input) + std::chrono::years(12u))); }

class Timing {
public:
    static const int      milliToNano          = 1000000;
    static const int      minTriggerOffset     = 100;
    static const uint64_t ECA_EVENT_ID_LATCH   = 0xfffe000000000000ull; /* FID=MAX & GRPID=MAX-1 */
    static const uint64_t ECA_EVENT_MASK_LATCH = 0xfffe000000000000ull;
    static const uint64_t IO_CONDITION_OFFSET  = 5000ull;
    /**
     * Structure to atuomatically encode and decode GSI/FAIR specific timing events as documented in:
     * https://www-acc.gsi.de/wiki/Timing/TimingSystemEvent
     *
     * The Timing system transmits 256 bit of metadata per timing event consisting of the following high level fields:
     *  bits   |size| name                   | description
     * --------|----|------------------------|-------------------------------------------------------------------
     * 000-063 | 64 | EventID                | contains application specific data, Timing Hardware performs a prefix match on this data
     * 064-127 | 64 | Param                  | application specific data
     * 128-159 | 32 | Reserved               | unused/not covered here
     * 160-191 | 32 | Timing Extension Field | used internally/not covered here
     * 192-255 | 64 | Timestamp              | time in nanoseconds since the origin of the TAI epoch
     *
     * At GSI/FAIR the EventID and Param fields are split into different fields:
     *
     * EventID:
     * bitsBE |bitsLE |size| name         | description
     *  ------|-------|----|--------------|-------------------------------------------------------------------
     *  00-03 | 60-63 |  4 | FID          | Format ID: always equals 0x1
     *  04-15 | 48-59 | 12 | GID          | Timing group ID: see event_definitions.hpp for the existing timing groups
     *  16-27 | 36-47 | 12 | EVENTNO      | Event Number: determines the type of event e.g CME_BP_START, see event_definitions.hpp
     *  28-31 | 32-35 |  4 | FLAGS        |
     *     28 |    35 |  1 |   BEAM-IN    |
     *     29 |    34 |  1 |   BPC-START  | First event in a new Beam Production Chain
     *     30 |    33 |  1 |   RESERVED 1 |
     *     31 |    32 |  1 |   RESERVED 2 |
     *  32-43 | 20-31 | 12 | SID          | Sequence ID
     *  44-57 |  6-19 | 14 | BPID         | Beam Process ID
     *  58-63 |  0- 5 |  6 | reserved     |
     *     58 |     5 |  1 |   reserved   |
     *     59 |     4 |  1 |   ReqNoBeam  |
     *  40-63 |  0- 3 |  4 |   VirtAcc    | Virtual Accelerator
     *
     * BigEndian    0    4            16           28   32           44                 58
     * LittleEndian   60           48           36   20           20              6      0
     *           0x f    f   f   f    f   f   f    f    f   f   f    f   f   f   8  8 f
     *           0b 1111 111111111111 111111111111 1111 111111111111 11111111111111 111111
     * size [bits]   4        12           12       4       12             14         6
     *              FID       GID        EVENTNO   FLAGS    SID           BPID      reserved
     *
     * param:
     * bitsBE|bitsLE |size| name         | description
     * ------|-------|----|--------------|-------------------------------------------------------------------
     *  0-21 | 42-63 | 22 | BPCID        | Beam Production Chain ID
     * 22-63 |  0-41 | 42 | BPCTS        | Beam Production Chain Timestamp
     *
     * BigEndian     0                      22                                      63
     * LittleEndian  63                  44                                          0
     * 0x            fffff8                 8ffffffffff
     * 0b            1111111111111111111111 111111111111111111111111111111111111111111
     * size [bits]            22                              42
     *               BPCID                  BPCTS
     *
     * The (BPCID, SID, BPID, GID) tuple corresponds to the timing selector (`FAIR.SELECTOR.C=__:S=__;P=__:T=__`)
     * described in https://github.com/fair-acc/opencmw-cpp/blob/main/assets/F-CS-SIS-en-B_0006_FAIR_Service_Middle_Ware_V1_0.pdf
     *
     * The additional fields `executed` and `flags` do not correspond to fields in the timing message but are used to
     * store local meta-information on whether the event was processed in time.
     */
    struct Event {
        // eventid - 64
        uint8_t  fid = 1; // 4
        uint16_t gid;     // 12
        uint16_t eventNo; // 12
        bool     flagBeamin{};
        bool     flagBpcStart{};
        bool     flagReserved1{};
        bool     flagReserved2{};
        uint16_t sid{}; // 12
        uint16_t bpid;  // 14
        bool     reserved{};
        bool     reqNoBeam{};
        uint8_t  virtAcc{}; // 4
        // param - 64
        uint32_t bpcid{}; // 22
        uint64_t bpcts{}; // 42
        // timing system
        uint64_t time     = 0;
        uint64_t executed = 0;
        uint16_t flags    = 0x0;
        bool     isIo     = false;

        Event(const Event&)            = default;
        Event(Event&&)                 = default;
        Event& operator=(const Event&) = default;
        Event& operator=(Event&&)      = default;

        template<typename ReturnType, std::size_t position, std::size_t bitsize>
        static constexpr ReturnType extractField(uint64_t value) {
            static_assert(position + bitsize <= 64);                                         // assert that we only consider existing bits
            static_assert(std::numeric_limits<ReturnType>::max() >= ((1UL << bitsize) - 1)); // make sure the data fits into the return type
            return static_cast<ReturnType>((value >> position) & ((1UL << bitsize) - 1));
        }

        template<std::size_t position, std::size_t bitsize, typename FieldType>
        static constexpr uint64_t fromField(FieldType value) {
            static_assert(position + bitsize <= 64);
            static_assert(std::numeric_limits<FieldType>::max() >= ((1UL << bitsize) - 1));
            return ((value & ((1UL << bitsize) - 1)) << position);
        }

        explicit Event(uint64_t timestamp = 0, uint64_t id = 1UL << 60, uint64_t param = 0, uint16_t _flags = 0, uint64_t _executed = 0, bool _isIo = false)
            : // id
              fid{extractField<uint8_t, 60, 4>(id)}, gid{extractField<uint16_t, 48, 12>(id)}, eventNo{extractField<uint16_t, 36, 12>(id)}, flagBeamin{extractField<bool, 35, 1>(id)}, flagBpcStart{extractField<bool, 34, 1>(id)}, flagReserved1{extractField<bool, 33, 1>(id)}, flagReserved2{extractField<bool, 32, 1>(id)}, sid{extractField<uint16_t, 20, 12>(id)}, bpid{extractField<uint16_t, 6, 14>(id)}, reserved{extractField<bool, 5, 1>(id)}, reqNoBeam{extractField<bool, 4, 1>(id)}, virtAcc{extractField<uint8_t, 0, 4>(id)},
              // param
              bpcid{extractField<uint32_t, 42, 22>(param)}, bpcts{extractField<uint64_t, 0, 42>(param)}, time{timestamp}, executed{_executed}, flags{_flags}, isIo{_isIo} {}

        struct fromGidEventnoBpidTag {};
        Event(const fromGidEventnoBpidTag, const uint16_t _gid, const uint16_t _eventNo, const uint16_t _bpid) : gid{_gid}, eventNo{_eventNo}, bpid{_bpid} {}

        [[nodiscard]] uint64_t id() const {
            // clang-format:off
            return fromField<0, 4>(virtAcc) + fromField<4, 1>(reqNoBeam) + fromField<5, 1>(reserved) + fromField<6, 14>(bpid) + fromField<20, 12>(sid) + fromField<32, 1>(flagReserved2) + fromField<33, 1>(flagReserved1) + fromField<34, 1>(flagBpcStart) + fromField<35, 1>(flagBeamin) + fromField<36, 12>(eventNo) + fromField<48, 12>(gid) + fromField<60, 4>(fid);
            // clang-format:on
        }

        [[nodiscard]] uint64_t param() const { return fromField<0, 42>(bpcts) + fromField<42, 22>(bpcid); }

        static std::optional<Event> fromString(std::string_view line) {
            using std::operator""sv;
#if defined(__clang__)
            std::array<uint64_t, 3> elements{};
            std::size_t             found         = 0;
            std::size_t             startingIndex = 0;
            for (std::size_t i = 0; i <= line.size() && found < elements.size(); i++) {
                if (i == line.size() || line[i] == ' ') {
                    if (startingIndex < i) {
                        auto parse        = [](auto el) { return std::stoul(std::string(std::string_view(el)), nullptr, 0); };
                        elements[found++] = parse(line.substr(startingIndex, i - startingIndex));
                    }
                    startingIndex = i;
                }
            }
            if (found >= 3) {
                return Event{elements[2], elements[0], elements[1]};
            }
#else
            auto event = std::views::split(line, " "sv) | std::views::take(3) | std::views::transform([](auto n) { return std::stoul(std::string(std::string_view(n)), nullptr, 0); }) | std::views::adjacent_transform<3>([](auto a, auto b, auto c) { return Timing::Event{c, a, b}; });
            if (!event.empty()) {
                return event.front();
            }
#endif
            return {};
        }

        static void loadEventsFromString(std::vector<Timing::Event>& events, std::string_view string) {
            events.clear();
            using std::operator""sv;
            try {
                for (const auto line : std::views::split(string, "\n"sv)) {
                    auto event = Timing::Event::fromString(std::string_view{line});
                    if (event) {
                        events.push_back(*event);
                    }
                }
            } catch (std::invalid_argument& e) {
                events.clear();
                fmt::print("Error parsing data, cannot convert string to number: {}\n### data ###\n{}\n### data end ###\n", e.what(), string);
            } catch (std::out_of_range& e) {
                events.clear();
                fmt::print("Error parsing data, value out of range: {}\n### data ###\n{}\n### data end ###\n", e.what(), string);
            }
        }
    };

    struct Trigger {
        std::array<bool, 20> outputs;
        uint64_t             id;
        double               delay;   // [ms]
        double               flattop; // [ms]

        bool operator<=>(const Trigger&) const = default;
    };

    gr::CircularBuffer<Event, 8192>                         snooped{8192};
    std::vector<std::tuple<uint, std::string, std::string>> outputs;
    std::map<uint64_t, Trigger>                             triggers;
    std::vector<Timing::Event>                              events = {};
    saftbus::SignalGroup                                    saftSigGroup;
    std::shared_ptr<SoftwareActionSink_Proxy>               sink;
    decltype(snooped.new_writer())                          snoop_writer = snooped.new_writer();

private:
    bool                                     tried = false;
    std::shared_ptr<SAFTd_Proxy>             saftd;
    std::shared_ptr<SoftwareCondition_Proxy> condition;
    std::shared_ptr<SoftwareCondition_Proxy> ioCondition;
    std::map<uint64_t, std::string>          map_PrefixName; /* Translation table IO name <> prefix */

public:
    bool                                  initialized = false;
    bool                                  simulate    = false;
    uint64_t                              snoopID     = 0x0;
    uint64_t                              snoopMask   = 0x0;
    std::shared_ptr<TimingReceiver_Proxy> receiver;
    std::string                           saftAppName = "gr_timing_example";
    std::string                           deviceName;

private:
    void updateExistingTrigger(const Trigger& trigger, const std::map<uint64_t, Timing::Trigger>::iterator& existing, const std::string& output) {
        auto proxy = saftlib::Output_Proxy::create(output, saftSigGroup);
        if (trigger.delay != existing->second.delay || trigger.flattop != existing->second.flattop) { // update condition for rising edge
            auto matchingConditions = proxy->getAllConditions() | std::views::transform([this](const auto& cond) { return saftlib::OutputCondition_Proxy::create(cond, saftSigGroup); }) | std::views::filter([&trigger](const auto& cond) { return cond->getID() == trigger.id && cond->getMask() == std::numeric_limits<uint64_t>::max(); });
            std::ranges::for_each(matchingConditions, [&trigger](const auto& cond) {
                if (cond->getOn()) {
                    cond->setOffset(static_cast<int64_t>(trigger.delay) * milliToNano + minTriggerOffset);
                } else {
                    cond->setOffset(static_cast<int64_t>(trigger.delay + trigger.flattop) * milliToNano + minTriggerOffset);
                }
            });
        }
    }

    void removeHardwareTrigger(const Trigger& trigger, const std::string& output) {
        auto proxy              = saftlib::Output_Proxy::create(output, saftSigGroup);
        auto matchingConditions = proxy->getAllConditions() | std::views::transform([this](const auto& cond) { return saftlib::OutputCondition_Proxy::create(cond, saftSigGroup); }) | std::views::filter([&trigger](const auto& cond) { return cond->getID() == trigger.id && cond->getMask() == std::numeric_limits<uint64_t>::max(); });
        std::ranges::for_each(matchingConditions, [](const auto& cond) { cond->Destroy(); });
    }

    void newHardwareTrigger(const Trigger& trigger, const std::string& output) {
        auto proxy = saftlib::Output_Proxy::create(output, saftSigGroup);
        proxy->NewCondition(true, trigger.id, std::numeric_limits<uint64_t>::max(), static_cast<int64_t>(trigger.delay) * milliToNano + minTriggerOffset, true);
        proxy->NewCondition(true, trigger.id, std::numeric_limits<uint64_t>::max(), static_cast<int64_t>(trigger.delay + trigger.flattop) * milliToNano + minTriggerOffset, false);
    }

public:
    void updateSnoopFilter() {
        if (simulate) {
            return;
        }
        if (condition) {
            condition->Destroy();
        }
        condition = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 0), saftSigGroup);
        condition->setAcceptLate(true);
        condition->setAcceptEarly(true);
        condition->setAcceptConflict(true);
        condition->setAcceptDelayed(true);
        condition->SigAction.connect([this](uint64_t id, uint64_t param, const saftlib::Time& deadline, const saftlib::Time& executed, uint16_t flags) {
            auto data = this->snoop_writer.reserve<gr::SpanReleasePolicy::ProcessAll>(1);
            data[0]   = Timing::Event{deadline.getTAI(), id, param, flags, executed.getTAI()};
        });
        condition->setActive(true);
        // Digital IO ports
        if (!ioCondition) {
            for (auto& [ioName, ioAddress] : receiver->getInputs()) {
                uint64_t prefix        = ECA_EVENT_ID_LATCH + (map_PrefixName.size() << 1); // fixed prefix for software condition, rest of the bits identify IO, least significant bit is io level
                map_PrefixName[prefix] = ioName;
                auto input             = saftlib::Input_Proxy::create(ioAddress, saftSigGroup);
                input->setEventEnable(false);
                input->setEventPrefix(prefix);
                input->setEventEnable(true);
            }
            ioCondition = saftlib::SoftwareCondition_Proxy::create(sink->NewCondition(true, ECA_EVENT_ID_LATCH, ECA_EVENT_MASK_LATCH, 10000), saftSigGroup);
            ioCondition->SigAction.connect([this](uint64_t id, uint64_t param, const saftlib::Time& deadline, const saftlib::Time& executed, uint16_t flags) {
                auto data = this->snoop_writer.reserve<gr::SpanReleasePolicy::ProcessAll>(1);
                data[0]   = Timing::Event{deadline.getTAI(), id, param, flags, executed.getTAI(), true};
            });
        }
    }

    std::string idToIoName(const std::uint64_t id) {
        std::uint64_t filter = id & (0xffffffffffffffffull - 1);
        if (map_PrefixName.contains(filter)) {
            return map_PrefixName.at(filter);
        } else {
            return "UNKNOWN-IO";
        }
    }

    void initialize() {
        if (simulate) {
            initialized = true;
        } else {
            try {
                saftd = SAFTd_Proxy::create("/de/gsi/saftlib", saftSigGroup);
                // get a specific device
                std::map<std::string, std::string> devices = saftd->getDevices();
                if (deviceName.empty()) {
                    if (devices.empty()) {
                        std::cerr << "" << std::endl;
                        fmt::print("No devices attached to saftd, continuing with simulated timing\n");
                        simulate    = true;
                        initialized = true;
                        return;
                    }
                    receiver = TimingReceiver_Proxy::create(devices.begin()->second, saftSigGroup);
                } else {
                    if (!devices.contains(deviceName)) {
                        std::cerr << "" << std::endl;
                        fmt::print("Could not find device {}, continuing with simulated timing\n", deviceName);
                        simulate    = true;
                        initialized = true;
                        return;
                    }
                    receiver = TimingReceiver_Proxy::create(devices.at(deviceName), saftSigGroup);
                }
                sink = SoftwareActionSink_Proxy::create(receiver->NewSoftwareActionSink(saftAppName), saftSigGroup);
                updateSnoopFilter();
                for (const auto& [i, output] : receiver->getOutputs() | std::views::enumerate) {
                    const auto& [name, port] = output;
                    outputs.emplace_back(i, name, port);
                }
                initialized = true;
            } catch (saftbus::Error& e) {
                fmt::print("Error initializing saftbus client: {}\ncontinuing with simulated timing\n", e.what());
                simulate    = true;
                initialized = true;
                return;
            }
        }
    }

    void process() {
        if (!initialized && !tried) {
            tried = true;
            initialize();
        } else if (initialized && !simulate) {
            const auto startTime = std::chrono::system_clock::now();
            while (true) {
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(startTime - std::chrono::system_clock::now() + std::chrono::milliseconds(5)).count();
                if (duration > 0) {
                    saftSigGroup.wait_for_signal(static_cast<int>(std::clamp(duration, 50L, std::numeric_limits<int>::max() + 0L)));
                } else {
                    break;
                }
            }
        }
    }

    void injectEvent(const Event& ev, uint64_t time_offset) {
        if (simulate && ((ev.id() & snoopMask) == (snoopID & snoopMask))) {
            auto buffer = this->snoop_writer.reserve<gr::SpanReleasePolicy::ProcessAll>(1);
            buffer[0]   = ev;
            buffer[0].time += time_offset;
            buffer[0].executed = buffer[0].time;
        } else if (!simulate) {
            receiver->InjectEvent(ev.id(), ev.param(), saftlib::makeTimeTAI(ev.time + time_offset));
        }
    }

    unsigned long currentTimeTAI() {
        if (simulate) {
            return static_cast<unsigned long>(std::max(0L, duration_cast<std::chrono::nanoseconds>(std::chrono::utc_clock::now().time_since_epoch()).count()));
        }
        return receiver->CurrentTime().getTAI();
    }

    void updateTrigger(Trigger& trigger) {
        auto existing = triggers.find(trigger.id);
        if (existing != triggers.end() && existing->second == trigger) {
            return; // nothing changed
        }
        if (!simulate) {
#if defined(__clang__)
            std::size_t i = 0;
            for (const auto& output : outputs) {
                bool enabled = trigger.outputs[i++];
#else
            for (const auto [i, output, enabled] : std::views::zip(std::views::iota(0), outputs, trigger.outputs)) {
#endif
                if (enabled && (existing == triggers.end() || !existing->second.outputs[static_cast<unsigned long>(i)])) { // newly enabled
                    newHardwareTrigger(trigger, std::get<2>(output));
                    auto [inserted, _] = triggers.try_emplace(trigger.id, trigger);
                    existing           = inserted;
                } else if (!enabled && existing != triggers.end() && existing->second.outputs[static_cast<unsigned long>(i)]) { // newly disabled
                    removeHardwareTrigger(trigger, std::get<2>(output));
                } else if (existing != triggers.end()) {
                    updateExistingTrigger(trigger, existing, std::get<2>(output));
                }
            }
        }
        triggers.insert_or_assign(trigger.id, trigger);
    }
};

#endif
