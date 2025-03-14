#ifndef FAIR_PICOSCOPE_PICOSCOPE3000A_HPP
#define FAIR_PICOSCOPE_PICOSCOPE3000A_HPP

#include <Picoscope.hpp>

#include <ps3000aApi.h>

namespace fair::picoscope {

// These structures are not available for 3000A API.
// We implemented them for consistency.
typedef struct tPS3000ACondition {
    PS3000A_CHANNEL       source;
    PS3000A_TRIGGER_STATE condition;
} PS3000A_CONDITION;

typedef enum enPS4000AConditionsInfo { PS3000A_CLEAR = 0x00000001, PS3000A_ADD = 0x00000002 } PS3000A_CONDITIONS_INFO;

typedef enum enPS3000ADeviceResolution { PS3000A_DR_8BIT } PS3000A_DEVICE_RESOLUTION;

template<typename T>
struct Picoscope3000a : public fair::picoscope::Picoscope<T, Picoscope3000a<T>> {
    using super_t = fair::picoscope::Picoscope<T, Picoscope3000a<T>>;

    Picoscope3000a(gr::property_map props) : super_t(std::move(props)) {}

    std::vector<gr::PortOut<T>> out{4};

    GR_MAKE_REFLECTABLE(Picoscope3000a, out);

private:
    std::array<std::vector<int16_t>, 2> _digitalBuffers;

public:
    using ChannelType            = PS3000A_CHANNEL;
    using ConditionType          = PS3000A_CONDITION;
    using CouplingType           = PS3000A_COUPLING;
    using RangeType              = PS3000A_RANGE;
    using ChannelRangeType       = RangeType;
    using ThresholdDirectionType = PS3000A_THRESHOLD_DIRECTION;
    using TriggerStateType       = PS3000A_TRIGGER_STATE;
    using ConditionsInfoType     = PS3000A_CONDITIONS_INFO;
    using TimeUnitsType          = PS3000A_TIME_UNITS;
    using StreamingReadyType     = ps3000aStreamingReady;
    using BlockReadyType         = ps3000aBlockReady;
    using RatioModeType          = PS3000A_RATIO_MODE;
    using DeviceResolutionType   = PS3000A_DEVICE_RESOLUTION;

    TimeUnitsType convertTimeUnits(TimeUnits tu) const {
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

    [[nodiscard]] constexpr TimebaseResult convertSampleRateToTimebase(int16_t /*handle*/, float desiredFreq) {
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
            return {0, 1.e9f};
        } else if (desiredFreq >= 500.e6f) {
            return {1, 500.e6f};
        } else if (desiredFreq >= 250.e6f) {
            return {2, 250.e6f};
        } else {
            const auto  timebase   = static_cast<uint32_t>((125.e6f / desiredFreq) + 2); // n = (125 MHz / f_S) + 2;
            const float actualFreq = 125.e6f / static_cast<float>(timebase - 2);
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
        static constexpr std::array<std::pair<std::string_view, ChannelType>, 5> channelMap{{//
            {"A", PS3000A_CHANNEL_A}, {"B", PS3000A_CHANNEL_B}, {"C", PS3000A_CHANNEL_C}, {"D", PS3000A_CHANNEL_D}, {"EXTERNAL", PS3000A_EXTERNAL}}};

        const auto it = std::ranges::find_if(channelMap, [source](auto&& kv) { return kv.first == source; });
        if (it != channelMap.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    static constexpr std::optional<ChannelType> convertToChannel(std::size_t channelIndex) {
        static constexpr std::array<std::pair<std::size_t, ChannelType>, 5> channelMap{{//
            {0UZ, PS3000A_CHANNEL_A}, {1UZ, PS3000A_CHANNEL_B}, {2UZ, PS3000A_CHANNEL_C}, {3UZ, PS3000A_CHANNEL_D}, {4UZ, PS3000A_EXTERNAL}}};

        const auto it = std::ranges::find_if(channelMap, [channelIndex](auto&& kv) { return kv.first == channelIndex; });
        if (it != channelMap.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    static constexpr CouplingType convertToCoupling(Coupling coupling) {
        if (coupling == Coupling::AC) {
            return PS3000A_AC;
        }
        if (coupling == Coupling::DC) {
            return PS3000A_DC;
        }
        throw std::runtime_error(fmt::format("Unsupported coupling mode: {}", static_cast<int>(coupling)));
    }

    static constexpr RangeType convertToRange(float range) {
        static constexpr std::array<std::pair<float, RangeType>, 12> rangeMap = {{                      //
            {0.01f, PS3000A_10MV}, {0.02f, PS3000A_20MV}, {0.05f, PS3000A_50MV}, {0.1f, PS3000A_100MV}, //
            {0.2f, PS3000A_200MV}, {0.5f, PS3000A_500MV}, {1.0f, PS3000A_1V}, {2.0f, PS3000A_2V},       //
            {5.0f, PS3000A_5V}, {10.0f, PS3000A_10V}, {20.0f, PS3000A_20V}, {50.0f, PS3000A_50V}}};

        const auto exactIt = std::ranges::find_if(rangeMap, [range](auto& kv) { return std::fabs(kv.first - range) < 1e-6f; });
        if (exactIt != rangeMap.end()) {
            return exactIt->second;
        }
        // if no exact match -> find the next higher range
        const auto upperIt = std::ranges::find_if(rangeMap, [range](const auto& kv) { return kv.first > range; });
        return upperIt != rangeMap.end() ? upperIt->second : PS3000A_50V;
    }

    constexpr ThresholdDirectionType convertToThresholdDirection(TriggerDirection direction) {
        using enum TriggerDirection;
        switch (direction) {
        case Rising: return PS3000A_RISING;
        case Falling: return PS3000A_FALLING;
        case Low: return PS3000A_BELOW;
        case High: return PS3000A_ABOVE;
        default: throw std::runtime_error(fmt::format("Unsupported trigger direction: {}", static_cast<int>(direction)));
        }
    };

    constexpr float uncertainty() {
        // https://www.picotech.com/oscilloscope/3000/picoscope-3000-oscilloscope-specifications
        return 0.0000160f;
    }

    PICO_STATUS
    setDataBuffer(int16_t handle, ChannelType channel, int16_t* buffer, int32_t bufferLth, uint32_t segmentIndex, RatioModeType mode) { return ps3000aSetDataBuffer(handle, channel, buffer, bufferLth, segmentIndex, mode); }

    PICO_STATUS
    getTimebase2(int16_t handle, uint32_t timebase, int32_t noSamples, float* timeIntervalNanoseconds, int32_t* maxSamples, uint32_t segmentIndex) { return ps3000aGetTimebase2(handle, timebase, noSamples, timeIntervalNanoseconds, /* oversample */ 0, maxSamples, segmentIndex); }

    PICO_STATUS
    openUnit(const std::string& serial_number) {
        // take any if serial number is not provided (useful for testing purposes)
        if (serial_number.empty()) {
            return ps3000aOpenUnit(&(this->_handle), nullptr);
        } else {
            return ps3000aOpenUnit(&(this->_handle), const_cast<int8_t*>(reinterpret_cast<const int8_t*>(serial_number.data())));
        }
    }

    PICO_STATUS
    closeUnit(int16_t handle) { return ps3000aCloseUnit(handle); }

    PICO_STATUS
    changePowerSource(int16_t handle, PICO_STATUS powerstate) { return ps3000aChangePowerSource(handle, powerstate); }

    PICO_STATUS
    maximumValue(int16_t handle, int16_t* value) { return ps3000aMaximumValue(handle, value); }

    PICO_STATUS
    memorySegments(int16_t handle, uint32_t nSegments, int32_t* nMaxSamples) { return ps3000aMemorySegments(handle, nSegments, nMaxSamples); }

    PICO_STATUS
    setNoOfCaptures(int16_t handle, uint32_t nCaptures) { return ps3000aSetNoOfCaptures(handle, nCaptures); }

    PICO_STATUS
    getNoOfCaptures(int16_t handle, uint32_t* nCaptures) const { return ps3000aGetNoOfCaptures(handle, nCaptures); }

    PICO_STATUS
    getNoOfProcessedCaptures(int16_t handle, uint32_t* nProcessedCaptures) const { return ps3000aGetNoOfProcessedCaptures(handle, nProcessedCaptures); }

    PICO_STATUS
    setChannel(int16_t handle, ChannelType channel, int16_t enabled, CouplingType type, ChannelRangeType range, float analogOffset) { return ps3000aSetChannel(handle, channel, enabled, type, range, analogOffset); }

    int maxChannel() { return PS3000A_MAX_CHANNELS; }

    int extTriggerMaxValue() { return PS3000A_EXT_MAX_VALUE; }

    int extTriggerMinValue() { return PS3000A_EXT_MIN_VALUE; }

    float extTriggerMaxValueVoltage() { return 5.f; }

    float extTriggerMinValueVoltage() { return -5.f; }

    CouplingType analogCoupling() { return PS3000A_AC; }

    TriggerStateType conditionDontCare() { return PS3000A_CONDITION_DONT_CARE; }

    ConditionsInfoType conditionsInfoClear() { return PS3000A_CLEAR; }

    ConditionsInfoType conditionsInfoAdd() { return PS3000A_ADD; }

    PICO_STATUS
    setSimpleTrigger(int16_t handle, int16_t enable, ChannelType source, int16_t threshold, ThresholdDirectionType direction, uint32_t delay, int16_t autoTriggerMs) { return ps3000aSetSimpleTrigger(handle, enable, source, threshold, direction, delay, autoTriggerMs); }

    PS3000A_TRIGGER_CONDITIONS
    convertToTriggerConditions(ConditionType* conditions, int16_t nConditions) {
        PS3000A_TRIGGER_CONDITIONS result{};
        for (int16_t i = 0; i < nConditions; ++i) {
            const ConditionType& cond = conditions[i];
            if (cond.source == PS3000A_CHANNEL_A) {
                result.channelA = cond.condition;
            } else if (cond.source == PS3000A_CHANNEL_B) {
                result.channelB = cond.condition;
            } else if (cond.source == PS3000A_CHANNEL_C) {
                result.channelC = cond.condition;
            } else if (cond.source == PS3000A_CHANNEL_D) {
                result.channelD = cond.condition;
            }
        }
        return result;
    }

    PICO_STATUS
    setTriggerChannelConditions(int16_t handle, ConditionType* conditions, int16_t nConditions, ConditionsInfoType) {
        PS3000A_TRIGGER_CONDITIONS conds = convertToTriggerConditions(conditions, nConditions);
        return ps3000aSetTriggerChannelConditions(handle, &conds, 1);
    }

    PICO_STATUS
    driverStop(int16_t handle) { return ps3000aStop(handle); }

    PICO_STATUS
    runBlock(int16_t handle, int32_t noOfPreTriggerSamples, int32_t noOfPostTriggerSamples, uint32_t timebase, int32_t* timeIndisposed, uint32_t segmentIndex, BlockReadyType ready, void* param) { return ps3000aRunBlock(handle, noOfPreTriggerSamples, noOfPostTriggerSamples, timebase, /* oversample, not used*/ 0, timeIndisposed, segmentIndex, ready, param); }

    PICO_STATUS
    runStreaming(int16_t handle, uint32_t* sampleInterval, TimeUnitsType timeUnits, uint32_t maxPreTriggerSamples, uint32_t maxPostTriggerSamples, int16_t autoStop, uint32_t downSampleRatio, RatioModeType downSampleRatioMode, uint32_t overviewBufferSize) { return ps3000aRunStreaming(handle, sampleInterval, timeUnits, maxPreTriggerSamples, maxPostTriggerSamples, autoStop, downSampleRatio, downSampleRatioMode, overviewBufferSize); }

    RatioModeType ratioNone() { return PS3000A_RATIO_MODE_NONE; }

    PICO_STATUS
    getStreamingLatestValues(int16_t handle, StreamingReadyType ready, void* param) { return ps3000aGetStreamingLatestValues(handle, ready, param); }

    PICO_STATUS
    getValues(int16_t handle, uint32_t startIndex, uint32_t* noOfSamples, uint32_t downSampleRatio, RatioModeType downSampleRatioMode, uint32_t segmentIndex, int16_t* overflow) { return ps3000aGetValues(handle, startIndex, noOfSamples, downSampleRatio, downSampleRatioMode, segmentIndex, overflow); }

    PICO_STATUS
    getUnitInfo(int16_t handle, int8_t* string, int16_t stringLength, int16_t* requiredSize, PICO_INFO info) const { return ps3000aGetUnitInfo(handle, string, stringLength, requiredSize, info); }

    PICO_STATUS
    getDeviceResolution(int16_t, DeviceResolutionType*) const { return PICO_NOT_SUPPORTED_BY_THIS_DEVICE; }

    // Digital picoscope inputs

    [[nodiscard]] Error setDigitalPorts() {
        for (std::size_t i = 0; i < 2; i++) {
            const PS3000A_DIGITAL_PORT channelId = i == 0 ? PS3000A_DIGITAL_PORT0 : PS3000A_DIGITAL_PORT1;
            const PICO_STATUS          status    = ps3000aSetDigitalPort(this->_handle, channelId, true, this->digital_port_threshold);

            if (status != PICO_OK) {
                fmt::println(std::cerr, "setDigitalPorts (chan {}): {}", magic_enum::enum_name(channelId), detail::getErrorMessage(status));
                return {status};
            }
        }
        return {};
    }

    [[nodiscard]] Error setDigitalBuffers(size_t nSamples, uint32_t segmentIndex) {
        for (std::size_t i = 0; i < 2; i++) {
            const auto channelId = i == 0 ? PS3000A_DIGITAL_PORT0 : PS3000A_DIGITAL_PORT1;
            _digitalBuffers[i].resize(std::max(nSamples, _digitalBuffers[i].size()));
            const PICO_STATUS status = ps3000aSetDataBuffer(this->_handle, static_cast<PS3000A_CHANNEL>(static_cast<int>(channelId)), _digitalBuffers[i].data(), static_cast<int32_t>(nSamples), segmentIndex, ratioNone());

            if (status != PICO_OK) {
                fmt::println(std::cerr, "setDigitalBuffers (chan {}): {}", magic_enum::enum_name(channelId), detail::getErrorMessage(status));
                return {status};
            }
        }
        return {};
    }

    void copyDigitalBuffersToOutput(std::span<std::uint16_t> output, std::size_t nSamples) {
        assert(output.size() >= nSamples);
        for (std::size_t i = 0; i < nSamples; i++) {
            uint16_t value = 0x00FF & static_cast<uint16_t>(_digitalBuffers[1][i]);
            value <<= 8;
            value |= 0x00FF & static_cast<uint16_t>(_digitalBuffers[0][i]);
            if (this->digital_port_invert_output) {
                value = ~value;
            }
            output[i] = value;
        }
    }
};

} // namespace fair::picoscope

#endif
