#ifndef FAIR_PICOSCOPE_PICOSCOPE3000A_HPP
#define FAIR_PICOSCOPE_PICOSCOPE3000A_HPP

#include <fair/picoscope/PicoscopeAPI.hpp>

#include <ps3000aApi.h>

namespace fair::picoscope {
// These structures are not available for 3000A API.
// We implemented them for consistency.
typedef struct tPS3000ACondition {
    PS3000A_CHANNEL       source;
    PS3000A_TRIGGER_STATE condition;
} PS3000A_CONDITION;

typedef enum enPS3000AConditionsInfo { PS3000A_CLEAR = 0x00000001, PS3000A_ADD = 0x00000002 } PS3000A_CONDITIONS_INFO;

typedef enum enPS3000ADeviceResolution { PS3000A_DR_8BIT } PS3000A_DEVICE_RESOLUTION;

struct Picoscope3000a {
    static constexpr std::size_t                                  N_DIGITAL_CHANNELS = 16UZ;
    static constexpr std::size_t                                  N_ANALOG_CHANNELS  = PS3000A_MAX_CHANNELS;
    static constexpr std::underlying_type_t<PS3000A_DIGITAL_PORT> DIGI_PORT_0        = PS3000A_DIGITAL_PORT0;

    using ChannelType            = PS3000A_CHANNEL;
    using ConditionType          = PS3000A_CONDITION;
    using CouplingType           = PS3000A_COUPLING;
    using RangeType              = PS3000A_RANGE;
    using ChannelRangeType       = RangeType;
    using ThresholdDirectionType = PS3000A_THRESHOLD_DIRECTION;
    using DigitalDirectionType   = PS3000A_DIGITAL_DIRECTION;
    using DigitalDirectionsType  = PS3000A_DIGITAL_CHANNEL_DIRECTIONS;
    using TriggerStateType       = PS3000A_TRIGGER_STATE;
    using TriggerConditionsType  = PS3000A_TRIGGER_CONDITIONS_V2;
    using ConditionsInfoType     = PS3000A_CONDITIONS_INFO;
    using TimeUnitsType          = PS3000A_TIME_UNITS;
    using StreamingReadyType     = ps3000aStreamingReady;
    using BlockReadyType         = ps3000aBlockReady;
    using RatioModeType          = PS3000A_RATIO_MODE;
    using DeviceResolutionType   = PS3000A_DEVICE_RESOLUTION;
    using NSamplesType           = int32_t;

    static constexpr std::array<std::pair<ChannelName, ChannelType>, 5> outputs{{
        {ChannelName::A, PS3000A_CHANNEL_A},
        {ChannelName::B, PS3000A_CHANNEL_B},
        {ChannelName::C, PS3000A_CHANNEL_C},
        {ChannelName::D, PS3000A_CHANNEL_D},
        {ChannelName::EXTERNAL, PS3000A_EXTERNAL},
    }};

    static constexpr std::array<std::pair<AnalogChannelRange, RangeType>, 12> ranges = {{
        {AnalogChannelRange::ps10mV, PS3000A_10MV},
        {AnalogChannelRange::ps20mV, PS3000A_20MV},
        {AnalogChannelRange::ps50mV, PS3000A_50MV},
        {AnalogChannelRange::ps100mV, PS3000A_100MV},
        {AnalogChannelRange::ps200mV, PS3000A_200MV},
        {AnalogChannelRange::ps500mV, PS3000A_500MV},
        {AnalogChannelRange::ps1V, PS3000A_1V},
        {AnalogChannelRange::ps2V, PS3000A_2V},
        {AnalogChannelRange::ps5V, PS3000A_5V},
        {AnalogChannelRange::ps10V, PS3000A_10V},
        {AnalogChannelRange::ps20V, PS3000A_20V},
        {AnalogChannelRange::ps50V, PS3000A_50V},
    }};

    static TimeUnitsType convertTimeUnits(TimeUnits tu) {
        switch (tu) {
        case TimeUnits::fs: return PS3000A_FS;
        case TimeUnits::ps: return PS3000A_PS;
        case TimeUnits::ns: return PS3000A_NS;
        case TimeUnits::us: return PS3000A_US;
        case TimeUnits::ms: return PS3000A_MS;
        case TimeUnits::s: return PS3000A_S;
        }
        return PS3000A_MAX_TIME_UNITS;
    }

    static constexpr RatioModeType ratioNone{PS3000A_RATIO_MODE_NONE};

    int16_t _handle;

    [[nodiscard]] static constexpr std::expected<TimebaseResult, Error> convertSampleRateToTimebase(const float desiredFreq) {
        // https://www.picotech.com/download/manuals/picoscope-3000-series-a-api-programmers-guide.pdf, page 15

        // For PicoScope 3000A / 3000D Series
        // -----------------------------------------------------------------------------------------
        // | Timebase (n)| Sampling Interval (tS)                       | Sampling Frequency (fS)  |
        // |-------------|----------------------------------------------|--------------------------|
        // |     0       | 1 ns   2^0 / 1 GHz                           | 1 GHz                    |
        // |     1       | 2 ns   2^1 / 1 GHz                           | 500 MHz                  |
        // |     2       | 4 ns   2^2 / 1 GHz                           | 250 MHz                  |
        // |     3       | 8 ns   (n - 2)/125 MHz                       | 125 MHz                  |
        // |    ...      | ...                                          | ...                      |
        // | 2^32 - 1    | ~34.4 s ((2^32 - 1) - 2)/125 MHz             | ~0.029 Hz                |
        // -----------------------------------------------------------------------------------------
        //  Notes:
        //   • The maximum sampling rate (minimum tS) depends on the number of enabled channels
        //     and digital ports. See the PicoScope 3000 Series data sheet for details.
        //   • In streaming mode, USB speed and the specific oscilloscope model may further limit
        //     the maximum real-time sampling rate.

        if (desiredFreq <= 0.f || desiredFreq >= 1.e9f) {
            return TimebaseResult{0, 1.e9f};
        } else if (desiredFreq >= 500.e6f) {
            return TimebaseResult{1, 500.e6f};
        } else if (desiredFreq >= 250.e6f) {
            return TimebaseResult{2, 250.e6f};
        } else {
            const auto  timebase   = static_cast<uint32_t>((125.e6f / desiredFreq) + 2.f); // n = (125 MHz / f_S) + 2;
            const float actualFreq = 125.e6f / static_cast<float>(timebase - 2);
            return TimebaseResult{timebase, actualFreq};
        }
    }

    static constexpr std::expected<CouplingType, Error> convertToCoupling(Coupling coupling) {
        if (coupling == Coupling::AC) {
            return PS3000A_AC;
        } else if (coupling == Coupling::DC) {
            return PS3000A_DC;
        }
        return std::unexpected(Error(std::format("Unsupported coupling mode: {}", static_cast<int>(coupling))));
    }

    static constexpr std::expected<ThresholdDirectionType, Error> convertToThresholdDirection(TriggerDirection direction) {
        using enum TriggerDirection;
        switch (direction) {
        case Rising: return PS3000A_RISING;
        case Falling: return PS3000A_FALLING;
        case Low: return PS3000A_BELOW;
        case High: return PS3000A_ABOVE;
        default: return std::unexpected(Error(std::format("Unsupported trigger direction: {}", static_cast<int>(direction))));
        }
    };

    static constexpr float uncertainty() {
        // TODO: https://www.picotech.com/oscilloscope/3000/picoscope-3000-oscilloscope-specifications
        return 0.0000160f;
    }

    PICO_STATUS setDataBuffer(ChannelType channel, int16_t* buffer, int32_t bufferLth, RatioModeType mode) const { return ps3000aSetDataBuffer(_handle, channel, buffer, bufferLth, 0UZ, mode); }

    PICO_STATUS setDataBuffer(ChannelType channel, int16_t* buffer, int32_t bufferLth, uint32_t segmentIndex, RatioModeType mode) const { return ps3000aSetDataBuffer(_handle, channel, buffer, bufferLth, segmentIndex, mode); }

    PICO_STATUS getTimebase2(uint32_t timebase, int32_t noSamples, float* timeIntervalNanoseconds, int32_t* maxSamples, uint32_t segmentIndex) const { return ps3000aGetTimebase2(_handle, timebase, noSamples, timeIntervalNanoseconds, /* oversample */ 0, maxSamples, segmentIndex); }

    PICO_STATUS openUnit(const std::string& serial_number) {
        // take any if the serial number is not provided (useful for testing purposes)
        if (serial_number.empty()) {
            return ps3000aOpenUnit(&_handle, nullptr);
        } else {
            return ps3000aOpenUnit(&_handle, const_cast<int8_t*>(reinterpret_cast<const int8_t*>(serial_number.data())));
        }
    }

    static PICO_STATUS openUnitAsync(std::int16_t* status, const std::string& serial_number) {
        if (serial_number.empty()) {
            return ps3000aOpenUnitAsync(status, nullptr);
        } else {
            return ps3000aOpenUnitAsync(status, const_cast<int8_t*>(reinterpret_cast<const int8_t*>(serial_number.data())));
        }
    }

    PICO_STATUS openUnitProgress(std::int16_t* progressPercent, std::int16_t* complete) { return ps3000aOpenUnitProgress(&_handle, progressPercent, complete); }

    [[nodiscard]] PICO_STATUS closeUnit() const { return ps3000aCloseUnit(_handle); }

    [[nodiscard]] bool isOpened() const { return _handle > 0; }

    [[nodiscard]] PICO_STATUS changePowerSource(PICO_STATUS powerstate) const { return ps3000aChangePowerSource(_handle, powerstate); }

    PICO_STATUS maximumValue(int16_t* value) const { return ps3000aMaximumValue(_handle, value); }

    PICO_STATUS memorySegments(uint32_t nSegments, int32_t* nMaxSamples) const { return ps3000aMemorySegments(_handle, nSegments, nMaxSamples); }

    [[nodiscard]] PICO_STATUS setNoOfCaptures(uint32_t nCaptures) const { return ps3000aSetNoOfCaptures(_handle, nCaptures); }

    PICO_STATUS getNoOfCaptures(uint32_t* nCaptures) const { return ps3000aGetNoOfCaptures(_handle, nCaptures); }

    PICO_STATUS getNoOfProcessedCaptures(uint32_t* nProcessedCaptures) const { return ps3000aGetNoOfProcessedCaptures(_handle, nProcessedCaptures); }

    [[nodiscard]] PICO_STATUS setChannel(ChannelType channel, int16_t enabled, CouplingType type, ChannelRangeType range, float analogOffset) const { return ps3000aSetChannel(_handle, channel, enabled, type, range, analogOffset); }

    static int maxChannel() { return PS3000A_MAX_CHANNELS; }

    static int extTriggerMaxValue() { return PS3000A_EXT_MAX_VALUE; }
    static int extTriggerMinValue() { return PS3000A_EXT_MIN_VALUE; }

    static float extTriggerMaxValueVoltage() { return 5.f; }
    static float extTriggerMinValueVoltage() { return -5.f; }

    static CouplingType analogCoupling() { return PS3000A_AC; }

    static TriggerStateType conditionDontCare() { return PS3000A_CONDITION_DONT_CARE; }

    static ConditionsInfoType conditionsInfoClear() { return PS3000A_CLEAR; }
    static ConditionsInfoType conditionsInfoAdd() { return PS3000A_ADD; }

    [[nodiscard]] PICO_STATUS setSimpleTrigger(int16_t enable, ChannelType source, int16_t threshold, ThresholdDirectionType direction, uint32_t delay, int16_t autoTriggerMs) const { return ps3000aSetSimpleTrigger(_handle, enable, source, threshold, direction, delay, autoTriggerMs); }

    static PS3000A_TRIGGER_CONDITIONS convertToTriggerConditions(const ConditionType* conditions, int16_t nConditions) {
        PS3000A_TRIGGER_CONDITIONS result{};
        for (int16_t i = 0; i < nConditions; ++i) {
            if (const auto& [source, condition] = conditions[i]; source == PS3000A_CHANNEL_A) {
                result.channelA = condition;
            } else if (source == PS3000A_CHANNEL_B) {
                result.channelB = condition;
            } else if (source == PS3000A_CHANNEL_C) {
                result.channelC = condition;
            } else if (source == PS3000A_CHANNEL_D) {
                result.channelD = condition;
            } else if (source == PS3000A_EXTERNAL) {
                result.external = condition;
            } else if (source == PS3000A_TRIGGER_AUX) {
                result.aux = condition;
            }
        }
        return result;
    }

    PICO_STATUS setTriggerChannelConditions(const ConditionType* conditions, int16_t nConditions, ConditionsInfoType) const {
        PS3000A_TRIGGER_CONDITIONS ps3000aConditions = convertToTriggerConditions(conditions, nConditions);
        return ps3000aSetTriggerChannelConditions(_handle, &ps3000aConditions, 1);
    }

    [[nodiscard]] PICO_STATUS driverStop() const { return ps3000aStop(_handle); }

    PICO_STATUS runBlock(int32_t noOfPreTriggerSamples, int32_t noOfPostTriggerSamples, uint32_t timebase, int32_t* timeIndisposed, uint32_t segmentIndex, BlockReadyType ready, void* param) const { return ps3000aRunBlock(_handle, noOfPreTriggerSamples, noOfPostTriggerSamples, timebase, /* oversample, not used*/ 0, timeIndisposed, segmentIndex, ready, param); }

    PICO_STATUS runStreaming(uint32_t* sampleInterval, TimeUnitsType timeUnits, uint32_t maxPreTriggerSamples, uint32_t maxPostTriggerSamples, int16_t autoStop, uint32_t downSampleRatio, RatioModeType downSampleRatioMode, uint32_t overviewBufferSize) const { return ps3000aRunStreaming(_handle, sampleInterval, timeUnits, maxPreTriggerSamples, maxPostTriggerSamples, autoStop, downSampleRatio, downSampleRatioMode, overviewBufferSize); }

    PICO_STATUS getStreamingLatestValues(StreamingReadyType ready, void* param) const { return ps3000aGetStreamingLatestValues(_handle, ready, param); }

    PICO_STATUS getValues(uint32_t startIndex, uint32_t* noOfSamples, uint32_t downSampleRatio, RatioModeType downSampleRatioMode, uint32_t segmentIndex, int16_t* overflow) const { return ps3000aGetValues(_handle, startIndex, noOfSamples, downSampleRatio, downSampleRatioMode, segmentIndex, overflow); }

    PICO_STATUS getValuesBulk(uint32_t* noOfSamples, uint32_t fromSegmentIndex, uint32_t toSegmentIndex, uint32_t downSampleRatio, RatioModeType downSampleRatioMode, int16_t* overflow) const { return ps3000aGetValuesBulk(_handle, noOfSamples, fromSegmentIndex, toSegmentIndex, downSampleRatio, downSampleRatioMode, overflow); }

    PICO_STATUS getValuesOverlappedBulk(uint32_t startIndex, uint32_t* noOfSamples, uint32_t downSampleRatio, RatioModeType downSampleRatioMode, uint32_t fromSegmentIndex, uint32_t toSegmentIndex, int16_t* overflow) const { return ps3000aGetValuesOverlappedBulk(_handle, startIndex, noOfSamples, downSampleRatio, downSampleRatioMode, fromSegmentIndex, toSegmentIndex, overflow); }

    PICO_STATUS isReady(int16_t* ready) const { return ps3000aIsReady(_handle, ready); }

    PICO_STATUS getValuesTriggerTimeOffsetBulk64(int64_t* times, TimeUnitsType* timeUnits, uint32_t fromSegmentIndex, uint32_t toSegmentIndex) const { return ps3000aGetValuesTriggerTimeOffsetBulk64(_handle, times, timeUnits, fromSegmentIndex, toSegmentIndex); }

    PICO_STATUS getUnitInfo(int8_t* string, int16_t stringLength, int16_t* requiredSize, PICO_INFO info) const { return ps3000aGetUnitInfo(_handle, string, stringLength, requiredSize, info); }

    static PICO_STATUS getDeviceResolution(int16_t, DeviceResolutionType*) { return PICO_NOT_SUPPORTED_BY_THIS_DEVICE; }

    [[nodiscard]] PICO_STATUS setDigitalPorts(int16_t enabled, int16_t digitalPortThreshold) const {
        for (std::size_t i = 0; i < 2; i++) {
            const PS3000A_DIGITAL_PORT channelId = i == 0 ? PS3000A_DIGITAL_PORT0 : PS3000A_DIGITAL_PORT1;
            if (const PICO_STATUS status = ps3000aSetDigitalPort(_handle, channelId, enabled, digitalPortThreshold); status != PICO_OK) {
                return status;
            }
        }
        return PICO_OK;
    }

    [[nodiscard]] PICO_STATUS setTriggerDigitalPort(uint pinNumber, TriggerDirection direction) const {
        if (pinNumber > 15U) {
            return PICO_INVALID_DIGITAL_CHANNEL;
        }
        PS3000A_DIGITAL_DIRECTION digitalDirection;
        switch (direction) {
        case TriggerDirection::Rising: digitalDirection = PS3000A_DIGITAL_DIRECTION_RISING; break;
        case TriggerDirection::Falling: digitalDirection = PS3000A_DIGITAL_DIRECTION_FALLING; break;
        case TriggerDirection::Low: digitalDirection = PS3000A_DIGITAL_DIRECTION_LOW; break;
        case TriggerDirection::High: digitalDirection = PS3000A_DIGITAL_DIRECTION_HIGH; break;
        default: return PICO_INVALID_DIGITAL_TRIGGER_DIRECTION;
        }

        PS3000A_TRIGGER_CONDITIONS_V2 ps3000aConditions = {
            PS3000A_CONDITION_DONT_CARE, // channelA;
            PS3000A_CONDITION_DONT_CARE, // channelB;
            PS3000A_CONDITION_DONT_CARE, // channelC;
            PS3000A_CONDITION_DONT_CARE, // channelD;
            PS3000A_CONDITION_DONT_CARE, // external;
            PS3000A_CONDITION_DONT_CARE, // aux;
            PS3000A_CONDITION_DONT_CARE, // pulseWidthQualifier;
            PS3000A_CONDITION_TRUE,      // digital;
        };
        if (const PICO_STATUS status = ps3000aSetTriggerChannelConditionsV2(_handle, &ps3000aConditions, 1); status != PICO_OK) {
            return status;
        }

        PS3000A_DIGITAL_CHANNEL_DIRECTIONS pinTrig;
        pinTrig.channel   = static_cast<PS3000A_DIGITAL_CHANNEL>(pinNumber);
        pinTrig.direction = digitalDirection;
        if (const PICO_STATUS status = ps3000aSetTriggerDigitalPortProperties(_handle, &pinTrig, 1); status != PICO_OK) {
            return status;
        }
        return PICO_OK;
    }

    static PICO_STATUS enumerateUnits(int16_t* count, int8_t* serials, int16_t* serialsSize) { return ps3000aEnumerateUnits(count, serials, serialsSize); };
};

static_assert(PicoscopeImplementationLike<Picoscope3000a>);

} // namespace fair::picoscope

#endif
