#ifndef FAIR_PICOSCOPE_PICOSCOPE6000_HPP
#define FAIR_PICOSCOPE_PICOSCOPE6000_HPP

#include <Picoscope.hpp>

#include <ps6000Api.h>

namespace fair::picoscope {

// These structures are not available for 6000 API.
// We implemented them for consistency.
typedef struct tPS6000Condition {
    PS6000_CHANNEL       source;
    PS6000_TRIGGER_STATE condition;
} PS6000_CONDITION;

typedef enum enPS6000ConditionsInfo { PS6000_CLEAR = 0x00000001, PS6000_ADD = 0x00000002 } PS6000_CONDITIONS_INFO;

typedef enum enPS6000DeviceResolution { PS6000A_DR_8BIT } PS6000_DEVICE_RESOLUTION;

template<typename T>
struct Picoscope6000 : public fair::picoscope::Picoscope<T, Picoscope6000<T>> {
    using super_t = fair::picoscope::Picoscope<T, Picoscope6000<T>>;

    Picoscope6000(gr::property_map props) : super_t(std::move(props)) {}

    std::vector<gr::PortOut<T>> out{4};

    GR_MAKE_REFLECTABLE(Picoscope6000, out);

private:
    std::array<std::vector<int16_t>, 2> _digitalBuffers;

public:
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
    using NSamplesType           = uint32_t;

    TimeUnitsType convertTimeUnits(TimeUnits tu) const {
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

    [[nodiscard]] constexpr TimebaseResult convertSampleRateToTimebase(int16_t /*handle*/, float desiredFreq) {
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
            return {0, 5.0e9f}; // timebase = 0 → 200 ps → 5 GHz
        } else if (desiredFreq >= 2.5e9f) {
            return {1, 2.5e9f}; // timebase = 1 → 400 ps → 2.5 GHz
        } else if (desiredFreq >= 1.25e9f) {
            return {2, 1.25e9f}; // timebase = 2 → 800 ps → 1.25 GHz
        } else if (desiredFreq >= 625.0e6f) {
            return {3, 625.0e6f}; // timebase = 3 → 1.6 ns → 625 MHz
        } else if (desiredFreq >= 312.5e6f) {
            return {4, 312.5e6f}; // timebase = 4 → 3.2 ns → 312.5 MHz
        } else {
            const auto  timebase   = static_cast<std::uint32_t>((156.25e6f / desiredFreq) + 4.0f); // n  = (156.25e6 / desiredFreq) + 4
            const float actualFreq = 156.25e6f / (static_cast<float>(timebase) - 4.0f);
            return {timebase, actualFreq};
        }
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
        static constexpr std::array<std::pair<std::string_view, ChannelType>, 6> channelMap{{//
            {"A", PS6000_CHANNEL_A}, {"B", PS6000_CHANNEL_B}, {"C", PS6000_CHANNEL_C}, {"D", PS6000_CHANNEL_D}, {"EXTERNAL", PS6000_EXTERNAL}, {"AUX", PS6000_TRIGGER_AUX}}};

        const auto it = std::ranges::find_if(channelMap, [source](auto&& kv) { return kv.first == source; });
        if (it != channelMap.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    static constexpr std::optional<ChannelType> convertToChannel(std::size_t channelIndex) {
        static constexpr std::array<std::pair<std::size_t, ChannelType>, 6> channelMap{{//
            {0UZ, PS6000_CHANNEL_A}, {1UZ, PS6000_CHANNEL_B}, {2UZ, PS6000_CHANNEL_C}, {3UZ, PS6000_CHANNEL_D}, {4UZ, PS6000_EXTERNAL}, {5UZ, PS6000_TRIGGER_AUX}}};

        const auto it = std::ranges::find_if(channelMap, [channelIndex](auto&& kv) { return kv.first == channelIndex; });
        if (it != channelMap.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    static constexpr CouplingType convertToCoupling(Coupling coupling) {
        if (coupling == Coupling::AC) {
            return PS6000_AC;
        } else if (coupling == Coupling::DC) {
            return PS6000_DC_1M;
        } else if (coupling == Coupling::DC_50R) {
            return PS6000_DC_50R;
        }
        throw std::runtime_error(fmt::format("Unsupported coupling mode: {}", static_cast<int>(coupling)));
    }

    static constexpr RangeType convertToRange(float range) {
        static constexpr std::array<std::pair<float, RangeType>, 12> rangeMap = {{                  //
            {0.01f, PS6000_10MV}, {0.02f, PS6000_20MV}, {0.05f, PS6000_50MV}, {0.1f, PS6000_100MV}, //
            {0.2f, PS6000_200MV}, {0.5f, PS6000_500MV}, {1.0f, PS6000_1V}, {2.0f, PS6000_2V},       //
            {5.0f, PS6000_5V}, {10.0f, PS6000_10V}, {20.0f, PS6000_20V}, {50.0f, PS6000_50V}}};

        const auto exactIt = std::ranges::find_if(rangeMap, [range](auto& kv) { return std::fabs(kv.first - range) < 1e-6f; });
        if (exactIt != rangeMap.end()) {
            return exactIt->second;
        }
        // if no exact match -> find the next higher range
        const auto upperIt = std::ranges::find_if(rangeMap, [range](const auto& kv) { return kv.first > range; });
        return upperIt != rangeMap.end() ? upperIt->second : PS6000_50V;
    }

    constexpr ThresholdDirectionType convertToThresholdDirection(TriggerDirection direction) {
        using enum TriggerDirection;
        switch (direction) {
        case Rising: return PS6000_RISING;
        case Falling: return PS6000_FALLING;
        case Low: return PS6000_BELOW;
        case High: return PS6000_ABOVE;
        default: throw std::runtime_error(fmt::format("Unsupported trigger direction: {}", static_cast<int>(direction)));
        }
    };

    constexpr float uncertainty() {
        // TODO https://www.picotech.com/oscilloscope/6000/picoscope-6000-specifications
        return 0.0000200f;
    }

    PICO_STATUS
    setDataBuffer(int16_t handle, ChannelType channel, int16_t* buffer, int32_t bufferLth, RatioModeType mode) { return ps6000SetDataBuffer(handle, channel, buffer, static_cast<uint32_t>(bufferLth), mode); }

    PICO_STATUS
    setDataBufferForSegment(int16_t handle, ChannelType channel, int16_t* buffer, int32_t bufferLth, uint32_t segmentIndex, RatioModeType mode) { return ps6000SetDataBufferBulk(handle, channel, buffer, static_cast<uint32_t>(bufferLth), segmentIndex, mode); }

    PICO_STATUS
    getTimebase2(int16_t handle, uint32_t timebase, int32_t noSamples, float* timeIntervalNanoseconds, int32_t* maxSamples, uint32_t segmentIndex) {
        // Need to do convertion because picoscope 6000 API requires uint32_t, other APIs requires int32_t
        uint32_t          maxSamplesU = static_cast<uint32_t>(*maxSamples);
        const PICO_STATUS status      = ps6000GetTimebase2(handle, timebase, static_cast<uint32_t>(noSamples), timeIntervalNanoseconds, /* oversample */ 0, &maxSamplesU, segmentIndex);
        *maxSamples                   = static_cast<int32_t>(maxSamplesU);
        return status;
    }

    PICO_STATUS
    openUnit(const std::string& serial_number) {
        // take any if serial number is not provided (useful for testing purposes)
        if (serial_number.empty()) {
            return ps6000OpenUnit(&(this->_handle), nullptr);
        } else {
            return ps6000OpenUnit(&(this->_handle), const_cast<int8_t*>(reinterpret_cast<const int8_t*>(serial_number.data())));
        }
    }

    PICO_STATUS
    closeUnit(int16_t handle) { return ps6000CloseUnit(handle); }

    PICO_STATUS
    changePowerSource(int16_t, PICO_STATUS) { return PICO_NOT_SUPPORTED_BY_THIS_DEVICE; }

    PICO_STATUS
    maximumValue(int16_t, int16_t* value) {
        *value = PS6000_MAX_VALUE;
        return PICO_OK;
    }

    PICO_STATUS
    memorySegments(int16_t handle, uint32_t nSegments, int32_t* nMaxSamples) {
        // Need to do convertion because picoscope 6000 API requires uint32_t, other APIs requires int32_t
        uint32_t          nMaxSamplesU = static_cast<uint32_t>(*nMaxSamples);
        const PICO_STATUS status       = ps6000MemorySegments(handle, nSegments, &nMaxSamplesU);
        *nMaxSamples                   = static_cast<int32_t>(nMaxSamplesU);
        return status;
    }

    PICO_STATUS
    setNoOfCaptures(int16_t handle, uint32_t nCaptures) { return ps6000SetNoOfCaptures(handle, nCaptures); }

    PICO_STATUS
    getNoOfCaptures(int16_t handle, uint32_t* nCaptures) const { return ps6000GetNoOfCaptures(handle, nCaptures); }

    PICO_STATUS
    getNoOfProcessedCaptures(int16_t handle, uint32_t* nProcessedCaptures) const { return ps6000GetNoOfProcessedCaptures(handle, nProcessedCaptures); }

    PICO_STATUS
    setChannel(int16_t handle, ChannelType channel, int16_t enabled, CouplingType type, ChannelRangeType range, float analogOffset) { return ps6000SetChannel(handle, channel, enabled, type, range, analogOffset, PS6000_BW_FULL); }

    int maxChannel() { return PS6000_MAX_CHANNELS; }

    int extTriggerMaxValue() { return 32767; } // TODO: taken from 3000a

    int extTriggerMinValue() { return -32767; } // TODO: taken from 3000a

    float extTriggerMaxValueVoltage() { return 5.f; }

    float extTriggerMinValueVoltage() { return -5.f; }

    CouplingType analogCoupling() { return PS6000_AC; }

    TriggerStateType conditionDontCare() { return PS6000_CONDITION_DONT_CARE; }

    ConditionsInfoType conditionsInfoClear() { return PS6000_CLEAR; }

    ConditionsInfoType conditionsInfoAdd() { return PS6000_ADD; }

    PICO_STATUS
    setSimpleTrigger(int16_t handle, int16_t enable, ChannelType source, int16_t threshold, ThresholdDirectionType direction, uint32_t delay, int16_t autoTriggerMs) { return ps6000SetSimpleTrigger(handle, enable, source, threshold, direction, delay, autoTriggerMs); }

    PS6000_TRIGGER_CONDITIONS
    convertToTriggerConditions(ConditionType* conditions, int16_t nConditions) {
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

    PICO_STATUS
    setTriggerChannelConditions(int16_t handle, ConditionType* conditions, int16_t nConditions, ConditionsInfoType) {
        PS6000_TRIGGER_CONDITIONS conds = convertToTriggerConditions(conditions, nConditions);
        return ps6000SetTriggerChannelConditions(handle, &conds, 1);
    }

    PICO_STATUS
    driverStop(int16_t handle) { return ps6000Stop(handle); }

    PICO_STATUS
    runBlock(int16_t handle, int32_t noOfPreTriggerSamples, int32_t noOfPostTriggerSamples, uint32_t timebase, int32_t* timeIndisposed, uint32_t segmentIndex, BlockReadyType ready, void* param) { //
        return ps6000RunBlock(handle, static_cast<uint32_t>(noOfPreTriggerSamples), static_cast<uint32_t>(noOfPostTriggerSamples), timebase, /* oversample, not used*/ 0, timeIndisposed, segmentIndex, ready, param);
    }

    PICO_STATUS
    runStreaming(int16_t handle, uint32_t* sampleInterval, TimeUnitsType timeUnits, uint32_t maxPreTriggerSamples, uint32_t maxPostTriggerSamples, int16_t autoStop, uint32_t downSampleRatio, RatioModeType downSampleRatioMode, uint32_t overviewBufferSize) { //
        return ps6000RunStreaming(handle, sampleInterval, timeUnits, maxPreTriggerSamples, maxPostTriggerSamples, autoStop, downSampleRatio, downSampleRatioMode, overviewBufferSize);
    }

    RatioModeType ratioNone() { return PS6000_RATIO_MODE_NONE; }

    PICO_STATUS
    getStreamingLatestValues(int16_t handle, StreamingReadyType ready, void* param) { return ps6000GetStreamingLatestValues(handle, ready, param); }

    PICO_STATUS
    getValues(int16_t handle, uint32_t startIndex, uint32_t* noOfSamples, uint32_t downSampleRatio, RatioModeType downSampleRatioMode, uint32_t segmentIndex, int16_t* overflow) { return ps6000GetValues(handle, startIndex, noOfSamples, downSampleRatio, downSampleRatioMode, segmentIndex, overflow); }

    PICO_STATUS
    getUnitInfo(int16_t handle, int8_t* string, int16_t stringLength, int16_t* requiredSize, PICO_INFO info) const { return ps6000GetUnitInfo(handle, string, stringLength, requiredSize, info); }

    PICO_STATUS
    getDeviceResolution(int16_t, DeviceResolutionType*) const { return PICO_NOT_SUPPORTED_BY_THIS_DEVICE; }

    // Digital picoscope inputs, not supported by picoscope 6000
    [[nodiscard]] PICO_STATUS setDigitalPorts() { return PICO_OK; }
    [[nodiscard]] PICO_STATUS setDigitalBuffers(size_t, uint32_t) { return PICO_OK; }
    void                      copyDigitalBuffersToOutput(std::span<std::uint16_t>, std::size_t) {}
    [[nodiscard]] PICO_STATUS SetTriggerDigitalPort(int16_t, int, TriggerDirection) { return PICO_OK; }
};

} // namespace fair::picoscope

#endif
