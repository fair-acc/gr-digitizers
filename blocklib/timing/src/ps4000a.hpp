// gr
#include <gnuradio-4.0/CircularBuffer.hpp>
// picoscope
#include <ps4000aApi.h>

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
    } CHANNEL_SETTINGS;

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
    } UNIT;

    static constexpr uint32_t	bufferLength = 10000;
    static constexpr uint32_t	ringBufferLength = 50000;
    static constexpr uint32_t inputRanges[] = { 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000 };

    typedef struct tBufferInfo {
        using buftype = gr::CircularBuffer<int16_t , 1<<14>;
        std::array<std::array<int16_t, bufferLength>, 8> driverBuffers{};
        std::array<buftype, 8> data{ buftype{ringBufferLength} ,buftype{ringBufferLength},buftype{ringBufferLength},buftype{ringBufferLength},buftype{ringBufferLength},buftype{ringBufferLength},buftype{ringBufferLength},buftype{ringBufferLength}};
        std::array<decltype(data[0].new_writer()), 8> writers = {
                data[0].new_writer(),data[1].new_writer(),data[2].new_writer(),data[3].new_writer(),
                data[4].new_writer(),data[5].new_writer(), data[6].new_writer(),data[7].new_writer()};
    } BUFFER_INFO;

    std::atomic_bool running = false;
    BUFFER_INFO bufferInfo;
    UNIT digitizer;
    bool initialized = false;
    bool tried = false;

    static void SetDefaults(UNIT *unit) {
        for (int32_t ch = 0; ch < unit->channelCount; ch++) {
            PICO_STATUS status = ps4000aSetChannel(unit->handle,																		// Handle to select the correct device
                                                   (PS4000A_CHANNEL)(PS4000A_CHANNEL_A + ch),											// channel
                                                   unit->channelSettings[PS4000A_CHANNEL_A + ch].enabled,								// If channel is enabled or not
                                                   (PS4000A_COUPLING)unit->channelSettings[PS4000A_CHANNEL_A + ch].DCcoupled,			// If AC or DC coupling
                                                   (PICO_CONNECT_PROBE_RANGE) unit->channelSettings[PS4000A_CHANNEL_A + ch].range,		// The voltage scale of the channel
                                                   unit->channelSettings[PS4000A_CHANNEL_A + ch].analogueOffset);						// Analogue offset
            printf(status ? "SetDefaults:ps4000aSetChannel------ 0x%08lx for channel %i\n" : "", status, ch);
        }
    }

    PICO_STATUS open_digitizer() {
        // Can call this function multiple times to open multiple devices
        // Note that the unit.handle is specific to each device make sure you don't overwrite them
        fmt::print("opening unit\n");
        PICO_STATUS status = ps4000aOpenUnit(&(digitizer.handle), nullptr);
        fmt::print("=> status: {:#x}\n", status);
        if (digitizer.handle == 0) {
            return status;
        }

        // Check status code returned for power status codes
        switch (status) {
            case PICO_OK: // No need to change power source
                break;
            case PICO_POWER_SUPPLY_NOT_CONNECTED:
                fmt::print("Warning: running on usb power!\n");
                status = ps4000aChangePowerSource(digitizer.handle, PICO_POWER_SUPPLY_NOT_CONNECTED);		// Tell the driver that's ok
                break;
            case PICO_USB3_0_DEVICE_NON_USB3_0_PORT:	// User must acknowledge they want to power via USB
                fmt::print("Warning: Connected to non usb 3 port!\n");
                status = ps4000aChangePowerSource(digitizer.handle, PICO_USB3_0_DEVICE_NON_USB3_0_PORT);		// Tell the driver that's ok
                break;
            default:
                return status;
        }

        // Device only has these min and max values
        digitizer.firstRange = PS4000A_10MV;
        digitizer.lastRange = PS4000A_50V;

        fmt::print("get info\n");
        std::string_view description[11] = { "Driver Version", "USB Version", "Hardware Version", "Variant Info", "Serial", "Cal Date",
                                             "Kernel Version", "Digital HW Version", "Analogue HW Version", "Firmware 1", "Firmware 2" };
        int16_t requiredSize;
        int32_t variant;
        char line[80] = { 0 };
        for (int i = 0; i <= 10; i++) {
            status = ps4000aGetUnitInfo(digitizer.handle, (int8_t *) line, sizeof(line), &requiredSize, i);
            if (i == PICO_VARIANT_INFO) { // info = 3 - PICO_VARIANT_INFO
                variant = atoi(line);
                memcpy(&(digitizer.modelString), line, sizeof(digitizer.modelString) == 5 ? 5 : sizeof(digitizer.modelString));
            } else if (i == PICO_BATCH_AND_SERIAL)	{ // info = 4 - PICO_BATCH_AND_SERIAL
                memcpy(&(digitizer.serial), line, static_cast<size_t>(requiredSize));
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
        status = ps4000aSigGenArbitraryMinMaxValues(digitizer.handle, &minArbitraryWaveformValue, &maxArbitraryWaveformValue, &minArbitraryWaveformBufferSize, &maxArbitraryWaveformBufferSize);
        switch (variant) {
            case MODEL_PS4824:
                digitizer.model = MODEL_PS4824;
                digitizer.sigGen = SIGGEN_AWG;
                digitizer.firstRange = PS4000A_10MV;
                digitizer.lastRange = PS4000A_50V;
                digitizer.channelCount = OCTO_SCOPE;
                digitizer.hasETS = false;
                digitizer.AWGFileSize = maxArbitraryWaveformBufferSize;
                digitizer.hasFlexibleResolution = 0;
                digitizer.hasIntelligentProbes = 0;
                break;
            default:
                digitizer.model = MODEL_NONE;
                break;
        }
        for (int ch = 0; ch < digitizer.channelCount; ch++) {
            digitizer.channelSettings[ch].enabled = (ch == 0);
            digitizer.channelSettings[ch].DCcoupled = true;
            digitizer.channelSettings[ch].range = PS4000A_5V;
            digitizer.channelSettings[ch].analogueOffset = 0.0f;
        }
        ps4000aMaximumValue(digitizer.handle, &digitizer.maxADCValue);
        SetDefaults(&digitizer);
        return status;
    }

    int32_t adc_to_mv(int32_t raw, int32_t rangeIndex, UNIT * unit) {
        return (raw * (int32_t) inputRanges[rangeIndex]) / unit->maxADCValue;
    }

    void setupStreaming() {
        PICO_STATUS status;

        // Setup data and temporary application buffers to copy data into
        for (std::size_t i = 0; i < digitizer.channelCount; i++) {
            if (digitizer.channelSettings[PS4000A_CHANNEL_A + i].enabled) {
                status = ps4000aSetDataBuffer(digitizer.handle, (PS4000A_CHANNEL) i, bufferInfo.driverBuffers[i].data(),
                                              bufferInfo.driverBuffers[i].size(), 0, PS4000A_RATIO_MODE_NONE);
                if (status != PICO_OK) { // If unit not found or open no need to continue
                    fmt::print("Failed to set data buffer. error code: {:#x}\n", status);
                    return;
                }
            }
        }
        uint32_t downsampleRatio = 1;
        PS4000A_TIME_UNITS timeUnits = PS4000A_US;
        uint32_t sampleInterval = 200; // 1/50 us = 5kHz
        PS4000A_RATIO_MODE ratioMode = PS4000A_RATIO_MODE_NONE;
        uint32_t preTrigger = 0;
        uint32_t postTrigger = 100; // => 0.1s
        int16_t autostop = false;

        printf("\nStreaming Data for %u samples", postTrigger / downsampleRatio);
        printf("Collect streaming...\n");

        do {
            // For streaming we use sample interval rather than timebase used in ps4000aRunBlock
            status = ps4000aRunStreaming(digitizer.handle, &sampleInterval, timeUnits, preTrigger, postTrigger, autostop,
                                         downsampleRatio, ratioMode, bufferLength);
            if (status != PICO_OK) {
                if (status == PICO_POWER_SUPPLY_CONNECTED || status == PICO_POWER_SUPPLY_NOT_CONNECTED ||
                    status == PICO_POWER_SUPPLY_UNDERVOLTAGE) {
                    status = ps4000aChangePowerSource(digitizer.handle, status);
                } else {
                    printf("setupStreaming:ps4000aRunStreaming ------ 0x%08x \n", status);
                    return;
                }
            }
        } while (status != PICO_OK);
    }

public:
    // open connection to digitizer
    void initialize() {
        PICO_STATUS status = open_digitizer();
        if (status != PICO_OK) { // If unit not found or open no need to continue
            fmt::print("Picoscope devices failed to open or select power source. error code: {:#x}\n", status);
            return;
        }
        fmt::print("setting up streaming acquisition\n");
        status = ps4000aSetSimpleTrigger(digitizer.handle, 0, PS4000A_CHANNEL_A, 0, PS4000A_RISING, 0, 0);
        // Disable trigger, no need to call three functions to turn off trigger
        if (status!= PICO_OK) {
            fmt::print("Error setting trigger. error code: {:#x}\n", status);
            return;
        }
        setupStreaming();
        initialized = true;
    }

    ~Ps4000a() {
        if (initialized) {
            ps4000aStop(digitizer.handle);
            ps4000aCloseUnit(digitizer.handle);
        }
    }

    void process() {
        if (!initialized && !tried) {
            tried = true;
            initialize();
        } else if (initialized) {
            if (!running) {
                PICO_STATUS status = ps4000aGetStreamingLatestValues(digitizer.handle, [](int16_t handle, int32_t noOfSamples, uint32_t startIndex, int16_t overflow, uint32_t triggerAt, int16_t triggered, int16_t autoStop, void * pParameter ) {
                    auto *pPs4000A = (Ps4000a*) pParameter;
                    if (noOfSamples > 0) {
                        for (std::size_t channel = 0; channel < pPs4000A->digitizer.channelCount; channel++) {
                            if (pPs4000A->digitizer.channelSettings[channel].enabled) {
                                if (!pPs4000A->bufferInfo.writers[channel].try_publish([startIndex, channel, &pPs4000A](std::span<int16_t > buffer) {
                                            auto &buf = pPs4000A->bufferInfo.driverBuffers[channel];
                                            std::copy(buf.data() + startIndex, buf.data() + startIndex + buffer.size(), buffer.begin());
                                        }, noOfSamples)) {
                                    fmt::print("digitizer buffer overrun, dropping samples\n");
                                }
                            }
                        }
                    }
                    pPs4000A->running = false;
                }, this);
                if (status != PICO_OK) {
                    fmt::print("Error reading from digitizer: {:#x}\n", status);
                }
            }
        }
    }

    auto buffer() {
        return bufferInfo.data[0];
    }
};
