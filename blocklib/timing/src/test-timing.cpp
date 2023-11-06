#include <format>
#include <thread>
#include <iostream>
#include <cstdio>

// CLI - interface
#include <CLI/CLI.hpp>

// UI
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>

// timing
#include <SAFTd.h>
#include <TimingReceiver.h>
#include <SoftwareActionSink.h>
#include <SoftwareCondition.h>
#include <CommonFunctions.h>
#include <etherbone.h>

// gr
#include <gnuradio-4.0/CircularBuffer.hpp>

// picoscope
#include <ps4000aApi.h>

using saftlib::SAFTd_Proxy;
using saftlib::TimingReceiver_Proxy;
using saftlib::SoftwareActionSink_Proxy;
using saftlib::SoftwareCondition_Proxy;

#define OCTO_SCOPE		8

class Ps4000a {
    typedef enum {
        MODEL_NONE = 0,
        MODEL_PS4824 = 0x12d8,
    } MODEL_TYPE;

    typedef struct {
        int16_t DCcoupled;
        int16_t range;
        int16_t enabled;
        float analogueOffset;
    }CHANNEL_SETTINGS;

    typedef enum {
        SIGGEN_NONE = 0,
        SIGGEN_FUNCTGEN = 1,
        SIGGEN_AWG = 2
    } SIGGEN_TYPE;

    typedef struct {
        int16_t				handle;
        MODEL_TYPE			model;
        int8_t				modelString[8];
        int8_t				serial[11];
        int16_t				complete;
        int16_t				openStatus;
        int16_t				openProgress;
        PS4000A_RANGE		firstRange;
        PS4000A_RANGE		lastRange;
        int16_t				channelCount;
        int16_t				maxADCValue;
        SIGGEN_TYPE			sigGen;
        int16_t				hasETS;
        uint16_t			AWGFileSize;
        CHANNEL_SETTINGS	channelSettings[PS4000A_MAX_CHANNELS];
        uint16_t			hasFlexibleResolution;
        uint16_t			hasIntelligentProbes;
    }UNIT;

    typedef struct tBufferInfo {
        using buftype = gr::CircularBuffer<char, 10000>;
        UNIT *unit;
        std::array<std::array<int16_t, 20000>, 8> driverBuffers{};
        std::array<buftype, 8> data{ buftype{0} ,buftype{0},buftype{0},buftype{0},buftype{0},buftype{0},buftype{0},buftype{0}};
        std::array<decltype(data[0].new_writer()), 8> writers = {
                data[0].new_writer(),data[1].new_writer(),data[2].new_writer(),data[3].new_writer(),
                data[4].new_writer(),data[5].new_writer(), data[6].new_writer(),data[7].new_writer()};
    } BUFFER_INFO;

    const uint32_t	bufferLength = 100000;
    PICO_STATUS	status = PICO_OK;
    int16_t    	g_ready = false;
    int32_t     g_sampleCount = 0;
    uint32_t	g_startIndex;
    int16_t		g_autoStop;
    int16_t		g_trig = 0;
    uint32_t	g_trigAt = 0;
    static constexpr uint32_t inputRanges[] = { 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000 };

    BUFFER_INFO bufferInfo;
    UNIT digitizer;


    void SetDefaults(UNIT *unit) {
        for (int32_t ch = 0; ch < unit->channelCount; ch++) {
            status = ps4000aSetChannel(unit->handle,																		// Handle to select the correct device
                                       (PS4000A_CHANNEL)(PS4000A_CHANNEL_A + ch),											// channel
                                       unit->channelSettings[PS4000A_CHANNEL_A + ch].enabled,								// If channel is enabled or not
                                       (PS4000A_COUPLING)unit->channelSettings[PS4000A_CHANNEL_A + ch].DCcoupled,			// If AC or DC coupling
                                       (PICO_CONNECT_PROBE_RANGE) unit->channelSettings[PS4000A_CHANNEL_A + ch].range,		// The voltage scale of the channel
                                       unit->channelSettings[PS4000A_CHANNEL_A + ch].analogueOffset);						// Analogue offset
            printf(status ? "SetDefaults:ps4000aSetChannel------ 0x%08lx for channel %i\n" : "", status, ch);
        }
    }

    PICO_STATUS open_digitizer(UNIT *unit) {
        // Can call this function multiple times to open multiple devices
        // Note that the unit.handle is specific to each device make sure you don't overwrite them
        fmt::print("opening unit\n");
        status = ps4000aOpenUnit(&(unit->handle), nullptr);
        fmt::print("=> status: {:#x}\n", status);
        if (unit->handle == 0) {
            return status;
        }

        // Check status code returned for power status codes
        switch (status) {
            case PICO_OK: // No need to change power source
                break;
            case PICO_POWER_SUPPLY_NOT_CONNECTED:
                fmt::print("Warning: running on usb power!\n");
                status = ps4000aChangePowerSource(unit->handle, PICO_POWER_SUPPLY_NOT_CONNECTED);		// Tell the driver that's ok
            case PICO_USB3_0_DEVICE_NON_USB3_0_PORT:	// User must acknowledge they want to power via USB
                fmt::print("Warning: Connected to non usb 3 port!\n");
                status = ps4000aChangePowerSource(unit->handle, PICO_USB3_0_DEVICE_NON_USB3_0_PORT);		// Tell the driver that's ok
            default:
                return status;
        }

        // Device only has these min and max values
        unit->firstRange = PS4000A_10MV;
        unit->lastRange = PS4000A_50V;

        fmt::print("get info\n");
        std::string_view description[11] = { "Driver Version", "USB Version", "Hardware Version", "Variant Info", "Serial", "Cal Date",
                                             "Kernel Version", "Digital HW Version", "Analogue HW Version", "Firmware 1", "Firmware 2" };
        int16_t requiredSize;
        int32_t variant;
        char line[80] = { 0 };
        for (int i = 0; i <= 10; i++) {
            status = ps4000aGetUnitInfo(unit->handle, (int8_t *) line, sizeof(line), &requiredSize, i);
            if (i == PICO_VARIANT_INFO) { // info = 3 - PICO_VARIANT_INFO
                variant = atoi(line);
                memcpy(&(unit->modelString), line, sizeof(unit->modelString) == 5 ? 5 : sizeof(unit->modelString));
            } else if (i == PICO_BATCH_AND_SERIAL)	{ // info = 4 - PICO_BATCH_AND_SERIAL
                memcpy(&(unit->serial), line, static_cast<size_t>(requiredSize));
            }
            fmt::print("{}: {}\n", description[i], std::string_view(line, static_cast<unsigned long>(requiredSize)));
        }
        printf("\n");

        // Find the maxiumum AWG buffer size
        int16_t minArbitraryWaveformValue = 0;
        int16_t maxArbitraryWaveformValue = 0;
        uint32_t minArbitraryWaveformBufferSize = 0;
        uint32_t maxArbitraryWaveformBufferSize = 0;
        fmt::print("get siggen buffer size\n");
        status = ps4000aSigGenArbitraryMinMaxValues(unit->handle, &minArbitraryWaveformValue, &maxArbitraryWaveformValue, &minArbitraryWaveformBufferSize, &maxArbitraryWaveformBufferSize);
        switch (variant) {
            case MODEL_PS4824:
                unit->model = MODEL_PS4824;
                unit->sigGen = SIGGEN_AWG;
                unit->firstRange = PS4000A_10MV;
                unit->lastRange = PS4000A_50V;
                unit->channelCount = OCTO_SCOPE;
                unit->hasETS = false;
                unit->AWGFileSize = maxArbitraryWaveformBufferSize;
                unit->hasFlexibleResolution = 0;
                unit->hasIntelligentProbes = 0;
                break;
            default:
                unit->model = MODEL_NONE;
                break;
        }
        for (int ch = 0; ch < unit->channelCount; ch++) {
            unit->channelSettings[ch].enabled = (ch == 0);
            unit->channelSettings[ch].DCcoupled = true;
            unit->channelSettings[ch].range = PS4000A_5V;
            unit->channelSettings[ch].analogueOffset = 0.0f;
        }
        ps4000aMaximumValue(unit->handle, &unit->maxADCValue);
        SetDefaults(unit);
        return status;
    }

    int32_t adc_to_mv(int32_t raw, int32_t rangeIndex, UNIT * unit) {
        return (raw * (int32_t) inputRanges[rangeIndex]) / unit->maxADCValue;
    }

    void StreamDataHandler(UNIT * unit) {
        int16_t * buffers[PS4000A_MAX_CHANNEL_BUFFERS];
        int16_t * appBuffers[PS4000A_MAX_CHANNEL_BUFFERS];

        PICO_STATUS status;

        // Setup data and temporary application buffers to copy data into
        for (std::size_t i = 0; i < unit->channelCount; i++) {
            if (unit->channelSettings[PS4000A_CHANNEL_A + i].enabled) {
                status = ps4000aSetDataBuffer(unit->handle, (PS4000A_CHANNEL)i, bufferInfo.driverBuffers[i].data(), bufferInfo.driverBuffers[0].size(), 0, PS4000A_RATIO_MODE_NONE);
            }
        }
        uint32_t downsampleRatio = 1;
        PS4000A_TIME_UNITS timeUnits = PS4000A_US;
        uint32_t sampleInterval = 1000; // 1/1000 us = 1000Hz
        PS4000A_RATIO_MODE ratioMode = PS4000A_RATIO_MODE_NONE;
        uint32_t preTrigger = 0;
        uint32_t postTrigger = 5000; //1000000; => 5s
        int16_t autostop = true;

        bufferInfo.unit = unit;

        printf("\nStreaming Data for %lu samples", postTrigger / downsampleRatio);
        printf("Collect streaming...\n");

        g_autoStop = false;
        do {
            // For streaming we use sample interval rather than timebase used in ps4000aRunBlock
            status = ps4000aRunStreaming(unit->handle, &sampleInterval, timeUnits, preTrigger, postTrigger, autostop, downsampleRatio, ratioMode, bufferLength);
            if (status != PICO_OK) {
                if (status == PICO_POWER_SUPPLY_CONNECTED || status == PICO_POWER_SUPPLY_NOT_CONNECTED || status == PICO_POWER_SUPPLY_UNDERVOLTAGE) {
                    status = ps4000aChangePowerSource(unit->handle, status);
                } else {
                    printf("StreamDataHandler:ps4000aRunStreaming ------ 0x%08lx \n", status);
                    return;
                }
            }
        } while (status != PICO_OK);

        int32_t totalSamples = 0;
        uint32_t triggeredAt = 0;
        while (!g_autoStop) { /* Poll until data is received. Until then, ps4000aGetStreamingLatestValues() wont call the callback */
            sleep(0);
            g_ready = false;
            status = ps4000aGetStreamingLatestValues(unit->handle, [](int16_t handle, int32_t noOfSamples, uint32_t startIndex, int16_t overflow, uint32_t triggerAt, int16_t triggered, int16_t autoStop, void * pParameter ) {
                auto *pPs4000A = (Ps4000a*) pParameter;

                // Used for streaming
                pPs4000A->g_sampleCount = noOfSamples;
                pPs4000A->g_startIndex = startIndex;
                pPs4000A->g_autoStop = autoStop;

                // Flag to say done reading data
                pPs4000A->g_ready = true;

                // Flags to show if & where a trigger has occurred
                pPs4000A->g_trig = triggered;
                pPs4000A->g_trigAt = triggerAt;

                if (noOfSamples) {
                    for (int channel = 0; channel < pPs4000A->bufferInfo.unit->channelCount; channel++) {
                        if (pPs4000A->bufferInfo.unit->channelSettings[channel].enabled) {
                            if (pPs4000A->bufferInfo.appBuffers && pPs4000A->bufferInfo.driverBuffers) {
                                if (pPs4000A->bufferInfo.appBuffers[channel * 2] && pPs4000A->bufferInfo.driverBuffers[channel * 2]) {
                                    std::memcpy(&pPs4000A->bufferInfo.appBuffers[channel * 2][0], &pPs4000A->bufferInfo.driverBuffers[channel * 2][startIndex], noOfSamples * sizeof(int16_t));
                                }
                                // Min buffers
                                if (pPs4000A->bufferInfo.appBuffers[channel * 2 + 1] && pPs4000A->bufferInfo.driverBuffers[1]) {
                                    std::memcpy(&pPs4000A->bufferInfo.appBuffers[channel * 2 + 1][0], &pPs4000A->bufferInfo.driverBuffers[channel * 2 + 1][startIndex], noOfSamples * sizeof(int16_t));
                                }
                            }
                        }
                    }
                }
            }, this);
            if (status == PICO_OK && g_ready && g_sampleCount > 0) /* can be ready and have no data, if autoStop has fired */ {
                if (g_trig) {
                    triggeredAt = totalSamples + g_trigAt;		// Calculate where the trigger occurred in the total samples collected
                }
                totalSamples += g_sampleCount;
                printf("\nCollected %3li samples, index = %5lu, Total: %7d samples", g_sampleCount, g_startIndex, totalSamples);
                if (g_trig) {
                    printf("Trig. at index %lu total %lu", g_trigAt, triggeredAt);	// Show where trigger occurred
                }
                for (int i = 0; i < g_sampleCount; i++) {
                    for (int j = 0; j < unit->channelCount; j++) {
                        if (unit->channelSettings[j].enabled) {
                            fmt::print("Ch{:c}  {:7d} = {:7d}mV, {:7d} = {:7d}mV\n",
                                       (char)('A' + j),
                                       appBuffers[j * 2][i],
                                       adc_to_mv(appBuffers[j * 2][i], unit->channelSettings[PS4000A_CHANNEL_A + j].range, unit),
                                       appBuffers[j * 2 + 1][i],
                                       adc_to_mv(appBuffers[j * 2 + 1][i], unit->channelSettings[PS4000A_CHANNEL_A + j].range, unit));
                        }
                    }
                }
            }
        }
        ps4000aStop(unit->handle);

        if (!g_autoStop) {
            printf("\nData collection aborted\n");
        } else {
            printf("\nData collection complete.\n\n");
        }

        for (int i = 0; i < unit->channelCount; i++) {
            if (unit->channelSettings[i].enabled) {
                free(buffers[i * 2]);
                free(appBuffers[i * 2]);
                free(buffers[i * 2 + 1]);
                free(appBuffers[i * 2 + 1]);
                if ((status = ps4000aSetDataBuffers(unit->handle, (PS4000A_CHANNEL)i, NULL, NULL, 0, 0, PS4000A_RATIO_MODE_NONE)) != PICO_OK) {
                    printf("ClearDataBuffers:ps4000aSetDataBuffers(channel %d) ------ 0x%08lx \n", i, status);
                }
            }
        }
    }
public:
    // open connection to digitizer
    Ps4000a() {
        open_digitizer(&digitizer);
        if (status != PICO_OK) { // If unit not found or open no need to continue
            fmt::print("Picoscope devices failed to open or select power source. error code: {:#x}\n", status);
        }
        fmt::print("setting up streaming acquisition\n");
        status = ps4000aSetSimpleTrigger(digitizer.handle, 0, PS4000A_CHANNEL_A, 0, PS4000A_RISING, 0, 0);
        // Disable trigger, no need to call three functions to turn off trigger
        if (status!= PICO_OK) {
            fmt::print("Error setting trigger. error code: {:#x}\n", status);
        }
        StreamDataHandler(&digitizer);
    }

    ~Ps4000a() {
        ps4000aCloseUnit(digitizer.handle);
    }
};

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
private:
    gr::CircularBuffer<event, 10000> snooped{10000};
    decltype(snooped.new_writer()) snoop_writer = snooped.new_writer();
    gr::CircularBuffer<event, 10000> to_inject{10000};
    decltype(to_inject.new_reader()) to_inject_reader = to_inject.new_reader();
public:
    bool ppsAlign= false;
    bool absoluteTime = false;
    bool UTC = false;
    bool UTCleap = false;
    uint64_t snoopID     = 0x0;
    uint64_t snoopMask   = 0x0;
    int64_t  snoopOffset = 0x0;

    // open connection to saftlib
    std::shared_ptr<SAFTd_Proxy> saftd = SAFTd_Proxy::create();
    // get a specific device
    std::map<std::string, std::string> devices = saftd->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> receiver;

    Timing() {
        if (devices.empty()) {
            std::cerr << "No devices attached to saftd" << std::endl;
        }
        receiver = TimingReceiver_Proxy::create(devices.begin()->second);
    }

    void process() {
        inject();
        snoop();
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
        std::shared_ptr<SoftwareActionSink_Proxy> sink = SoftwareActionSink_Proxy::create(receiver->NewSoftwareActionSink("gr_timing_example"));
        std::shared_ptr<SoftwareCondition_Proxy> condition = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, snoopOffset));
        // Accept all errors
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
        const auto startTime = std::chrono::system_clock::now();
        while(true) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(startTime - std::chrono::system_clock::now() + std::chrono::milliseconds(5)).count();
            if (duration > 0) {
                saftlib::wait_for_signal( duration >= std::numeric_limits<int>::max() ? std::numeric_limits<int>::max() : duration);
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
    eb_socket_t socket{};
    eb_device_t device{};
    eb_address_t tx{}, rx{};

    gr::CircularBuffer<char, 10000> input{10000};
    decltype(input.new_writer()) writer = input.new_writer();
    gr::CircularBuffer<char, 10000> output{10000};
    decltype(output.new_reader()) reader = output.new_reader();

public:
    WBConsole() {
        if (eb_status_t status{}; (status = eb_socket_open(EB_ABI_CODE, 0, EB_DATAX|EB_ADDRX, &socket)) != EB_OK) {
            fmt::print("eb_socket_open failed: {}", status);
            throw std::logic_error("failed to open socket");
        }

        if (eb_status_t status{}; (status = eb_device_open(socket, "dev/wbm0", EB_DATAX|EB_ADDRX, 3, &device)) != EB_OK) {
            fmt::print("failed to open etherbone device: {} status: {}", "dev/wbm0", status);
            throw std::logic_error("failed to open device");
        }

        std::array<sdb_device, 1> sdb{};
        int c = sdb.size();
        if (eb_status_t status{}; (status = eb_sdb_find_by_identity(device, CERN_ID, UART_ID, sdb.data(), &c)) != EB_OK) {
            fmt::print("eb_sdb_find_by_identity", status);
            throw std::logic_error("failed to find by identity");
        }

        if (c > 1) {
            fprintf(stderr, "found %d UARTs on that device; pick one with -i #\n", c);
            throw std::logic_error("uart name not ambiguous");
        } else if (c < 1) {
            fprintf(stderr, "could not find UART #%d on that device (%d total)\n", 1, c);
            throw std::logic_error("want to use nonexisting uart");
        }

        printf("Connected to uart at address %" PRIx64"\n", sdb[1].sdb_component.addr_first);
        tx = sdb[1].sdb_component.addr_first + VUART_TX;
        rx = sdb[1].sdb_component.addr_first + VUART_RX;
    }

    ~WBConsole() {
        eb_device_close(device);
        eb_socket_close(socket);
    }

    void process() {
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

    void send_command(std::string_view cmd) {
        output.new_writer().publish([&cmd](std::span<char> buf) { std::copy(cmd.begin(), cmd.end(), buf.begin()); }, cmd.size());
    }

    void setMasterMode(bool enable) {
        send_command(enable ? "mode master" : "mode slave");
    }
};

int interactive(Ps4000a &a, Timing &timing, WBConsole &console) {
    // Initialize UI
    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }
    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop

    bool done = false;
    while (!done) {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void draw_plot() {
    static float xs1[1001], ys1[1001];
    for (int i = 0; i < 1001; ++i) {
        xs1[i] = i * 0.001f;
        ys1[i] = 0.5f + 0.5f * sinf(50.f * (xs1[i] + (float)ImGui::GetTime() / 10.f));
    }
    static double xs2[20], ys2[20];
    for (int i = 0; i < 20; ++i) {
        xs2[i] = i * 1/19.0f;
        ys2[i] = xs2[i] * xs2[i];
    }
    if (ImPlot::BeginPlot("Line Plots")) {
        ImPlot::SetupAxes("x","y");
        ImPlot::PlotLine("f(x)", xs1, ys1, 1001);
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
        ImPlot::PlotLine("g(x)", xs2, ys2, 20,ImPlotLineFlags_Segments);
        ImPlot::EndPlot();
    }
}

int main(int argc, char** argv) {
    Ps4000a picoscope; // a picoscope to examine generated timing signals
    Timing timing; // an interface to the timing card allowing condition & io configuration and event injection & snooping
    WBConsole console; // a whishbone console to send raw commands to the timing card and switch it beteween master and slave mode

    CLI::App app{"timing receiver saftbus example"};
    bool scope = false;
    app.add_flag("--scope", scope, "enter interactive scope mode showing the timing input connected to a picoscope and the generated and received timing msgs");
    app.add_flag("--pps", timing.ppsAlign, "add time to next pps pulse");
    app.add_flag("--abs", timing.absoluteTime, "time is an absolute time instead of an offset");
    app.add_flag("--utc", timing.UTC, "absolute time is in utc, default is tai");
    app.add_flag("--leap", timing.UTCleap, "utc calculation leap second flag");

    bool snoop = false;
    app.add_flag("-s", snoop, "snoop");
    app.add_option("-f", timing.snoopID, "id filter");
    app.add_option("-m", timing.snoopMask, "snoop mask");
    app.add_option("-o", timing.snoopOffset, "snoop offset");

    CLI11_PARSE(app, argc, argv);

    if (scope) {
        fmt::print("entering interactive timing scope mode\n");
        return interactive(picoscope, timing, console);
    }

    return 0;
}
