#ifndef FAIR_PICOSCOPE_PICOSCOPE4000A_HPP
#define FAIR_PICOSCOPE_PICOSCOPE4000A_HPP

#include <fair/picoscope/PicoscopeAPI.hpp>

#include <ps4000aApi.h>

namespace fair::picoscope {

struct Picoscope4000a {
    static constexpr std::size_t N_DIGITAL_CHANNELS = 0UZ;
    static constexpr std::size_t N_ANALOG_CHANNELS  = PS4000A_MAX_CHANNELS;

    using ChannelType              = PS4000A_CHANNEL;
    using ConditionType            = PS4000A_CONDITION;
    using CouplingType             = PS4000A_COUPLING;
    using RangeType                = PICO_CONNECT_PROBE_RANGE;
    using ThresholdDirectionType   = PS4000A_THRESHOLD_DIRECTION;
    using TriggerStateType         = PS4000A_TRIGGER_STATE;
    using TriggerChannelProperties = PS4000A_TRIGGER_CHANNEL_PROPERTIES;
    using ConditionsInfoType       = PS4000A_CONDITIONS_INFO;
    using TimeUnitsType            = PS4000A_TIME_UNITS;
    using StreamingReadyType       = ps4000aStreamingReady;
    using BlockReadyType           = ps4000aBlockReady;
    using RatioModeType            = PS4000A_RATIO_MODE;
    using DeviceResolutionType     = PS4000A_DEVICE_RESOLUTION;
    using NSamplesType             = int32_t;

    int16_t _handle;

    static constexpr std::array<std::pair<ChannelName, ChannelType>, 10> outputs{{
        {ChannelName::A, PS4000A_CHANNEL_A},
        {ChannelName::B, PS4000A_CHANNEL_B},
        {ChannelName::C, PS4000A_CHANNEL_C},
        {ChannelName::D, PS4000A_CHANNEL_D},
        {ChannelName::E, PS4000A_CHANNEL_E},
        {ChannelName::F, PS4000A_CHANNEL_F},
        {ChannelName::G, PS4000A_CHANNEL_G},
        {ChannelName::H, PS4000A_CHANNEL_H},
        {ChannelName::EXTERNAL, PS4000A_EXTERNAL},
        {ChannelName::AUX, PS4000A_TRIGGER_AUX},
    }};

    static constexpr std::array<std::pair<AnalogChannelRange, RangeType>, 14> ranges = {{
        /* this picoscope supports "smart probes", that's why we have to use special enum values here */
        {AnalogChannelRange::ps10mV, PICO_X1_PROBE_10MV /* PS4000A_10MV */},
        {AnalogChannelRange::ps20mV, PICO_X1_PROBE_20MV /* PS4000A_20MV */},
        {AnalogChannelRange::ps50mV, PICO_X1_PROBE_50MV /* PS4000A_50MV */},
        {AnalogChannelRange::ps100mV, PICO_X1_PROBE_100MV /* PS4000A_100MV */},
        {AnalogChannelRange::ps200mV, PICO_X1_PROBE_200MV /* PS4000A_200MV */},
        {AnalogChannelRange::ps500mV, PICO_X1_PROBE_500MV /* PS4000A_500MV */},
        {AnalogChannelRange::ps1V, PICO_X1_PROBE_1V /* PS4000A_1V */},
        {AnalogChannelRange::ps2V, PICO_X1_PROBE_2V /* PS4000A_2V */},
        {AnalogChannelRange::ps5V, PICO_X1_PROBE_5V /* PS4000A_5V */},
        {AnalogChannelRange::ps10V, PICO_X1_PROBE_10V /* PS4000A_10V */},
        {AnalogChannelRange::ps20V, PICO_X1_PROBE_20V /* PS4000A_20V */},
        {AnalogChannelRange::ps50V, PICO_X1_PROBE_50V /* PS4000A_50V */},
        {AnalogChannelRange::ps100V, PICO_X1_PROBE_100V /* PS4000A_100V */},
        {AnalogChannelRange::ps200V, PICO_X1_PROBE_200V /* PS4000A_200V */},
    }};

    static constexpr TimeUnitsType convertTimeUnits(TimeUnits tu) {
        switch (tu) {
        case TimeUnits::fs: return PS4000A_FS;
        case TimeUnits::ps: return PS4000A_PS;
        case TimeUnits::ns: return PS4000A_NS;
        case TimeUnits::us: return PS4000A_US;
        case TimeUnits::ms: return PS4000A_MS;
        case TimeUnits::s: return PS4000A_S;
        }
        return PS4000A_MAX_TIME_UNITS;
    }

    static constexpr RatioModeType ratioNone{PS4000A_RATIO_MODE_NONE};

    [[nodiscard]] static constexpr std::expected<TimebaseResult, Error> convertSampleRateToTimebase(float desiredFreq) {
        // https://www.picotech.com/download/manuals/picoscope-4000-series-a-api-programmers-guide.pdf, page 24
        // For picoscope PicoScope 4824 and 4000A Series
        // -----------------------------------------------------------------------------
        // | Timebase (n)| Sampling Interval (tS)          | Sampling Frequency (fS)   |
        // |             | tS = 12.5 ns × (n+1)            | fS = 80 MHz / (n+1)       |
        // |-------------|---------------------------------|---------------------------|
        // |      0      | 12.5 ns                         | 80 MHz                    |
        // |      1      | 25 ns                           | 40 MHz                    |
        // |      2      | 37.5 ns                         | 26.67 MHz                 |
        // |      3      | 50 ns                           | 20 MHz                    |
        // |      4      | 62.5 ns                         | 16 MHz                    |
        // |     ...     | ...                             | ...                       |
        // |  2^32 - 1   | ~54 s                           | ~18.6 mHz                 |
        // -----------------------------------------------------------------------------
        // Notes: The max sampling rate depends on enabled channels and ADC resolution.
        //        In streaming mode, USB speed may limit the max rate.

        if (desiredFreq <= 0.f || desiredFreq >= 80'000'000.f) {
            return TimebaseResult{0, 80'000'000.f};
        }
        const auto  timebase   = static_cast<uint32_t>((80'000'000.f / desiredFreq) - 1.f); // n = (80 MHz / f_S) - 1
        const float actualFreq = 80'000'000.f / static_cast<float>(timebase + 1);
        return TimebaseResult{timebase, actualFreq};
    }

    static constexpr std::expected<CouplingType, Error> convertToCoupling(Coupling coupling) {
        if (coupling == Coupling::AC) {
            return PS4000A_AC;
        } else if (coupling == Coupling::DC) {
            return PS4000A_DC;
        }
        return std::unexpected(Error(std::format("Unsupported coupling mode: {}", static_cast<int>(coupling))));
    }

    static constexpr std::expected<ThresholdDirectionType, Error> convertToThresholdDirection(TriggerDirection direction) {
        using enum TriggerDirection;
        switch (direction) {
        case Rising: return PS4000A_RISING;
        case Falling: return PS4000A_FALLING;
        case Low: return PS4000A_BELOW;
        case High: return PS4000A_ABOVE;
        default: return std::unexpected(Error(std::format("Unsupported trigger direction: {}", static_cast<int>(direction))));
        }
    };

    static constexpr float uncertainty() {
        // TODO https://www.picotech.com/oscilloscope/4000/picoscope-4000-specifications
        return 0.0000045f;
    }

    PICO_STATUS setDataBuffer(ChannelType channel, int16_t* buffer, int32_t bufferLth, RatioModeType mode) const { return ps4000aSetDataBuffer(_handle, channel, buffer, bufferLth, 0UZ, mode); }

    PICO_STATUS setDataBuffer(ChannelType channel, int16_t* buffer, int32_t bufferLth, uint32_t segmentIndex, RatioModeType mode) const { return ps4000aSetDataBuffer(_handle, channel, buffer, bufferLth, segmentIndex, mode); }

    PICO_STATUS getTimebase2(uint32_t timebase, int32_t noSamples, float* timeIntervalNanoseconds, int32_t* maxSamples, uint32_t segmentIndex) const { return ps4000aGetTimebase2(_handle, timebase, noSamples, timeIntervalNanoseconds, maxSamples, segmentIndex); }

    PICO_STATUS openUnit(const std::string& serial_number) {
        if (serial_number.empty()) { // take any if no serial number is provided (useful for testing purposes)
            return ps4000aOpenUnit(&_handle, nullptr);
        } else {
            return ps4000aOpenUnit(&_handle, const_cast<int8_t*>(reinterpret_cast<const int8_t*>(serial_number.data())));
        }
    }

    static PICO_STATUS openUnitAsync(std::int16_t* status, const std::string& serial_number) {
        if (serial_number.empty()) {
            return ps4000aOpenUnitAsync(status, nullptr);
        } else {
            return ps4000aOpenUnitAsync(status, const_cast<int8_t*>(reinterpret_cast<const int8_t*>(serial_number.data())));
        }
    }

    PICO_STATUS openUnitProgress(std::int16_t* progressPercent, std::int16_t* complete) { return ps4000aOpenUnitProgress(&_handle, progressPercent, complete); }

    [[nodiscard]] bool isOpened() const { return _handle > 0; }

    [[nodiscard]] PICO_STATUS closeUnit() const { return ps4000aCloseUnit(_handle); }

    [[nodiscard]] PICO_STATUS changePowerSource(PICO_STATUS powerState) const { return ps4000aChangePowerSource(_handle, powerState); }

    PICO_STATUS maximumValue(int16_t* value) const { return ps4000aMaximumValue(_handle, value); }

    PICO_STATUS memorySegments(uint32_t nSegments, int32_t* nMaxSamples) const { return ps4000aMemorySegments(_handle, nSegments, nMaxSamples); }

    [[nodiscard]] PICO_STATUS setNoOfCaptures(uint32_t nCaptures) const { return ps4000aSetNoOfCaptures(_handle, nCaptures); }

    PICO_STATUS getNoOfCaptures(uint32_t* nCaptures) const { return ps4000aGetNoOfCaptures(_handle, nCaptures); }

    PICO_STATUS getNoOfProcessedCaptures(uint32_t* nProcessedCaptures) const { return ps4000aGetNoOfProcessedCaptures(_handle, nProcessedCaptures); }

    [[nodiscard]] PICO_STATUS setChannel(ChannelType channel, int16_t enabled, CouplingType type, RangeType range, float analogOffset) const { return ps4000aSetChannel(_handle, channel, enabled, type, range, analogOffset); }

    static int maxChannel() { return PS4000A_MAX_CHANNELS; }

    static int extTriggerMaxValue() { return PS4000A_EXT_MAX_VALUE; }

    static int extTriggerMinValue() { return PS4000A_EXT_MIN_VALUE; }

    static float extTriggerMaxValueVoltage() { return 5.f; }

    static float extTriggerMinValueVoltage() { return -5.f; }

    static CouplingType analogCoupling() { return PS4000A_AC; }

    static TriggerStateType conditionDontCare() { return PS4000A_CONDITION_DONT_CARE; }
    static TriggerStateType conditionTrue() { return PS4000A_CONDITION_TRUE; }
    static TriggerStateType conditionFalse() { return PS4000A_CONDITION_FALSE; }

    static ConditionsInfoType conditionsInfoClear() { return PS4000A_CLEAR; }
    static ConditionsInfoType conditionsInfoAdd() { return PS4000A_ADD; }

    [[nodiscard]] PICO_STATUS setSimpleTrigger(int16_t enable, ChannelType source, int16_t threshold, ThresholdDirectionType direction, uint32_t delay, int16_t autoTriggerMs) const { return ps4000aSetSimpleTrigger(_handle, enable, source, threshold, direction, delay, autoTriggerMs); }

    PICO_STATUS setTriggerChannelConditions(ConditionType* conditions, int16_t nConditions, ConditionsInfoType info) const { return ps4000aSetTriggerChannelConditions(_handle, conditions, nConditions, info); }

    [[nodiscard]] PICO_STATUS driverStop() const { return ps4000aStop(_handle); }

    PICO_STATUS runBlock(int32_t noOfPreTriggerSamples, int32_t noOfPostTriggerSamples, uint32_t timebase, int32_t* timeIndisposed, uint32_t segmentIndex, BlockReadyType ready, void* param) const { return ps4000aRunBlock(_handle, noOfPreTriggerSamples, noOfPostTriggerSamples, timebase, timeIndisposed, segmentIndex, ready, param); }

    PICO_STATUS runStreaming(uint32_t* sampleInterval, TimeUnitsType timeUnits, uint32_t maxPreTriggerSamples, uint32_t maxPostTriggerSamples, int16_t autoStop, uint32_t downSampleRatio, RatioModeType downSampleRatioMode, uint32_t overviewBufferSize) const { return ps4000aRunStreaming(_handle, sampleInterval, timeUnits, maxPreTriggerSamples, maxPostTriggerSamples, autoStop, downSampleRatio, downSampleRatioMode, overviewBufferSize); }

    PICO_STATUS getStreamingLatestValues(StreamingReadyType ready, void* param) const { return ps4000aGetStreamingLatestValues(_handle, ready, param); }

    PICO_STATUS getValues(uint32_t startIndex, uint32_t* noOfSamples, uint32_t downSampleRatio, RatioModeType downSampleRatioMode, uint32_t segmentIndex, int16_t* overflow) const { return ps4000aGetValues(_handle, startIndex, noOfSamples, downSampleRatio, downSampleRatioMode, segmentIndex, overflow); }

    PICO_STATUS getValuesBulk(uint32_t* noOfSamples, uint32_t fromSegmentIndex, uint32_t toSegmentIndex, uint32_t downSampleRatio, RatioModeType downSampleRatioMode, int16_t* overflow) const { return ps4000aGetValuesBulk(_handle, noOfSamples, fromSegmentIndex, toSegmentIndex, downSampleRatio, downSampleRatioMode, overflow); }

    PICO_STATUS getValuesOverlappedBulk(uint32_t startIndex, uint32_t* noOfSamples, uint32_t downSampleRatio, RatioModeType downSampleRatioMode, uint32_t fromSegmentIndex, uint32_t toSegmentIndex, int16_t* overflow) const { return ps4000aGetValuesOverlappedBulk(_handle, startIndex, noOfSamples, downSampleRatio, downSampleRatioMode, fromSegmentIndex, toSegmentIndex, overflow); }

    PICO_STATUS isReady(int16_t* ready) const { return ps4000aIsReady(_handle, ready); }

    PICO_STATUS getValuesTriggerTimeOffsetBulk64(int64_t* times, TimeUnitsType* timeUnits, uint32_t fromSegmentIndex, uint32_t toSegmentIndex) const { return ps4000aGetValuesTriggerTimeOffsetBulk64(_handle, times, timeUnits, fromSegmentIndex, toSegmentIndex); }

    PICO_STATUS getUnitInfo(int8_t* string, int16_t stringLength, int16_t* requiredSize, PICO_INFO info) const { return ps4000aGetUnitInfo(_handle, string, stringLength, requiredSize, info); }

    PICO_STATUS getDeviceResolution(DeviceResolutionType* deviceResolution) const { return ps4000aGetDeviceResolution(_handle, deviceResolution); }

#ifdef ps4000aCheckForUpdate
    std::vector<std::pair<std::string, PICO_FIRMWARE_INFO>> checkFirmwareUpdates(uint16_t& updatesRequired) const {
        std::vector<PICO_FIRMWARE_INFO> firmwareInfo{};
        int16_t                         nFirmwareInfo = 32;
        firmwareInfo.resize(static_cast<std::size_t>(nFirmwareInfo));
        if (ps4000aCheckForUpdate(_handle, firmwareInfo.data(), &nFirmwareInfo, &updatesRequired) != PICO_OK) {
            return {};
        }
        if (ps4000aCheckForUpdate(_handle, nullptr, &nFirmwareInfo, &updatesRequired) != PICO_OK) {
            return {};
        } // it seems like the number of firmware infos populated is only set if we provide a nullptr, so calling again
        firmwareInfo.resize(static_cast<std::size_t>(nFirmwareInfo));
        auto firmwareType = [](auto& info) -> std::string {
            switch (info.firmwareType) {
            case PICO_DRIVER_VERSION: return "driver";
            case PICO_USB_VERSION: return "usb";
            case PICO_HARDWARE_VERSION: return "hardware";
            case PICO_VARIANT_INFO: return "variant";
            case PICO_BATCH_AND_SERIAL: return "batch/serial";
            case PICO_CAL_DATE: return "cal";
            case PICO_KERNEL_VERSION: return "kernel";
            case PICO_DIGITAL_HARDWARE_VERSION: return "hardware";
            case PICO_ANALOGUE_HARDWARE_VERSION: return "analogue hardware";
            case PICO_FIRMWARE_VERSION_1: return "firmware 1";
            case PICO_FIRMWARE_VERSION_2: return "firmware 2";
            case PICO_MAC_ADDRESS: return "mac address";
            case PICO_SHADOW_CAL: return "shadow";
            case PICO_IPP_VERSION: return "ipp";
            case PICO_DRIVER_PATH: return "driver path";
            case PICO_FIRMWARE_VERSION_3: return "firmware 3";
            case PICO_FRONT_PANEL_FIRMWARE_VERSION: return "front panel firmware";
            case PICO_BOOTLOADER_VERSION: return "bootloader";
            default: return "unknown";
            }
        };
        return firmwareInfo | std::views::transform([&](auto& info) { return std::pair{firmwareType(info), info}; }) | std::ranges::to<std::vector>();
    }
#endif

    static PICO_STATUS enumerateUnits(int16_t* count, int8_t* serials, int16_t* serialsSize) { return ps4000aEnumerateUnits(count, serials, serialsSize); };
};

static_assert(PicoscopeImplementationLike<Picoscope4000a>);

} // namespace fair::picoscope

#endif
