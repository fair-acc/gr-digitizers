#ifndef GR_DIGITIZERS_PICOSCOPEAPI_HPP
#define GR_DIGITIZERS_PICOSCOPEAPI_HPP

#include "StatusMessages.hpp"

#include <PicoConnectProbes.h>
#include <TimingMatcher.hpp>

namespace fair::picoscope {
using namespace std::literals;

struct Error {
    PICO_STATUS code = PICO_OK;

    [[nodiscard]] std::string message() const { return detail::getErrorMessage(code); }

    explicit constexpr operator bool() const noexcept { return code != PICO_OK; }
};

enum class AcquisitionMode { Streaming, RapidBlock };

enum class Coupling {
    DC,     // DC, 1 MOhm
    AC,     // AC, 1 MOhm
    DC_50R, // DC, 50 Ohm
};

enum class TriggerDirection { Rising, Falling, Low, High };

enum class TimeUnits { fs, ps, ns, us, ms, s };

struct GetValuesResult {
    Error       error;
    std::size_t nSamples;
    int16_t     overflow;
};

struct TimebaseResult {
    uint32_t timebase;
    float    actualFreq;
};

struct TimeInterval {
    TimeUnits unit;
    uint32_t  interval;
};

namespace detail {

[[nodiscard]] inline constexpr TimeInterval convertSampleRateToTimeInterval(float sampleRate) {
    const double intervalSec = 1.0 / static_cast<double>(sampleRate);
    double       factor      = 0.;
    TimeUnits    unit;

    if (intervalSec < 0.000001) {
        unit   = TimeUnits::ps;
        factor = 1e12;
    } else if (intervalSec < 0.001) {
        unit   = TimeUnits::ns;
        factor = 1e9;
    } else if (intervalSec < 0.1) {
        unit   = TimeUnits::us;
        factor = 1e6;
    } else {
        unit   = TimeUnits::ms;
        factor = 1e3;
    }
    return {unit, static_cast<std::uint32_t>(factor / static_cast<double>(sampleRate))};
}

[[nodiscard]] inline constexpr float convertTimeIntervalToSampleRate(TimeInterval timeInterval) {
    double factor;
    switch (timeInterval.unit) {
        case TimeUnits::fs: factor = 1e15; break;
        case TimeUnits::ps: factor = 1e12; break;
        case TimeUnits::ns: factor = 1e9; break;
        case TimeUnits::us: factor = 1e6; break;
        case TimeUnits::ms: factor = 1e3; break;
        default: factor = 1.; break;
    }
    return timeInterval.interval == 0 ? 1.f : static_cast<float>(factor / static_cast<double>(timeInterval.interval));
}
} // end namespace detail

template<typename T>
concept PicoscopeImplementationLike = requires (T picoScopeImpl, PICO_STATUS status, int16_t i16, uint32_t ui32, int32_t i32) {
    { T::N_ANALOG_CHANNELS } -> std::convertible_to<const std::size_t>;
    { T::N_DIGITAL_CHANNELS } -> std::convertible_to<const std::size_t>;
    { picoScopeImpl.changePowerSource(status) } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.closeUnit() } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.convertSampleRateToTimebase(.0f) } -> std::same_as<TimebaseResult>;
    { picoScopeImpl.convertTimeUnits(std::declval<TimeUnits>()) } -> std::same_as<typename T::TimeUnitsType>;
    { picoScopeImpl.convertToChannel(""s) } -> std::same_as<std::optional<typename T::ChannelType>>;
    { picoScopeImpl.convertToCoupling(std::declval<Coupling>()) } -> std::same_as<typename T::CouplingType>;
    { picoScopeImpl.convertToOutputIndex(""sv) } -> std::same_as<std::optional<std::size_t>>;
    { picoScopeImpl.convertToRange(.0f) } -> std::same_as<typename T::RangeType>;
    { picoScopeImpl.convertToThresholdDirection(std::declval<TriggerDirection>()) } -> std::same_as<typename T::ThresholdDirectionType>;
    { picoScopeImpl.driverStop() } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.extTriggerMaxValue() } -> std::same_as<int>;
    { picoScopeImpl.extTriggerMinValue() } -> std::same_as<int>;
    { picoScopeImpl.extTriggerMaxValueVoltage() } -> std::same_as<float>;
    { picoScopeImpl.extTriggerMinValueVoltage() } -> std::same_as<float>;
    { picoScopeImpl.getNoOfCaptures(&ui32) } -> std::same_as<PICO_STATUS >;
    { picoScopeImpl.getUnitInfo(std::declval<std::int8_t*>(), i16, &i16, std::declval<PICO_INFO>()) } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.maxChannel() } -> std::same_as<int>;
    { picoScopeImpl.maximumValue(&i16) } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.memorySegments(ui32, &i32) } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.openUnit(""s) } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.ratioNone() } -> std::same_as<typename T::RatioModeType>;
    { picoScopeImpl.runStreaming(&ui32, std::declval<typename T::TimeUnitsType>(), ui32, ui32, i16, ui32, std::declval<typename T::RatioModeType>(), ui32 ) } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.setChannel(std::declval<typename T::ChannelType>(), i16, std::declval<typename T::CouplingType>(), std::declval<typename T::RangeType>(), .0f) } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.setDataBuffer(std::declval<typename T::ChannelType>(), &i16, i32, std::declval<typename T::RatioModeType>()) } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.setDataBufferForSegment(std::declval<typename T::ChannelType>(), &i16, i32, ui32, std::declval<typename T::RatioModeType>()) } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.setNoOfCaptures(ui32) } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.setSimpleTrigger(i16, std::declval<typename T::ChannelType>(), i16, std::declval<typename T::ThresholdDirectionType>(), ui32, i16) } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.uncertainty() } -> std::same_as<float>;
    requires requires { T::N_DIGITAL_CHANNELS == 0  || requires {
            { picoScopeImpl.setDigitalBuffers(0UZ, ui32) } -> std::same_as<PICO_STATUS>;
            { picoScopeImpl.setDigitalPorts() } -> std::same_as<PICO_STATUS>;
            { picoScopeImpl.setTriggerDigitalPort(0, std::declval<TriggerDirection>()) } -> std::same_as<PICO_STATUS>;
            { picoScopeImpl.copyDigitalBuffersToOutput(std::declval<std::span<std::uint16_t>>(), std::size_t()) } -> std::same_as<void>;
        }; };
};

}
#endif //GR_DIGITIZERS_PICOSCOPEAPI_HPP
