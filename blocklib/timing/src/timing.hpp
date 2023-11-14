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
#include <Output.hpp>
#include <Input.hpp>
#include <OutputCondition.hpp>

// gr
#include <gnuradio-4.0/CircularBuffer.hpp>

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

    bool initialized = false;
public:
    gr::CircularBuffer<event, 10000> snooped{10000};
    gr::CircularBuffer<event, 10000> to_inject{10000};
private:
    decltype(snooped.new_writer()) snoop_writer = snooped.new_writer();
    decltype(to_inject.new_reader()) to_inject_reader = to_inject.new_reader();
    bool tried = false;
public:
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
            condition->SigAction.connect([this](uint64_t id, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags) {
                this->snoop_writer.publish([this, id, param, deadline, executed, flags](std::span<event> buffer) {
                    buffer[0] = Timing::event{deadline.getTAI(), id, param, flags, executed.getTAI()};
                }, 1);
            });
            condition->setActive(true);
            initialized = true;
        } catch (...) {}
    }

    void process() {
        if (!initialized && !tried) {
            tried = true;
            initialize();
        } else if (initialized) {
            inject();
            snoop();
        }
    }

    void inject() {
        for (auto &event: to_inject_reader.get()) {
            const saftlib::Time wrTime = receiver->CurrentTime(); // current WR time
            saftlib::Time eventTime; // time for next event in PTP time
            if (ppsAlign) {
                const saftlib::Time ppsNext   = (wrTime - (wrTime.getTAI() % 1000000000)) + 1000000000; // time for next PPS
                eventTime = (ppsNext + event.time);
            } else if (absoluteTime) {
                if (UTC) {
                    eventTime = saftlib::makeTimeUTC(event.time, UTCleap);
                } else {
                    eventTime = saftlib::makeTimeTAI(event.time);
                }
            } else {
                eventTime = wrTime + event.time;
            }
            receiver->InjectEvent(event.id(), event.param(), eventTime);
        }
    }

    void snoop() {
        const auto startTime = std::chrono::system_clock::now();
        while(true) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(startTime - std::chrono::system_clock::now() + std::chrono::milliseconds(5)).count();
            if (duration > 0) {
                saftlib::wait_for_signal( duration < 50 ? 50 : (duration >= std::numeric_limits<int>::max() ? std::numeric_limits<int>::max() : duration));
            } else {
                break;
            }
        }
    }
};

class WBConsole {
    // hardware address of the timing receiver on the wishbone bus
    static constexpr unsigned int CERN_ID = 0xce42;
    static constexpr unsigned int UART_ID = 0xe2d13d04;
    static constexpr unsigned int VUART_TX = 0x10;
    static constexpr unsigned int VUART_RX = 0x14;
    sdb_device sdbDevice{};
    eb_socket_t socket{};
    eb_device_t device{};
    eb_address_t tx{}, rx{};

    gr::CircularBuffer<char, 10000> input{10000};
    decltype(input.new_writer()) writer = input.new_writer();
    gr::CircularBuffer<char, 10000> output{10000};
    decltype(output.new_reader()) reader = output.new_reader();

    bool initialized = false;
    bool tried = false;

public:
    void initialize() {
        if (eb_status_t status{}; (status = eb_socket_open(EB_ABI_CODE, 0, EB_DATAX|EB_ADDRX, &socket)) != EB_OK) {
            fmt::print("eb_socket_open failed: {}", status);
            //throw std::logic_error("failed to open socket");
            return;
        }

        if (eb_status_t status{}; (status = eb_device_open(socket, "dev/wbm0", EB_DATAX|EB_ADDRX, 3, &device)) != EB_OK) {
            fmt::print("failed to open etherbone device: {} status: {}", "dev/wbm0", status);
            //throw std::logic_error("failed to open device");
            return;
        }

        std::array<sdb_device, 1> sdb{};
        int c = sdb.size();
        if (eb_status_t status{}; (status = eb_sdb_find_by_identity(device, CERN_ID, UART_ID, sdb.data(), &c)) != EB_OK) {
            fmt::print("eb_sdb_find_by_identity", status);
            //throw std::logic_error("failed to find by identity");
            return;
        }

        if (c > 1) {
            fprintf(stderr, "found %d UARTs on that device; pick one with -i #\n", c);
            //throw std::logic_error("uart name not ambiguous");
            return;
        } else if (c < 1) {
            fprintf(stderr, "could not find UART #%d on that device (%d total)\n", 1, c);
            //throw std::logic_error("want to use nonexisting uart");
            return;
        }

        printf("Connected to uart at address %" PRIx64"\n", sdb[0].sdb_component.addr_first);
        tx = sdb[0].sdb_component.addr_first + VUART_TX;
        rx = sdb[0].sdb_component.addr_first + VUART_RX;
        initialized = true;
    }

    ~WBConsole() {
        if (initialized) {
            eb_device_close(device);
            eb_socket_close(socket);
        }
    }

    void process() {
        if (!initialized && !tried) {
            tried = true;
            initialize();
        } else if (initialized) {
            std::array<eb_data_t, 200> rx_data{};
            bool busy = false;
            eb_data_t done{};
            eb_cycle_t cycle{};
            /* Poll for status */
            eb_cycle_open(device, nullptr, eb_block, &cycle);
            eb_cycle_read(cycle, rx, EB_BIG_ENDIAN|EB_DATA32, &rx_data[0]);
            eb_cycle_read(cycle, tx, EB_BIG_ENDIAN|EB_DATA32, &done);
            eb_cycle_close(cycle);

            /* Bulk read anything extra */
            if ((rx_data[0] & 0x100) != 0) {
                eb_cycle_open(device, nullptr, eb_block, &cycle);
                for (uint i = 1; i < rx_data.size(); ++i) {
                    eb_cycle_read(cycle, rx, EB_BIG_ENDIAN|EB_DATA32, &rx_data[i]);
                }
                eb_cycle_close(cycle);

                for (unsigned long data : rx_data) {
                    if ((data & 0x100) == 0) {
                        continue;
                    }
                    char byte = data & 0xFF;
                    // copy received byte to ringbuffer
                    output.new_writer().publish([byte](std::span<char> buffer) {buffer[0] = byte;}, 1);
                }
            }

            busy = busy && (done & 0x100) == 0;
            if (!busy) {
                eb_cycle_open(device, nullptr, eb_block, &cycle);
                for (auto tx_data : reader.get()) {
                    eb_device_write(device, tx, EB_BIG_ENDIAN|EB_DATA32, static_cast<eb_data_t>(tx_data), eb_block, nullptr);
                }
                eb_cycle_close(cycle);
                busy = true;
                eb_cycle_close(cycle);
            }
        }
    }

    void send_command(std::string_view cmd) {
        output.new_writer().publish([&cmd](std::span<char> buf) { std::copy(cmd.begin(), cmd.end(), buf.begin()); }, cmd.size());
    }

    void setMasterMode(bool enable) {
        send_command(enable ? "mode master" : "mode slave");
    }
};
