#ifndef FAIR_PICOSCOPE_PICOSCOPE6000_HPP
#define FAIR_PICOSCOPE_PICOSCOPE6000_HPP

#include <array>
#include <fair/picoscope/PicoscopeAPI.hpp>

#include <ps6000Api.h>

namespace fair::picoscope {

// These structures are not available for 6000 API. We implement them for consistency.
typedef struct tPS6000Condition {
    PS6000_CHANNEL       source;
    PS6000_TRIGGER_STATE condition;
} PS6000_CONDITION;
typedef enum enPS6000ConditionsInfo { PS6000_CLEAR = 0x00000001, PS6000_ADD = 0x00000002 } PS6000_CONDITIONS_INFO;
typedef enum enPS6000DeviceResolution { PS6000A_DR_8BIT } PS6000_DEVICE_RESOLUTION;

struct Picoscope6000 {
    static constexpr std::size_t N_DIGITAL_CHANNELS = 0UZ;
    static constexpr std::size_t N_ANALOG_CHANNELS  = PS6000_MAX_CHANNELS;

    using ChannelType            = PS6000_CHANNEL;
    using ConditionType          = PS6000_CONDITION;
    using CouplingType           = PS6000_COUPLING;
    using RangeType              = PS6000_RANGE;
    using ChannelRangeType       = RangeType;
    using ThresholdDirectionType = PS6000_THRESHOLD_DIRECTION;
    using TriggerStateType       = PS6000_TRIGGER_STATE;
    using ConditionsInfoType     = PS6000_CONDITIONS_INFO;
    using TimeUnitsType          = PS6000_TIME_UNITS;
    using StreamingReadyType     = ps6000StreamingReady;
    using BlockReadyType         = ps6000BlockReady;
    using RatioModeType          = PS6000_RATIO_MODE;
    using DeviceResolutionType   = PS6000_DEVICE_RESOLUTION;
    using NSamplesType           = std::uint32_t;

    std::int16_t _handle;

    static constexpr RatioModeType ratioNone{PS6000_RATIO_MODE_NONE};

    static constexpr std::array<std::pair<ChannelName, ChannelType>, 8> outputs{{
        {ChannelName::A, PS6000_CHANNEL_A},
        {ChannelName::B, PS6000_CHANNEL_B},
        {ChannelName::C, PS6000_CHANNEL_C},
        {ChannelName::D, PS6000_CHANNEL_D},
        {ChannelName::EXTERNAL, PS6000_EXTERNAL},
        {ChannelName::AUX, PS6000_TRIGGER_AUX},
    }};

    static constexpr std::array<std::pair<AnalogChannelRange, RangeType>, 12> ranges = {{
        {AnalogChannelRange::ps10mV, PS6000_10MV},
        {AnalogChannelRange::ps20mV, PS6000_20MV},
        {AnalogChannelRange::ps50mV, PS6000_50MV},
        {AnalogChannelRange::ps100mV, PS6000_100MV},
        {AnalogChannelRange::ps200mV, PS6000_200MV},
        {AnalogChannelRange::ps500mV, PS6000_500MV},
        {AnalogChannelRange::ps1V, PS6000_1V},
        {AnalogChannelRange::ps2V, PS6000_2V},
        {AnalogChannelRange::ps5V, PS6000_5V},
        {AnalogChannelRange::ps10V, PS6000_10V},
        {AnalogChannelRange::ps20V, PS6000_20V},
        {AnalogChannelRange::ps50V, PS6000_50V},
    }};

    static TimeUnitsType convertTimeUnits(TimeUnits tu) {
        switch (tu) {
        case TimeUnits::fs: return PS6000_FS;
        case TimeUnits::ps: return PS6000_PS;
        case TimeUnits::ns: return PS6000_NS;
        case TimeUnits::us: return PS6000_US;
        case TimeUnits::ms: return PS6000_MS;
        case TimeUnits::s: return PS6000_S;
        }
        return PS6000_MAX_TIME_UNITS;
    }

    [[nodiscard]] static constexpr std::expected<TimebaseResult, Error> convertSampleRateToTimebase(const float desiredFreq) {
        // https://www.picotech.com/download/manuals/picoscope-6000-series-programmers-guide.pdf, page 24
        // For PicoScope 6000 Series:
        // -------------------------------------------------------------------------------------------------
        // | Timebase (n) | Sampling Interval (tS)          | Sampling Frequency (fS)                      |
        // |-------------:|---------------------------------|----------------------------------------------|
        // |           0  | 2^0 / 5 GHz = 200 ps            | 5 GHz                                        |
        // |           1  | 2^1 / 5 GHz = 400 ps            | 2.5 GHz                                      |
        // |           2  | 2^2 / 5 GHz = 800 ps            | 1.25 GHz                                     |
        // |           3  | 2^3 / 5 GHz = 1.6 ns            | 625 MHz                                      |
        // |           4  | 2^4 / 5 GHz = 3.2 ns            | 312.5 MHz                                    |
        // |-------------:|---------------------------------|----------------------------------------------|
        // | For timebase ≥ 5: tS = (n - 4) / 156.25e6      | fS = 156.25e6 / (n - 4)                      |
        // |           5  | (5 - 4) / 156.25e6 = 6.4 ns     | ~156.25e6                                    |
        // |   2^32 - 1   | (2^32 - 1 - 4)/156.25e6 ~ 6.87s | ~0.15 Hz                                     |
        // -------------------------------------------------------------------------------------------------
        //
        // Notes:
        // 1. The maximum possible sampling rate may depend on the number of enabled channels and on the sampling mode: please refer to the data sheet for details.
        // 2. In streaming mode, the speed of the USB port may affect the rate of data transfer.
        //

        // If out of range (<= 0 or > 5 GHz), default to maximum sampling (timebase=0).
        if (desiredFreq <= 0.f || desiredFreq >= 5.0e9f) {
            return TimebaseResult{0, 5.0e9f}; // timebase = 0 → 200 ps → 5 GHz
        } else if (desiredFreq >= 2.5e9f) {
            return TimebaseResult{1, 2.5e9f}; // timebase = 1 → 400 ps → 2.5 GHz
        } else if (desiredFreq >= 1.25e9f) {
            return TimebaseResult{2, 1.25e9f}; // timebase = 2 → 800 ps → 1.25 GHz
        } else if (desiredFreq >= 625.0e6f) {
            return TimebaseResult{3, 625.0e6f}; // timebase = 3 → 1.6 ns → 625 MHz
        } else if (desiredFreq >= 312.5e6f) {
            return TimebaseResult{4, 312.5e6f}; // timebase = 4 → 3.2 ns → 312.5 MHz
        } else {
            const auto  timebase   = static_cast<std::uint32_t>((156.25e6f / desiredFreq) + 4.0f); // n  = (156.25e6 / desiredFreq) + 4
            const float actualFreq = 156.25e6f / (static_cast<float>(timebase) - 4.0f);
            return TimebaseResult{timebase, actualFreq};
        }
    }

    static constexpr std::expected<CouplingType, Error> convertToCoupling(Coupling coupling) {
        if (coupling == Coupling::AC) {
            return PS6000_AC;
        } else if (coupling == Coupling::DC) {
            return PS6000_DC_1M;
        } else if (coupling == Coupling::DC_50R) {
            return PS6000_DC_50R;
        }
        return std::unexpected(Error(std::format("Unsupported coupling mode: {}", static_cast<int>(coupling))));
    }

    static constexpr std::expected<ThresholdDirectionType, Error> convertToThresholdDirection(TriggerDirection direction) {
        using enum TriggerDirection;
        switch (direction) {
        case Rising: return PS6000_RISING;
        case Falling: return PS6000_FALLING;
        case Low: return PS6000_BELOW;
        case High: return PS6000_ABOVE;
        default: return std::unexpected(Error(std::format("Unsupported trigger direction: {}", static_cast<int>(direction))));
        }
    };

    static constexpr float uncertainty() {
        // TODO https://www.picotech.com/oscilloscope/6000/picoscope-6000-specifications
        return 0.0000200f;
    }

    PICO_STATUS setDataBuffer(ChannelType channel, int16_t* buffer, int32_t bufferLth, RatioModeType mode) const { return ps6000SetDataBuffer(_handle, channel, buffer, static_cast<uint32_t>(bufferLth), mode); }

    PICO_STATUS setDataBuffer(ChannelType channel, int16_t* buffer, int32_t bufferLth, uint32_t segmentIndex, RatioModeType mode) const { return ps6000SetDataBufferBulk(_handle, channel, buffer, static_cast<uint32_t>(bufferLth), segmentIndex, mode); }

    PICO_STATUS getTimebase2(uint32_t timebase, int32_t noSamples, float* timeIntervalNanoseconds, int32_t* maxSamples, uint32_t segmentIndex) const {
        // Need to do convertion because picoscope 6000 API requires uint32_t, other APIs require int32_t
        auto              maxSamplesU = static_cast<uint32_t>(*maxSamples);
        const PICO_STATUS status      = ps6000GetTimebase2(_handle, timebase, static_cast<uint32_t>(noSamples), timeIntervalNanoseconds, /* oversample */ 0, &maxSamplesU, segmentIndex);
        *maxSamples                   = static_cast<int32_t>(maxSamplesU);
        return status;
    }

    PICO_STATUS openUnit(const std::string& serial_number) {
        // take any if no serial number is provided (useful for testing purposes)
        if (serial_number.empty()) {
            return ps6000OpenUnit(&_handle, nullptr);
        } else {
            return ps6000OpenUnit(&_handle, const_cast<int8_t*>(reinterpret_cast<const int8_t*>(serial_number.data())));
        }
    }

    static PICO_STATUS openUnitAsync(std::int16_t* status, const std::string& serial_number) {
        if (serial_number.empty()) {
            return ps6000OpenUnitAsync(status, nullptr);
        } else {
            return ps6000OpenUnitAsync(status, const_cast<int8_t*>(reinterpret_cast<const int8_t*>(serial_number.data())));
        }
    }

    PICO_STATUS openUnitProgress(std::int16_t* progressPercent, std::int16_t* complete) { return ps6000OpenUnitProgress(&_handle, progressPercent, complete); }

    [[nodiscard]] bool isOpened() const { return _handle > 0; }

    [[nodiscard]] PICO_STATUS closeUnit() const { return ps6000CloseUnit(_handle); }

    static PICO_STATUS changePowerSource(PICO_STATUS) { return PICO_NOT_SUPPORTED_BY_THIS_DEVICE; }

    static PICO_STATUS maximumValue(int16_t* value) {
        *value = PS6000_MAX_VALUE;
        return PICO_OK;
    }

    PICO_STATUS memorySegments(uint32_t nSegments, int32_t* nMaxSamples) const {
        // Need to do convertion because picoscope 6000 API requires uint32_t, other APIs require int32_t
        auto              nMaxSamplesU = static_cast<uint32_t>(*nMaxSamples);
        const PICO_STATUS status       = ps6000MemorySegments(_handle, nSegments, &nMaxSamplesU);
        *nMaxSamples                   = static_cast<int32_t>(nMaxSamplesU);
        return status;
    }

    [[nodiscard]] PICO_STATUS setNoOfCaptures(uint32_t nCaptures) const { return ps6000SetNoOfCaptures(_handle, nCaptures); }

    PICO_STATUS getNoOfCaptures(uint32_t* nCaptures) const { return ps6000GetNoOfCaptures(_handle, nCaptures); }

    PICO_STATUS getNoOfProcessedCaptures(uint32_t* nProcessedCaptures) const { return ps6000GetNoOfProcessedCaptures(_handle, nProcessedCaptures); }

    [[nodiscard]] PICO_STATUS setChannel(ChannelType channel, int16_t enabled, CouplingType type, ChannelRangeType range, float analogOffset) const { return ps6000SetChannel(_handle, channel, enabled, type, range, analogOffset, PS6000_BW_FULL); }

    static int maxChannel() { return PS6000_MAX_CHANNELS; }

    static int extTriggerMaxValue() { return 32767; }  // TODO: taken from 3000a
    static int extTriggerMinValue() { return -32767; } // TODO: taken from 3000a

    static float extTriggerMaxValueVoltage() { return 5.f; }
    static float extTriggerMinValueVoltage() { return -5.f; }

    static CouplingType analogCoupling() { return PS6000_AC; }

    static TriggerStateType conditionDontCare() { return PS6000_CONDITION_DONT_CARE; }

    static ConditionsInfoType conditionsInfoClear() { return PS6000_CLEAR; }
    static ConditionsInfoType conditionsInfoAdd() { return PS6000_ADD; }

    [[nodiscard]] PICO_STATUS setSimpleTrigger(int16_t enable, ChannelType source, int16_t threshold, ThresholdDirectionType direction, uint32_t delay, int16_t autoTriggerMs) const { return ps6000SetSimpleTrigger(_handle, enable, source, threshold, direction, delay, autoTriggerMs); }

    static PS6000_TRIGGER_CONDITIONS convertToTriggerConditions(const ConditionType* conditions, int16_t nConditions) {
        PS6000_TRIGGER_CONDITIONS result{};
        for (int16_t i = 0; i < nConditions; ++i) {
            const ConditionType& cond = conditions[i];
            if (cond.source == PS6000_CHANNEL_A) {
                result.channelA = cond.condition;
            } else if (cond.source == PS6000_CHANNEL_B) {
                result.channelB = cond.condition;
            } else if (cond.source == PS6000_CHANNEL_C) {
                result.channelC = cond.condition;
            } else if (cond.source == PS6000_CHANNEL_D) {
                result.channelD = cond.condition;
            } else if (cond.source == PS6000_EXTERNAL) {
                result.external = cond.condition;
            } else if (cond.source == PS6000_TRIGGER_AUX) {
                result.aux = cond.condition;
            }
        }
        return result;
    }

    PICO_STATUS setTriggerChannelConditions(const ConditionType* conditions, int16_t nConditions, ConditionsInfoType) const {
        PS6000_TRIGGER_CONDITIONS ps6000Conditions = convertToTriggerConditions(conditions, nConditions);
        return ps6000SetTriggerChannelConditions(_handle, &ps6000Conditions, 1);
    }

    [[nodiscard]] PICO_STATUS driverStop() const { return ps6000Stop(_handle); }

    PICO_STATUS runBlock(int32_t noOfPreTriggerSamples, int32_t noOfPostTriggerSamples, uint32_t timebase, int32_t* timeIndisposed, uint32_t segmentIndex, BlockReadyType ready, void* param) const { //
        return ps6000RunBlock(_handle, static_cast<uint32_t>(noOfPreTriggerSamples), static_cast<uint32_t>(noOfPostTriggerSamples), timebase, /* oversample, not used*/ 0, timeIndisposed, segmentIndex, ready, param);
    }

    PICO_STATUS runStreaming(uint32_t* sampleInterval, TimeUnitsType timeUnits, uint32_t maxPreTriggerSamples, uint32_t maxPostTriggerSamples, int16_t autoStop, uint32_t downSampleRatio, RatioModeType downSampleRatioMode, uint32_t overviewBufferSize) const { //
        return ps6000RunStreaming(_handle, sampleInterval, timeUnits, maxPreTriggerSamples, maxPostTriggerSamples, autoStop, downSampleRatio, downSampleRatioMode, overviewBufferSize);
    }

    PICO_STATUS getStreamingLatestValues(StreamingReadyType ready, void* param) const { return ps6000GetStreamingLatestValues(_handle, ready, param); }

    PICO_STATUS getValues(uint32_t startIndex, uint32_t* noOfSamples, uint32_t downSampleRatio, RatioModeType downSampleRatioMode, uint32_t segmentIndex, int16_t* overflow) const { return ps6000GetValues(_handle, startIndex, noOfSamples, downSampleRatio, downSampleRatioMode, segmentIndex, overflow); }

    PICO_STATUS getValuesBulk(uint32_t* noOfSamples, uint32_t fromSegmentIndex, uint32_t toSegmentIndex, uint32_t downSampleRatio, RatioModeType downSampleRatioMode, int16_t* overflow) const { return ps6000GetValuesBulk(_handle, noOfSamples, fromSegmentIndex, toSegmentIndex, downSampleRatio, downSampleRatioMode, overflow); }

    PICO_STATUS getValuesOverlappedBulk(uint32_t startIndex, uint32_t* noOfSamples, uint32_t downSampleRatio, RatioModeType downSampleRatioMode, uint32_t fromSegmentIndex, uint32_t toSegmentIndex, int16_t* overflow) const { return ps6000GetValuesOverlappedBulk(_handle, startIndex, noOfSamples, downSampleRatio, downSampleRatioMode, fromSegmentIndex, toSegmentIndex, overflow); }

    PICO_STATUS isReady(int16_t* ready) const { return ps6000IsReady(_handle, ready); }

    PICO_STATUS getValuesTriggerTimeOffsetBulk64(int64_t* times, TimeUnitsType* timeUnits, uint32_t fromSegmentIndex, uint32_t toSegmentIndex) const { return ps6000GetValuesTriggerTimeOffsetBulk64(_handle, times, timeUnits, fromSegmentIndex, toSegmentIndex); }

    PICO_STATUS getUnitInfo(int8_t* string, int16_t stringLength, int16_t* requiredSize, PICO_INFO info) const { return ps6000GetUnitInfo(_handle, string, stringLength, requiredSize, info); }

    static PICO_STATUS getDeviceResolution(DeviceResolutionType*) { return PICO_NOT_SUPPORTED_BY_THIS_DEVICE; }

    static PICO_STATUS enumerateUnits(int16_t* count, int8_t* serials, int16_t* serialsSize) { return ps6000EnumerateUnits(count, serials, serialsSize); };
};

static_assert(PicoscopeImplementationLike<Picoscope6000>);

} // namespace fair::picoscope

#endif
