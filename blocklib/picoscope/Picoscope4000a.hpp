#ifndef FAIR_PICOSCOPE_PICOSCOPE4000A_HPP
#define FAIR_PICOSCOPE_PICOSCOPE4000A_HPP

#include <Picoscope.hpp>

#include <ps4000aApi.h>

namespace fair::picoscope {

template<typename T>
struct Picoscope4000a : public fair::picoscope::Picoscope<T, Picoscope4000a<T>> {
    using super_t = fair::picoscope::Picoscope<T, Picoscope4000a<T>>;

    Picoscope4000a(gr::property_map props) : super_t(std::move(props)) {}

    std::vector<gr::PortOut<T>> out{8};

    using ChannelType            = PS4000A_CHANNEL;
    using ConditionType          = PS4000A_CONDITION;
    using CouplingType           = PS4000A_COUPLING;
    using RangeType              = PS4000A_RANGE;
    using ChannelRangeType       = PICO_CONNECT_PROBE_RANGE;
    using ThresholdDirectionType = PS4000A_THRESHOLD_DIRECTION;
    using TriggerStateType       = PS4000A_TRIGGER_STATE;
    using ConditionsInfoType     = PS4000A_CONDITIONS_INFO;
    using TimeUnitsType          = PS4000A_TIME_UNITS;
    using StreamingReadyType     = ps4000aStreamingReady;
    using BlockReadyType         = ps4000aBlockReady;
    using RatioModeType          = PS4000A_RATIO_MODE;
    using DeviceResolutionType   = PS4000A_DEVICE_RESOLUTION;

    GR_MAKE_REFLECTABLE(Picoscope4000a, out);

    TimeUnitsType convertTimeUnits(TimeUnits tu) const {
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

    [[nodiscard]] constexpr TimebaseResult convertSampleRateToTimebase(int16_t /*handle*/, float desiredFreq) {
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
            return {0, 80'000'000.f};
        } else {

            const auto  timebase   = static_cast<uint32_t>((80'000'000.f / desiredFreq) - 1); // n = (80 MHz / f_S) - 1
            const float actualFreq = 80'000'000.f / static_cast<float>(timebase + 1);
            return {timebase, actualFreq};
        }
    }

    static constexpr std::optional<std::size_t> convertToOutputIndex(std::string_view source) {
        static constexpr std::array<std::pair<std::string_view, std::size_t>, 8> channelMap{{{"A", 0}, {"B", 1}, {"C", 2}, {"D", 3}, {"E", 4}, {"F", 5}, {"G", 6}, {"H", 7}}};

        const auto it = std::ranges::find_if(channelMap, [source](auto&& kv) { return kv.first == source; });
        if (it != channelMap.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    static constexpr std::optional<ChannelType> convertToChannel(std::string_view source) {
        static constexpr std::array<std::pair<std::string_view, ChannelType>, 9> channelMap{{                       //
            {"A", PS4000A_CHANNEL_A}, {"B", PS4000A_CHANNEL_B}, {"C", PS4000A_CHANNEL_C}, {"D", PS4000A_CHANNEL_D}, //
            {"E", PS4000A_CHANNEL_E}, {"F", PS4000A_CHANNEL_F}, {"G", PS4000A_CHANNEL_G}, {"H", PS4000A_CHANNEL_H}, {"EXTERNAL", PS4000A_EXTERNAL}}};

        const auto it = std::ranges::find_if(channelMap, [source](auto&& kv) { return kv.first == source; });
        if (it != channelMap.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    static constexpr std::optional<ChannelType> convertToChannel(std::size_t channelIndex) {
        static constexpr std::array<std::pair<std::size_t, ChannelType>, 9> channelMap{{                            //
            {0UZ, PS4000A_CHANNEL_A}, {1UZ, PS4000A_CHANNEL_B}, {2UZ, PS4000A_CHANNEL_C}, {3UZ, PS4000A_CHANNEL_D}, //
            {4UZ, PS4000A_CHANNEL_E}, {5UZ, PS4000A_CHANNEL_F}, {6UZ, PS4000A_CHANNEL_G}, {7UZ, PS4000A_CHANNEL_H}, {8UZ, PS4000A_EXTERNAL}}};

        const auto it = std::ranges::find_if(channelMap, [channelIndex](auto&& kv) { return kv.first == channelIndex; });
        if (it != channelMap.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    static constexpr CouplingType convertToCoupling(Coupling coupling) {
        if (coupling == Coupling::AC) {
            return PS4000A_AC;
        }
        if (coupling == Coupling::DC) {
            return PS4000A_DC;
        }
        throw std::runtime_error(fmt::format("Unsupported coupling mode: {}", static_cast<int>(coupling)));
    }

    static constexpr RangeType convertToRange(float range) {
        static constexpr std::array<std::pair<float, RangeType>, 14> rangeMap = {{                      //
            {0.01f, PS4000A_10MV}, {0.02f, PS4000A_20MV}, {0.05f, PS4000A_50MV}, {0.1f, PS4000A_100MV}, //
            {0.2f, PS4000A_200MV}, {0.5f, PS4000A_500MV}, {1.f, PS4000A_1V}, {2.f, PS4000A_2V},         //
            {5.f, PS4000A_5V}, {10.f, PS4000A_10V}, {20.f, PS4000A_20V}, {50.f, PS4000A_50V},           //
            {100.f, PS4000A_100V}, {200.f, PS4000A_200V}}};

        const auto exactIt = std::ranges::find_if(rangeMap, [range](auto& kv) { return std::fabs(kv.first - range) < 1e-6f; });
        if (exactIt != rangeMap.end()) {
            return exactIt->second;
        }
        // if no exact match -> find the next higher range
        const auto upperIt = std::ranges::find_if(rangeMap, [range](const auto& kv) { return kv.first > range; });
        return upperIt != rangeMap.end() ? upperIt->second : PS4000A_200V;
    }

    constexpr ThresholdDirectionType convertToThresholdDirection(TriggerDirection direction) {
        using enum TriggerDirection;
        switch (direction) {
        case Rising: return PS4000A_RISING;
        case Falling: return PS4000A_FALLING;
        case Low: return PS4000A_BELOW;
        case High: return PS4000A_ABOVE;
        default: throw std::runtime_error(fmt::format("Unsupported trigger direction: {}", static_cast<int>(direction)));
        }
    };

    constexpr float uncertainty() {
        // https://www.picotech.com/oscilloscope/4000/picoscope-4000-specifications
        return 0.0000045f;
    }

    PICO_STATUS
    setDataBuffer(int16_t handle, ChannelType channel, int16_t* buffer, int32_t bufferLth, uint32_t segmentIndex, RatioModeType mode) { return ps4000aSetDataBuffer(handle, channel, buffer, bufferLth, segmentIndex, mode); }

    PICO_STATUS
    getTimebase2(int16_t handle, uint32_t timebase, int32_t noSamples, float* timeIntervalNanoseconds, int32_t* maxSamples, uint32_t segmentIndex) { return ps4000aGetTimebase2(handle, timebase, noSamples, timeIntervalNanoseconds, maxSamples, segmentIndex); }

    PICO_STATUS
    openUnit(const std::string& serial_number) {
        // take any if serial number is not provided (useful for testing purposes)
        if (serial_number.empty()) {
            return ps4000aOpenUnit(&this->_handle, nullptr);
        } else {
            return ps4000aOpenUnit(&this->_handle, const_cast<int8_t*>(reinterpret_cast<const int8_t*>(serial_number.data())));
        }
    }

    PICO_STATUS
    closeUnit(int16_t handle) { return ps4000aCloseUnit(handle); }

    PICO_STATUS
    changePowerSource(int16_t handle, PICO_STATUS powerstate) { return ps4000aChangePowerSource(handle, powerstate); }

    PICO_STATUS
    maximumValue(int16_t handle, int16_t* value) { return ps4000aMaximumValue(handle, value); }

    PICO_STATUS
    memorySegments(int16_t handle, uint32_t nSegments, int32_t* nMaxSamples) { return ps4000aMemorySegments(handle, nSegments, nMaxSamples); }

    PICO_STATUS
    setNoOfCaptures(int16_t handle, uint32_t nCaptures) { return ps4000aSetNoOfCaptures(handle, nCaptures); }

    PICO_STATUS
    getNoOfCaptures(int16_t handle, uint32_t* nCaptures) const { return ps4000aGetNoOfCaptures(handle, nCaptures); }

    PICO_STATUS
    getNoOfProcessedCaptures(int16_t handle, uint32_t* nProcessedCaptures) const { return ps4000aGetNoOfProcessedCaptures(handle, nProcessedCaptures); }

    PICO_STATUS
    setChannel(int16_t handle, ChannelType channel, int16_t enabled, CouplingType type, ChannelRangeType range, float analogOffset) { return ps4000aSetChannel(handle, channel, enabled, type, range, analogOffset); }

    int maxChannel() { return PS4000A_MAX_CHANNELS; }

    int maxADCCount() { return PS4000A_EXT_MAX_VALUE; }

    CouplingType analogCoupling() { return PS4000A_AC; }

    TriggerStateType conditionDontCare() { return PS4000A_CONDITION_DONT_CARE; }

    ConditionsInfoType conditionsInfoClear() { return PS4000A_CLEAR; }

    PICO_STATUS
    setSimpleTrigger(int16_t handle, int16_t enable, ChannelType source, int16_t threshold, ThresholdDirectionType direction, uint32_t delay, int16_t autoTriggerMs) { return ps4000aSetSimpleTrigger(handle, enable, source, threshold, direction, delay, autoTriggerMs); }

    PICO_STATUS
    setTriggerChannelConditions(int16_t handle, ConditionType* conditions, int16_t nConditions, ConditionsInfoType info) { return ps4000aSetTriggerChannelConditions(handle, conditions, nConditions, info); }

    PICO_STATUS
    setAutoTriggerMicroSeconds(int16_t handle, uint64_t autoTriggerMicroseconds) { return PICO_NOT_SUPPORTED_BY_THIS_DEVICE; }

    PICO_STATUS
    driverStop(int16_t handle) { return ps4000aStop(handle); }

    PICO_STATUS
    runBlock(int16_t handle, int32_t noOfPreTriggerSamples, int32_t noOfPostTriggerSamples, uint32_t timebase, int32_t* timeIndisposed, uint32_t segmentIndex, BlockReadyType ready, void* param) { return ps4000aRunBlock(handle, noOfPreTriggerSamples, noOfPostTriggerSamples, timebase, timeIndisposed, segmentIndex, ready, param); }

    PICO_STATUS
    runStreaming(int16_t handle, uint32_t* sampleInterval, TimeUnitsType timeUnits, uint32_t maxPreTriggerSamples, uint32_t maxPostTriggerSamples, int16_t autoStop, uint32_t downSampleRatio, RatioModeType downSampleRatioMode, uint32_t overviewBufferSize) { return ps4000aRunStreaming(handle, sampleInterval, timeUnits, maxPreTriggerSamples, maxPostTriggerSamples, autoStop, downSampleRatio, downSampleRatioMode, overviewBufferSize); }

    RatioModeType ratioNone() { return PS4000A_RATIO_MODE_NONE; }

    PICO_STATUS
    getStreamingLatestValues(int16_t handle, StreamingReadyType ready, void* param) { return ps4000aGetStreamingLatestValues(handle, ready, param); }

    PICO_STATUS
    getValues(int16_t handle, uint32_t startIndex, uint32_t* noOfSamples, uint32_t downSampleRatio, RatioModeType downSampleRatioMode, uint32_t segmentIndex, int16_t* overflow) { return ps4000aGetValues(handle, startIndex, noOfSamples, downSampleRatio, downSampleRatioMode, segmentIndex, overflow); }

    PICO_STATUS
    getUnitInfo(int16_t handle, int8_t* string, int16_t stringLength, int16_t* requiredSize, PICO_INFO info) const { return ps4000aGetUnitInfo(handle, string, stringLength, requiredSize, info); }

    PICO_STATUS
    getDeviceResolution(int16_t handle, DeviceResolutionType* deviceResolution) const { return ps4000aGetDeviceResolution(handle, deviceResolution); }
};

} // namespace fair::picoscope

#endif
