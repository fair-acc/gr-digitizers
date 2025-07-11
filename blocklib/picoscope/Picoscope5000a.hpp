#ifndef FAIR_PICOSCOPE_PICOSCOPE5000A_HPP
#define FAIR_PICOSCOPE_PICOSCOPE5000A_HPP

#include <PicoscopeAPI.hpp>

#include <ps5000aApi.h>

namespace fair::picoscope {

struct Picoscope5000a {
    static constexpr std::size_t N_ANALOG_CHANNELS = 4uz;
    static constexpr std::size_t N_DIGITAL_CHANNELS = 2uz;
    static constexpr std::underlying_type_t<PS5000A_CHANNEL> DIGI_PORT_0 = PS5000A_DIGITAL_PORT0;

public:
    using ChannelType            = PS5000A_CHANNEL;
    using ConditionType          = PS5000A_CONDITION;
    using CouplingType           = PS5000A_COUPLING;
    using RangeType              = PS5000A_RANGE;
    using ChannelRangeType       = RangeType;
    using ThresholdDirectionType = PS5000A_THRESHOLD_DIRECTION;
    using TriggerStateType       = PS5000A_TRIGGER_STATE;
    //using TriggerDirectionType   = PS5000A_DIRECTION;
    //using TriggerChannelProperties = PS5000A_TRIGGER_CHANNEL_PROPERTIES_V2;
    using ConditionsInfoType     = PS5000A_CONDITIONS_INFO;
    using TimeUnitsType          = PS5000A_TIME_UNITS;
    using StreamingReadyType     = ps5000aStreamingReady;
    using BlockReadyType         = ps5000aBlockReady;
    using RatioModeType          = PS5000A_RATIO_MODE;
    using DeviceResolutionType   = PS5000A_DEVICE_RESOLUTION;
    using NSamplesType           = int32_t;

    int16_t _handle;

    static TimeUnitsType convertTimeUnits(TimeUnits tu) {
        switch (tu) {
        case TimeUnits::fs: return PS5000A_FS;
        case TimeUnits::ps: return PS5000A_PS;
        case TimeUnits::ns: return PS5000A_NS;
        case TimeUnits::us: return PS5000A_US;
        case TimeUnits::ms: return PS5000A_MS;
        case TimeUnits::s: return PS5000A_S;
        }
        return PS5000A_MAX_TIME_UNITS;
    }

    [[nodiscard]] constexpr TimebaseResult convertSampleRateToTimebase(float desiredFreq) const {
        // https://www.picotech.com/download/manuals/picoscope-5000-series-a-api-programmers-guide.pdf, page 28
        // -----------------------------------------------------------------------------------------------------------------------
        // 8-bit mode
        // -----------------------------------------------------------------------------------------------------------------------
        // | Timebase (n) | Sampling Interval (tS)               | Sampling Frequency (fS)           |
        // |--------------|--------------------------------------|-----------------------------------|
        // |     0        | 1 ns                                 | 1 GHz (only one channel enabled)  |
        // |     1        | 2 ns                                 | 500 MHz                           |
        // |     2        | 4 ns                                 | 250 MHz                           |
        // |  3 ... 2^32-1| (n–2) / 125,000,000                  | 125 MHz / (n-2)                   |
        //
        // -----------------------------------------------------------------------------------------------------------------------
        // 12-bit mode
        // -----------------------------------------------------------------------------------------------------------------------
        // | Timebase (n) | Sampling Interval (tS)               | Sampling Frequency (fS)           |
        // |--------------|--------------------------------------|-----------------------------------|
        // |     1        | 2 ns                                 | 500 MHz (only one channel enabled)|
        // |     2        | 4 ns                                 | 250 MHz                           |
        // |     3        | 8 ns                                 | 125 MHz                           |
        // |  4 ... 2^32-2| (n–3) / 62,500,000                   | 62.5 MHz / (n-3)                  |
        //
        // -----------------------------------------------------------------------------------------------------------------------
        // 14-bit mode
        // -----------------------------------------------------------------------------------------------------------------------
        // | Timebase (n) | Sampling Interval (tS)               | Sampling Frequency (fS)           |
        // |--------------|--------------------------------------|-----------------------------------|
        // |     3        | 8 ns                                 | 125 MHz (single channel)          |
        // |  4 ... 2^32-1| (n–2) / 125,000,000                  | 125 MHz / (n-2)                   |
        //
        // -----------------------------------------------------------------------------------------------------------------------
        // 15-bit mode
        // -----------------------------------------------------------------------------------------------------------------------
        // | Timebase (n) | Sampling Interval (tS)               | Sampling Frequency (fS)           |
        // |--------------|--------------------------------------|-----------------------------------|
        // |     3        | 8 ns                                 | 125 MHz (up to two channels)      |
        // |  4 ... 2^32-1| (n–2) / 125,000,000                  | 125 MHz / (n-2)                   |
        //
        // -----------------------------------------------------------------------------------------------------------------------
        // 16-bit mode
        // -----------------------------------------------------------------------------------------------------------------------
        // | Timebase (n) | Sampling Interval (tS)               | Sampling Frequency (fS)           |
        // |--------------|--------------------------------------|-----------------------------------|
        // |     4        | 16 ns                                | 62.5 MHz (one channel enabled)    |
        // |  5 ... 2^32-2| (n–3) / 62,500,000                   | 62.5 MHz / (n-3)                  |
        // -----------------------------------------------------------------------------------------------------------------------

        // TODO: make it as function parameter?
        DeviceResolutionType deviceResolution;
        auto                 status = getDeviceResolution(&deviceResolution);
        if (status != PICO_OK) {
            throw std::runtime_error("Cannot get device resolution");
        }

        switch (deviceResolution) {
        case PS5000A_DR_8BIT: {
            if (desiredFreq <= 0.f || desiredFreq >= 1.e9f) {
                return {0, 1.e9f};
            } else if (desiredFreq >= 500.e6f) {
                return {1, 500.e6f};
            } else if (desiredFreq >= 250.e6f) {
                return {2, 250.e6f};
            }
            const auto  timebase   = static_cast<uint32_t>((125.e6f / desiredFreq) + 2); // n = (125 MHz / f_S) + 2;
            const float actualFreq = 125.e6f / static_cast<float>(timebase - 2);
            return {timebase, actualFreq};
        }

        case PS5000A_DR_12BIT: {
            if (desiredFreq <= 0.f || desiredFreq >= 500.e6f) {
                return {1, 500.e6f};
            } else if (desiredFreq >= 250.e6f) {
                return {2, 250.e6f};
            } else if (desiredFreq >= 125.e6f) {
                return {3, 125.e6f};
            }
            const auto  timebase   = static_cast<uint32_t>((62.5e6f / desiredFreq) + 3.f); // n = (62.5 MHz / f_S) + 3
            const float actualFreq = 62.5e6f / static_cast<float>(timebase - 3);
            return {timebase, actualFreq};
        }

        case PS5000A_DR_14BIT:
        case PS5000A_DR_15BIT: {
            if (desiredFreq <= 0.f || desiredFreq >= 125.e6f) {
                return {3, 125.e6f};
            }
            const auto  timebase   = static_cast<uint32_t>((125.e6f / desiredFreq) + 2); // n = (125 MHz / f_S) + 2
            const float actualFreq = 125.e6f / static_cast<float>(timebase - 2);
            return {timebase, actualFreq};
        }

        case PS5000A_DR_16BIT: {
            if (desiredFreq <= 0.f || desiredFreq >= 62.5e6f) {
                return {4, 62.5e6f};
            }
            const auto  timebase   = static_cast<uint32_t>((62.5e6f / desiredFreq) + 3); // n = (62.5 MHz / f_S) + 3
            const float actualFreq = 62.5e6f / static_cast<float>(timebase - 3);
            return {timebase, actualFreq};
        }
        }
        throw std::runtime_error(std::format("Unsupported device resolution: {}", static_cast<int>(deviceResolution)));
    }

    static constexpr std::optional<std::size_t> convertToOutputIndex(std::string_view source) {
        static constexpr std::array<std::pair<std::string_view, std::size_t>, 4> channelMap{{{"A", 0}, {"B", 1}, {"C", 2}, {"D", 3}}};

        const auto it = std::ranges::find_if(channelMap, [source](auto&& kv) { return kv.first == source; });
        if (it != channelMap.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    static constexpr std::optional<ChannelType> convertToChannel(std::string_view source) {
        static constexpr std::array<std::pair<std::string_view, ChannelType>, 5> channelMap{{//
            {"A", PS5000A_CHANNEL_A}, {"B", PS5000A_CHANNEL_B}, {"C", PS5000A_CHANNEL_C}, {"D", PS5000A_CHANNEL_D}, {"EXTERNAL", PS5000A_EXTERNAL}}};

        const auto it = std::ranges::find_if(channelMap, [source](auto&& kv) { return kv.first == source; });
        if (it != channelMap.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    static constexpr std::optional<ChannelType> convertToChannel(std::size_t channelIndex) {
        static constexpr std::array<std::pair<std::size_t, ChannelType>, 5> channelMap{{//
            {0UZ, PS5000A_CHANNEL_A}, {1UZ, PS5000A_CHANNEL_B}, {2UZ, PS5000A_CHANNEL_C}, {3UZ, PS5000A_CHANNEL_D}, {4UZ, PS5000A_EXTERNAL}}};

        const auto it = std::ranges::find_if(channelMap, [channelIndex](auto&& kv) { return kv.first == channelIndex; });
        if (it != channelMap.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    static constexpr CouplingType convertToCoupling(Coupling coupling) {
        if (coupling == Coupling::AC) {
            return PS5000A_AC;
        } else if (coupling == Coupling::DC) {
            return PS5000A_DC;
        }
        throw std::runtime_error(std::format("Unsupported coupling mode: {}", static_cast<int>(coupling)));
    }

    static constexpr RangeType convertToRange(float range) {
        static constexpr std::array<std::pair<float, RangeType>, 12> rangeMap = {{                      //
            {0.01f, PS5000A_10MV}, {0.02f, PS5000A_20MV}, {0.05f, PS5000A_50MV}, {0.1f, PS5000A_100MV}, //
            {0.2f, PS5000A_200MV}, {0.5f, PS5000A_500MV}, {1.0f, PS5000A_1V}, {2.0f, PS5000A_2V},       //
            {5.0f, PS5000A_5V}, {10.0f, PS5000A_10V}, {20.0f, PS5000A_20V}, {50.0f, PS5000A_50V}}};

        const auto exactIt = std::ranges::find_if(rangeMap, [range](auto& kv) { return std::fabs(kv.first - range) < 1e-6f; });
        if (exactIt != rangeMap.end()) {
            return exactIt->second;
        }
        // if no exact match -> find the next higher range
        const auto upperIt = std::ranges::find_if(rangeMap, [range](const auto& kv) { return kv.first > range; });
        return upperIt != rangeMap.end() ? upperIt->second : PS5000A_50V;
    }

    static constexpr ThresholdDirectionType convertToThresholdDirection(TriggerDirection direction) {
        using enum TriggerDirection;
        switch (direction) {
        case Rising: return PS5000A_RISING;
        case Falling: return PS5000A_FALLING;
        case Low: return PS5000A_BELOW;
        case High: return PS5000A_ABOVE;
        default: throw std::runtime_error(std::format("Unsupported trigger direction: {}", static_cast<int>(direction)));
        }
    };

    static constexpr float uncertainty() {
        // TODO https://www.picotech.com/oscilloscope/5000/picoscope-5000-specifications
        return 0.0000120f;
    }

    PICO_STATUS setDataBuffer(ChannelType channel, int16_t* buffer, int32_t bufferLth, RatioModeType mode) const { return ps5000aSetDataBuffer(_handle, channel, buffer, bufferLth, 0UZ, mode); }

    PICO_STATUS setDataBufferForSegment(ChannelType channel, int16_t* buffer, int32_t bufferLth, uint32_t segmentIndex, RatioModeType mode) const { return ps5000aSetDataBuffer(_handle, channel, buffer, bufferLth, segmentIndex, mode); }

    PICO_STATUS getTimebase2(uint32_t timebase, int32_t noSamples, float* timeIntervalNanoseconds, int32_t* maxSamples, uint32_t segmentIndex) const { return ps5000aGetTimebase2(_handle, timebase, noSamples, timeIntervalNanoseconds, maxSamples, segmentIndex); }

    PICO_STATUS openUnit(const std::string& serial_number) {
        // take any if serial number is not provided (useful for testing purposes)
        // TODO: do we need to make `resolution` a setting?
        if (serial_number.empty()) {
            return ps5000aOpenUnit(&_handle, nullptr, PS5000A_DR_8BIT);
        } else {
            return ps5000aOpenUnit(&_handle, const_cast<int8_t*>(reinterpret_cast<const int8_t*>(serial_number.data())), PS5000A_DR_8BIT);
        }
    }

    [[nodiscard]] bool isOpened() const { return _handle > 0; }

    [[nodiscard]] PICO_STATUS closeUnit() const { return ps5000aCloseUnit(_handle); }

    [[nodiscard]] PICO_STATUS changePowerSource(PICO_STATUS powerstate) const { return ps5000aChangePowerSource(_handle, powerstate); }

    PICO_STATUS maximumValue(int16_t* value) const { return ps5000aMaximumValue(_handle, value); }

    PICO_STATUS memorySegments(uint32_t nSegments, int32_t* nMaxSamples) const { return ps5000aMemorySegments(_handle, nSegments, nMaxSamples); }

    [[nodiscard]] PICO_STATUS setNoOfCaptures(uint32_t nCaptures) const { return ps5000aSetNoOfCaptures(_handle, nCaptures); }

    PICO_STATUS getNoOfCaptures(uint32_t* nCaptures) const { return ps5000aGetNoOfCaptures(_handle, nCaptures); }

    PICO_STATUS getNoOfProcessedCaptures(uint32_t* nProcessedCaptures) const { return ps5000aGetNoOfProcessedCaptures(_handle, nProcessedCaptures); }

    [[nodiscard]] PICO_STATUS setChannel(ChannelType channel, int16_t enabled, CouplingType type, ChannelRangeType range, float analogOffset) const { return ps5000aSetChannel(_handle, channel, enabled, type, range, analogOffset); }

    static int maxChannel() { return PS5000A_MAX_CHANNELS; }

    static int extTriggerMaxValue() { return PS5000A_EXT_MAX_VALUE; }

    static int extTriggerMinValue() { return PS5000A_EXT_MIN_VALUE; }

    static float extTriggerMaxValueVoltage() { return 5.f; }

    static float extTriggerMinValueVoltage() { return -5.f; }

    static CouplingType analogCoupling() { return PS5000A_AC; }

    static TriggerStateType conditionDontCare() { return PS5000A_CONDITION_DONT_CARE; }
    static TriggerStateType conditionTrue() { return PS5000A_CONDITION_TRUE; }
    static TriggerStateType conditionFalse() { return PS5000A_CONDITION_FALSE; }

    static ConditionsInfoType conditionsInfoClear() { return PS5000A_CLEAR; }

    static ConditionsInfoType conditionsInfoAdd() { return PS5000A_ADD; }

    [[nodiscard]] PICO_STATUS setSimpleTrigger(int16_t enable, ChannelType source, int16_t threshold, ThresholdDirectionType direction, uint32_t delay, int16_t autoTriggerMs) const { return ps5000aSetSimpleTrigger(_handle, enable, source, threshold, direction, delay, autoTriggerMs); }

    PICO_STATUS setTriggerChannelConditions(ConditionType* conditions, int16_t nConditions, ConditionsInfoType info) const { return ps5000aSetTriggerChannelConditionsV2(_handle, conditions, nConditions, info); }

    [[nodiscard]] PICO_STATUS driverStop() const { return ps5000aStop(_handle); }

    PICO_STATUS runBlock(int32_t noOfPreTriggerSamples, int32_t noOfPostTriggerSamples, uint32_t timebase, int32_t* timeIndisposed, uint32_t segmentIndex, BlockReadyType ready, void* param) const { return ps5000aRunBlock(_handle, noOfPreTriggerSamples, noOfPostTriggerSamples, timebase, timeIndisposed, segmentIndex, ready, param); }

    PICO_STATUS runStreaming(uint32_t* sampleInterval, TimeUnitsType timeUnits, uint32_t maxPreTriggerSamples, uint32_t maxPostTriggerSamples, int16_t autoStop, uint32_t downSampleRatio, RatioModeType downSampleRatioMode, uint32_t overviewBufferSize) const { return ps5000aRunStreaming(_handle, sampleInterval, timeUnits, maxPreTriggerSamples, maxPostTriggerSamples, autoStop, downSampleRatio, downSampleRatioMode, overviewBufferSize); }

    static RatioModeType ratioNone() { return PS5000A_RATIO_MODE_NONE; }

    PICO_STATUS getStreamingLatestValues(StreamingReadyType ready, void* param) const { return ps5000aGetStreamingLatestValues(_handle, ready, param); }

    PICO_STATUS getValues(uint32_t startIndex, uint32_t* noOfSamples, uint32_t downSampleRatio, RatioModeType downSampleRatioMode, uint32_t segmentIndex, int16_t* overflow) const { return ps5000aGetValues(_handle, startIndex, noOfSamples, downSampleRatio, downSampleRatioMode, segmentIndex, overflow); }

    PICO_STATUS getValuesBulk(uint32_t* noOfSamples, uint32_t fromSegmentIndex, uint32_t toSegmentIndex, uint32_t downSampleRatio, RatioModeType downSampleRatioMode, int16_t* overflow) const { return ps5000aGetValuesBulk(_handle, noOfSamples, fromSegmentIndex, toSegmentIndex, downSampleRatio, downSampleRatioMode, overflow); }

    PICO_STATUS getValuesOverlappedBulk(uint32_t startIndex, uint32_t* noOfSamples, uint32_t downSampleRatio, RatioModeType downSampleRatioMode, uint32_t fromSegmentIndex, uint32_t toSegmentIndex, int16_t* overflow) const { return ps5000aGetValuesOverlappedBulk(_handle, startIndex, noOfSamples, downSampleRatio, downSampleRatioMode, fromSegmentIndex, toSegmentIndex, overflow); }

    PICO_STATUS isReady(int16_t* ready) const { return ps5000aIsReady(_handle, ready); }

    PICO_STATUS getValuesTriggerTimeOffsetBulk64(int64_t* times, TimeUnitsType* timeUnits, uint32_t fromSegmentIndex, uint32_t toSegmentIndex) const { return ps5000aGetValuesTriggerTimeOffsetBulk64(_handle, times, timeUnits, fromSegmentIndex, toSegmentIndex); }

    PICO_STATUS getUnitInfo(int8_t* string, int16_t stringLength, int16_t* requiredSize, PICO_INFO info) const { return ps5000aGetUnitInfo(_handle, string, stringLength, requiredSize, info); }

    PICO_STATUS getDeviceResolution(DeviceResolutionType* deviceResolution) const { return ps5000aGetDeviceResolution(_handle, deviceResolution); }

    // Digital picoscope inputs

    [[nodiscard]] PICO_STATUS setDigitalPorts(int16_t digitalPortThreshold) const {
        for (std::size_t i = 0; i < 2; i++) {
            const PS5000A_CHANNEL channelId = i == 0 ? PS5000A_DIGITAL_PORT0 : PS5000A_DIGITAL_PORT1;
            if (const PICO_STATUS status = ps5000aSetDigitalPort(_handle, channelId, true, digitalPortThreshold); status != PICO_OK) {
                return status;
            }
        }
        return PICO_OK;
    }

    [[nodiscard]] PICO_STATUS setTriggerDigitalPort(int pinNumber, TriggerDirection direction) const {
        if (pinNumber < 0 || pinNumber > 15) {
            return PICO_INVALID_DIGITAL_CHANNEL;
        }
        PS5000A_DIGITAL_DIRECTION digitalDirection;
        switch (direction) {
        case TriggerDirection::Rising: digitalDirection = PS5000A_DIGITAL_DIRECTION_RISING; break;
        case TriggerDirection::Falling: digitalDirection = PS5000A_DIGITAL_DIRECTION_FALLING; break;
        case TriggerDirection::Low: digitalDirection = PS5000A_DIGITAL_DIRECTION_LOW; break;
        case TriggerDirection::High: digitalDirection = PS5000A_DIGITAL_DIRECTION_HIGH; break;
        default: return PICO_INVALID_DIGITAL_TRIGGER_DIRECTION;
        }

        PS5000A_CONDITION cond;
        cond.source    = pinNumber < 8 ? PS5000A_DIGITAL_PORT0 : PS5000A_DIGITAL_PORT1;
        cond.condition = PS5000A_CONDITION_TRUE;

        if (const PICO_STATUS status = ps5000aSetTriggerChannelConditionsV2(_handle, &cond, 1, static_cast<PS5000A_CONDITIONS_INFO>(PS5000A_CLEAR | PS5000A_ADD)); status != PICO_OK) {
            return status;
        }

        PS5000A_DIGITAL_CHANNEL_DIRECTIONS pinTrig;
        pinTrig.channel   = static_cast<PS5000A_DIGITAL_CHANNEL>(pinNumber);
        pinTrig.direction = digitalDirection;

        if (const PICO_STATUS status = ps5000aSetTriggerDigitalPortProperties(_handle, &pinTrig, 1); status != PICO_OK) {
            return status;
        }

        return PICO_OK;
    }
};

static_assert(PicoscopeImplementationLike<Picoscope5000a>);

} // namespace fair::picoscope

#endif
