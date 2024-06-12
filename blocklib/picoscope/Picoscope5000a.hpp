#ifndef FAIR_PICOSCOPE_PICOSCOPE5000A_HPP
#define FAIR_PICOSCOPE_PICOSCOPE5000A_HPP

#include <Picoscope.hpp>

#include <ps5000aApi.h>

namespace fair::picoscope {

template<typename T, AcquisitionMode acquisitionMode>
struct Picoscope5000a : public fair::picoscope::Picoscope<T, acquisitionMode, Picoscope5000a<T, acquisitionMode>> {
    std::array<gr::PortOut<T>, 4> analog_out;

    using ChannelType            = PS5000A_CHANNEL;
    using ConditionType          = PS5000A_CONDITION;
    using CouplingType           = PS5000A_COUPLING;
    using RangeType              = PS5000A_RANGE;
    using ChannelRangeType       = RangeType;
    using ThresholdDirectionType = PS5000A_THRESHOLD_DIRECTION;
    using TriggerStateType       = PS5000A_TRIGGER_STATE;
    using ConditionsInfoType     = PS5000A_CONDITIONS_INFO;
    using TimeUnitsType          = PS5000A_TIME_UNITS;
    using StreamingReadyType     = ps5000aStreamingReady;
    using BlockReadyType         = ps5000aBlockReady;
    using RatioModeType          = PS5000A_RATIO_MODE;

    /*!
     * a structure used for streaming setup
     */
    struct UnitInterval {
        TimeUnitsType unit;
        uint32_t      interval;
    };

    constexpr UnitInterval
    convertFrequencyToTimeUnitsAndInterval(double desired_freq, double &actual_freq) {
        UnitInterval unint;

        if (const auto interval = 1.0 / desired_freq; interval < 0.000001) {
            unint.unit     = PS5000A_PS;
            unint.interval = static_cast<uint32_t>(1000000000000.0 / desired_freq);
            actual_freq    = 1000000000000.0 / unint.interval;
        } else if (interval < 0.001) {
            unint.unit     = PS5000A_NS;
            unint.interval = static_cast<uint32_t>(1000000000.0 / desired_freq);
            actual_freq    = 1000000000.0 / unint.interval;
        } else if (interval < 0.1) {
            unint.unit     = PS5000A_US;
            unint.interval = static_cast<uint32_t>(1000000.0 / desired_freq);
            actual_freq    = 1000000.0 / unint.interval;
        } else {
            unint.unit     = PS5000A_MS;
            unint.interval = static_cast<uint32_t>(1000.0 / desired_freq);
            actual_freq    = 1000.0 / unint.interval;
        }
        this->validateDesiredActualFrequency(desired_freq, actual_freq);
        return unint;
    }

    static constexpr std::optional<ChannelType>
    convertToChannel(std::string_view source) {
        if (source == "A") return PS5000A_CHANNEL_A;
        if (source == "B") return PS5000A_CHANNEL_B;
        if (source == "C") return PS5000A_CHANNEL_C;
        if (source == "D") return PS5000A_CHANNEL_D;
        if (source == "EXTERNAL") return PS5000A_EXTERNAL;
        return {};
    }

    static constexpr CouplingType
    convertToCoupling(Coupling coupling) {
        if (coupling == Coupling::AC_1M) return PS5000A_AC;
        if (coupling == Coupling::DC_1M) return PS5000A_DC;
        throw std::runtime_error(fmt::format("Unsupported coupling mode: {}", static_cast<int>(coupling)));
    }

    static constexpr RangeType
    convertToRange(double range) {
        if (range == 0.01) return PS5000A_10MV;
        if (range == 0.02) return PS5000A_20MV;
        if (range == 0.05) return PS5000A_50MV;
        if (range == 0.1) return PS5000A_100MV;
        if (range == 0.2) return PS5000A_200MV;
        if (range == 0.5) return PS5000A_500MV;
        if (range == 1.0) return PS5000A_1V;
        if (range == 2.0) return PS5000A_2V;
        if (range == 5.0) return PS5000A_5V;
        if (range == 10.0) return PS5000A_10V;
        if (range == 20.0) return PS5000A_20V;
        if (range == 50.0) return PS5000A_50V;
        throw std::runtime_error(fmt::format("Range value not supported: {}", range));
    }

    constexpr ThresholdDirectionType
    convertToThresholdDirection(TriggerDirection direction) {
        using enum TriggerDirection;
        switch (direction) {
        case Rising: return PS5000A_RISING;
        case Falling: return PS5000A_FALLING;
        case Low: return PS5000A_BELOW;
        case High: return PS5000A_ABOVE;
        default: throw std::runtime_error(fmt::format("Unsupported trigger direction: {}", static_cast<int>(direction)));
        }
    };

    constexpr float
    uncertainty() {
        // https://www.picotech.com/oscilloscope/5000/picoscope-5000-specifications
        return 0.0000120f;
    }

    PICO_STATUS
    setDataBuffer(int16_t handle, ChannelType channel, int16_t *buffer, int32_t bufferLth, uint32_t segmentIndex, RatioModeType mode) {
        return ps5000aSetDataBuffer(handle, channel, buffer, bufferLth, segmentIndex, mode);
    }

    PICO_STATUS
    getTimebase2(int16_t handle, uint32_t timebase, int32_t noSamples, float *timeIntervalNanoseconds, int32_t *maxSamples, uint32_t segmentIndex) {
        return ps5000aGetTimebase2(handle, timebase, noSamples, timeIntervalNanoseconds, maxSamples, segmentIndex);
    }

    PICO_STATUS
    openUnit(const std::string &serial_number) {
        // take any if serial number is not provided (useful for testing purposes)
        if (serial_number.empty()) {
            return ps5000aOpenUnit(&this->ps_state.handle, nullptr, PS5000A_DR_8BIT);
        } else {
            return ps5000aOpenUnit(&this->ps_state.handle, const_cast<int8_t *>(reinterpret_cast<const int8_t *>(serial_number.data())), PS5000A_DR_8BIT);
        }
    }

    PICO_STATUS
    closeUnit(int16_t handle) { return ps5000aCloseUnit(handle); }

    PICO_STATUS
    changePowerSource(int16_t handle, PICO_STATUS powerstate) { return ps5000aChangePowerSource(handle, powerstate); }

    PICO_STATUS
    maximumValue(int16_t handle, int16_t *value) { return ps5000aMaximumValue(handle, value); }

    PICO_STATUS
    memorySegments(int16_t handle, uint32_t nSegments, int32_t *nMaxSamples) { return ps5000aMemorySegments(handle, nSegments, nMaxSamples); }

    PICO_STATUS
    setNoOfCaptures(int16_t handle, uint32_t nCaptures) { return ps5000aSetNoOfCaptures(handle, nCaptures); }

    PICO_STATUS
    setChannel(int16_t handle, ChannelType channel, int16_t enabled, CouplingType type, ChannelRangeType range, float analogOffset) {
        return ps5000aSetChannel(handle, channel, enabled, type, range, analogOffset);
    }

    int
    maxChannel() {
        return PS5000A_MAX_CHANNELS;
    }

    int
    maxVoltage() {
        return PS5000A_EXT_MAX_VALUE;
    }

    CouplingType
    analogCoupling() {
        return PS5000A_AC;
    }

    TriggerStateType
    conditionDontCare() {
        return PS5000A_CONDITION_DONT_CARE;
    }

    ConditionsInfoType
    conditionsInfoClear() {
        return PS5000A_CLEAR;
    }

    PICO_STATUS
    setSimpleTrigger(int16_t handle, int16_t enable, ChannelType source, int16_t threshold, ThresholdDirectionType direction, uint32_t delay, int16_t autoTriggerMs) {
        return ps5000aSetSimpleTrigger(handle, enable, source, threshold, direction, delay, autoTriggerMs);
    }

    PS5000A_TRIGGER_CONDITIONS
    conditionsShim(ConditionType *condition, int16_t nConditions) {
        PS5000A_TRIGGER_CONDITIONS result;
        std::memset(&result, 0, sizeof(result));
        for (int16_t i = 0; i < nConditions; ++i) {
            auto cond = *(condition + i);
            if (cond.source == PS5000A_CHANNEL_A) {
                result.channelA = cond.condition;
            } else if (cond.source == PS5000A_CHANNEL_B) {
                result.channelB = cond.condition;
            } else if (cond.source == PS5000A_CHANNEL_C) {
                result.channelC = cond.condition;
            } else if (cond.source == PS5000A_CHANNEL_D) {
                result.channelD = cond.condition;
            }
        }
        return result;
    }

    PICO_STATUS
    setTriggerChannelConditions(int16_t handle, ConditionType *conditions, int16_t nConditions, ConditionsInfoType) {
        PS5000A_TRIGGER_CONDITIONS conds = conditionsShim(conditions, nConditions);
        return ps5000aSetTriggerChannelConditions(handle, &conds, 1);
    }

    PICO_STATUS
    driver_stop(int16_t handle) { return ps5000aStop(handle); }

    PICO_STATUS
    runBlock(int16_t handle, int32_t noOfPreTriggerSamples, int32_t noOfPostTriggerSamples, uint32_t timebase, int32_t *timeIndisposed, uint32_t segmentIndex, BlockReadyType ready, void *param) {
        return ps5000aRunBlock(handle, noOfPreTriggerSamples, noOfPostTriggerSamples, timebase, timeIndisposed, segmentIndex, ready, param);
    }

    PICO_STATUS
    runStreaming(int16_t handle, uint32_t *sampleInterval, TimeUnitsType timeUnits, uint32_t maxPreTriggerSamples, uint32_t maxPostTriggerSamples, int16_t autoStop, uint32_t downSampleRatio,
                 RatioModeType downSampleRatioMode, uint32_t overviewBufferSize) {
        return ps5000aRunStreaming(handle, sampleInterval, timeUnits, maxPreTriggerSamples, maxPostTriggerSamples, autoStop, downSampleRatio, downSampleRatioMode, overviewBufferSize);
    }

    RatioModeType
    ratioNone() {
        return PS5000A_RATIO_MODE_NONE;
    }

    PICO_STATUS
    getStreamingLatestValues(int16_t handle, StreamingReadyType ready, void *param) { return ps5000aGetStreamingLatestValues(handle, ready, param); }

    PICO_STATUS
    getValues(int16_t handle, uint32_t startIndex, uint32_t *noOfSamples, uint32_t downSampleRatio, RatioModeType downSampleRatioMode, uint32_t segmentIndex, int16_t *overflow) {
        return ps5000aGetValues(handle, startIndex, noOfSamples, downSampleRatio, downSampleRatioMode, segmentIndex, overflow);
    }

    PICO_STATUS
    getUnitInfo(int16_t handle, int8_t *string, int16_t stringLength, int16_t *requiredSize, PICO_INFO info) const { return ps5000aGetUnitInfo(handle, string, stringLength, requiredSize, info); }
};

} // namespace fair::picoscope

ENABLE_REFLECTION_FOR_TEMPLATE_FULL((typename T, fair::picoscope::AcquisitionMode acquisitionMode), (fair::picoscope::Picoscope5000a<T, acquisitionMode>), analog_out, serial_number, sample_rate,
                                    pre_samples, post_samples, acquisition_mode, rapid_block_nr_captures, streaming_mode_poll_rate, auto_arm, trigger_once, channel_ids, channel_names, channel_units,
                                    channel_ranges, channel_offsets, channel_couplings, trigger_source, trigger_threshold, trigger_direction, trigger_pin);

#endif
