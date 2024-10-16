#ifndef FAIR_PICOSCOPE_PICOSCOPE4000A_HPP
#define FAIR_PICOSCOPE_PICOSCOPE4000A_HPP

#include <Picoscope.hpp>

#include <ps4000aApi.h>

namespace fair::picoscope {

template<typename T, AcquisitionMode acquisitionMode>
class Picoscope4000a : public fair::picoscope::Picoscope<T, acquisitionMode, Picoscope4000a<T, acquisitionMode>> {
public:
    using super_t = fair::picoscope::Picoscope<T, acquisitionMode, Picoscope4000a<T, acquisitionMode>>;

    Picoscope4000a(gr::property_map props) : super_t(std::move(props)) {}

    std::vector<gr::PortOut<T>> analog_out{8};

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

    GR_MAKE_REFLECTABLE(Picoscope4000a, analog_out);

    /*!
     * a structure used for streaming setup
     */
    struct UnitInterval {
        TimeUnitsType unit;
        uint32_t      interval;
    };

    constexpr UnitInterval convertFrequencyToTimeUnitsAndInterval(double desired_freq, double& actual_freq) {
        UnitInterval unint;

        if (const auto interval = 1.0 / desired_freq; interval < 0.000001) {
            unint.unit     = PS4000A_PS;
            unint.interval = static_cast<uint32_t>(1000000000000.0 / desired_freq);
            actual_freq    = 1000000000000.0 / unint.interval;
        } else if (interval < 0.001) {
            unint.unit     = PS4000A_NS;
            unint.interval = static_cast<uint32_t>(1000000000.0 / desired_freq);
            actual_freq    = 1000000000.0 / unint.interval;
        } else if (interval < 0.1) {
            unint.unit     = PS4000A_US;
            unint.interval = static_cast<uint32_t>(1000000.0 / desired_freq);
            actual_freq    = 1000000.0 / unint.interval;
        } else {
            unint.unit     = PS4000A_MS;
            unint.interval = static_cast<uint32_t>(1000.0 / desired_freq);
            actual_freq    = 1000.0 / unint.interval;
        }
        this->validateDesiredActualFrequency(desired_freq, actual_freq);
        return unint;
    }

    static constexpr std::optional<ChannelType> convertToChannel(std::string_view source) {
        if (source == "A") {
            return PS4000A_CHANNEL_A;
        }
        if (source == "B") {
            return PS4000A_CHANNEL_B;
        }
        if (source == "C") {
            return PS4000A_CHANNEL_C;
        }
        if (source == "D") {
            return PS4000A_CHANNEL_D;
        }
        if (source == "E") {
            return PS4000A_CHANNEL_E;
        }
        if (source == "F") {
            return PS4000A_CHANNEL_F;
        }
        if (source == "G") {
            return PS4000A_CHANNEL_G;
        }
        if (source == "H") {
            return PS4000A_CHANNEL_H;
        }
        if (source == "EXTERNAL") {
            return PS4000A_EXTERNAL;
        }
        return {};
    }

    static constexpr CouplingType convertToCoupling(Coupling coupling) {
        if (coupling == Coupling::AC_1M) {
            return PS4000A_AC;
        }
        if (coupling == Coupling::DC_1M) {
            return PS4000A_DC;
        }
        throw std::runtime_error(fmt::format("Unsupported coupling mode: {}", static_cast<int>(coupling)));
    }

    static constexpr RangeType convertToRange(double range) {
        if (range == 0.01) {
            return PS4000A_10MV;
        }
        if (range == 0.02) {
            return PS4000A_20MV;
        }
        if (range == 0.05) {
            return PS4000A_50MV;
        }
        if (range == 0.1) {
            return PS4000A_100MV;
        }
        if (range == 0.2) {
            return PS4000A_200MV;
        }
        if (range == 0.5) {
            return PS4000A_500MV;
        }
        if (range == 1.0) {
            return PS4000A_1V;
        }
        if (range == 2.0) {
            return PS4000A_2V;
        }
        if (range == 5.0) {
            return PS4000A_5V;
        }
        if (range == 10.0) {
            return PS4000A_10V;
        }
        if (range == 20.0) {
            return PS4000A_20V;
        }
        if (range == 50.0) {
            return PS4000A_50V;
        }
        if (range == 100.0) {
            return PS4000A_100V;
        }
        if (range == 200.0) {
            return PS4000A_200V;
        }
        throw std::runtime_error(fmt::format("Range value not supported: {}", range));
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
            return ps4000aOpenUnit(&this->ps_state.handle, nullptr);
        } else {
            return ps4000aOpenUnit(&this->ps_state.handle, const_cast<int8_t*>(reinterpret_cast<const int8_t*>(serial_number.data())));
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
    setChannel(int16_t handle, ChannelType channel, int16_t enabled, CouplingType type, ChannelRangeType range, float analogOffset) { return ps4000aSetChannel(handle, channel, enabled, type, range, analogOffset); }

    int maxChannel() { return PS4000A_MAX_CHANNELS; }

    int maxVoltage() { return PS4000A_EXT_MAX_VALUE; }

    CouplingType analogCoupling() { return PS4000A_AC; }

    TriggerStateType conditionDontCare() { return PS4000A_CONDITION_DONT_CARE; }

    ConditionsInfoType conditionsInfoClear() { return PS4000A_CLEAR; }

    PICO_STATUS
    setSimpleTrigger(int16_t handle, int16_t enable, ChannelType source, int16_t threshold, ThresholdDirectionType direction, uint32_t delay, int16_t autoTriggerMs) { return ps4000aSetSimpleTrigger(handle, enable, source, threshold, direction, delay, autoTriggerMs); }

    PICO_STATUS
    setTriggerChannelConditions(int16_t handle, ConditionType* conditions, int16_t nConditions, ConditionsInfoType info) { return ps4000aSetTriggerChannelConditions(handle, conditions, nConditions, info); }

    PICO_STATUS
    driver_stop(int16_t handle) { return ps4000aStop(handle); }

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
};

} // namespace fair::picoscope

#endif
