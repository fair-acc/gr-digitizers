// gr
#include <gnuradio-4.0/CircularBuffer.hpp>
// timing
#include <SAFTd.h>
#include <TimingReceiver.h>
#include <SoftwareActionSink.h>
#include <SoftwareCondition.h>
#include <CommonFunctions.h>
#include <etherbone.h>

// gr
#include <gnuradio-4.0/CircularBuffer.hpp>

using saftlib::SAFTd_Proxy;
using saftlib::TimingReceiver_Proxy;
using saftlib::SoftwareActionSink_Proxy;
using saftlib::SoftwareCondition_Proxy;

class Timing {
public:
    struct event {
        uint64_t id = 0x0; // full 64 bit EventID contained in the timing message
        uint64_t param  = 0x0; // full 64 bit parameter contained in the timing message
        uint64_t time = 0; // time for next event (this value is added to the current time or the next PPS, see option -p
        uint64_t executed = 0;
        uint16_t flags = 0x0;

        void print() const {
            // print everything out
            std::cout << "tDeadline: " << tr_formatDate(saftlib::makeTimeTAI(time), PMODE_HEX & PMODE_VERBOSE, false);
            // print event id
            auto fid   = ((id >> 60) & 0xf);   // 1
            auto gid   = ((id >> 48) & 0xfff); // 4
            auto evtno = ((id >> 36) & 0xfff); // 4
            if (fid == 0) { //full << " FLAGS: N/A";
                auto flgs = 0;
                auto sid   = ((id >> 24) & 0xfff);  // 4
                auto bpid  = ((id >> 10) & 0x3fff); // 5
                auto res   = (id & 0x3ff);          // 4
                std::cout << std::format("FID={:#01X}, GID={:#04X}, EVTNO={:#04X}, FLAGS={:#01X}, SID={:#04X}, BPID={:#05X}, RES={:#04X}, id={:#016X}",
                                         fid, gid, evtno, flgs, sid, bpid, res, id);
            } else if (fid == 1) {
                auto flgs = ((id >> 32) & 0xf);   // 1
                auto bpc   = ((id >> 34) & 0x1);   // 1
                auto sid   = ((id >> 20) & 0xfff); // 4
                auto bpid  = ((id >> 6) & 0x3fff); // 5
                auto res   = (id & 0x3f);          // 4
                std::cout << std::format("FID={:#01X}, GID={:#04X}, EVTNO={:#04X}, FLAGS={:#01X}, BPC={:#01X}, SID={:#04X}, BPID={:#05X}, RES={:#04X}, id={:#016X}",
                                         fid, gid, evtno, flgs, bpc, sid, bpid, res, id);
            } else {
                auto other = (id & 0xfffffffff);   // 9
                std::cout << std::format("FID={:#01X}, GID={:#04X}, EVTNO={:#04X}, OTHER={:#09X}, id={:#016X}", fid, gid, evtno, other, id);
            }
            // print parameter
            std::cout << std::format(", Param={:#016X}", param);
            // print flags
            auto delay = executed - time;
            if (flags & 1) {
                std::cout << std::format(" !late (by {} ns)", delay);
            }
            if (flags & 2) {
                std::cout << std::format(" !early (by {} ns)", delay);
            }
            if (flags & 4) {
                std::cout << std::format(" !conflict (delayed by {} ns)", delay);
            }
            if (flags & 8) {
                std::cout << std::format(" !delayed (by {} ns)", delay);
            }
        }
    };
public:
    gr::CircularBuffer<event, 10000> snooped{10000};
    gr::CircularBuffer<event, 10000> to_inject{10000};
private:
    decltype(snooped.new_writer()) snoop_writer = snooped.new_writer();
    decltype(to_inject.new_reader()) to_inject_reader = to_inject.new_reader();
    bool initialized = false;
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
                    buffer[0].param = param;
                    buffer[0].id = id;
                    buffer[0].time = deadline.getTAI();
                    buffer[0].executed = executed.getTAI();
                    buffer[0].flags = flags;
                    buffer[0].print();
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
            receiver->InjectEvent(event.id, event.param, eventTime);
            event.print();
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
