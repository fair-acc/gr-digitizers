#ifndef GR_DIGITIZERS_PICOSCOPEAPI_HPP
#define GR_DIGITIZERS_PICOSCOPEAPI_HPP

#include <source_location>
#include <thread>
#include <utility>

#ifdef __GNUC__
#pragma GCC diagnostic push // ignore warning of external libraries that from this lib-context we do not have any control over
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#endif
#include <magic_enum.hpp>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "fair/picoscope/StatusMessages.hpp"

#include <PicoConnectProbes.h>
#include <fair/picoscope/TimingMatcher.hpp>

namespace fair::picoscope {

struct Error {
    std::variant<PICO_STATUS, std::string> error;
    std::source_location                   location;

    explicit Error(PICO_STATUS errorCode, const std::source_location _location = std::source_location::current()) : error{errorCode}, location{_location} {}
    explicit Error(const std::string& errorMsg, const std::source_location _location = std::source_location::current()) : error{errorMsg}, location{_location} {}

    [[nodiscard]] std::string_view getError() const { return std::holds_alternative<PICO_STATUS>(error) ? detail::statusToString(std::get<PICO_STATUS>(error)) : "PICOSCOPE_WRAPPER_ERROR"; }
    [[nodiscard]] std::string_view getDescription() const { return std::holds_alternative<PICO_STATUS>(error) ? detail::statusToStringVerbose(std::get<PICO_STATUS>(error)) : std::get<std::string>(error); }
};

enum class AcquisitionMode { Streaming, RapidBlock };

enum class AnalogChannelRange { ps10mV, ps20mV, ps50mV, ps100mV, ps200mV, ps500mV, ps1V, ps2V, ps5V, ps10V, ps20V, ps50V, ps100V, ps200V, ps500V };
constexpr std::array<std::pair<float, AnalogChannelRange>, magic_enum::enum_count<AnalogChannelRange>()> analogChannelRanges{{
    {000.01f, AnalogChannelRange::ps10mV},
    {000.02f, AnalogChannelRange::ps20mV},
    {000.05f, AnalogChannelRange::ps50mV},
    {000.10f, AnalogChannelRange::ps100mV},
    {000.20f, AnalogChannelRange::ps200mV},
    {000.50f, AnalogChannelRange::ps500mV},
    {001.00f, AnalogChannelRange::ps1V},
    {002.00f, AnalogChannelRange::ps2V},
    {005.00f, AnalogChannelRange::ps5V},
    {010.00f, AnalogChannelRange::ps10V},
    {020.00f, AnalogChannelRange::ps20V},
    {050.00f, AnalogChannelRange::ps50V},
    {100.00f, AnalogChannelRange::ps100V},
    {200.00f, AnalogChannelRange::ps200V},
    {500.00f, AnalogChannelRange::ps500V},
}};
inline std::expected<AnalogChannelRange, Error>                                                          toAnalogChannelRange(const float value) {
    if (const auto it = std::ranges::find(analogChannelRanges, value, &std::pair<float, AnalogChannelRange>::first); it != analogChannelRanges.end()) {
        return it->second;
    }
    return std::unexpected(Error{std::format("unsupported analog channel range: {}", value)});
}
inline float toAnalogChannelRangeValue(const AnalogChannelRange range) {
    if (const auto it = std::ranges::find(analogChannelRanges, range, &std::pair<float, AnalogChannelRange>::second); it != analogChannelRanges.end()) {
        return it->first;
    }
    return 0.0f; // should never be reached. analogChannelRanges contains all defined enum values
}

enum class Coupling {
    DC,     // DC, 1 MOhm
    AC,     // AC, 1 MOhm
    DC_50R, // DC, 50 Ohm (only supported on PS6000)
};

enum class TriggerDirection { Rising, Falling, Low, High };

enum class TimeUnits { fs, ps, ns, us, ms, s };

enum class ChannelName { A, B, C, D, E, F, G, H, EXTERNAL, AUX };
constexpr std::optional<ChannelName> toChannel(const std::string_view channel_name) { return magic_enum::enum_cast<ChannelName>(channel_name); }
constexpr std::string_view           toChannelString(const ChannelName channel) { return magic_enum::enum_name(channel); }

struct TimebaseResult {
    uint32_t timebase;
    float    actualFreq;
};

struct TimeInterval {
    TimeUnits unit;
    uint32_t  interval;
};

namespace detail {

[[nodiscard]] constexpr TimeInterval convertSampleRateToTimeInterval(const float sampleRate) {
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

[[nodiscard]] constexpr float convertTimeIntervalToSampleRate(const TimeInterval timeInterval) {
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

/**
 * Concept with the required methods that need to be implemented by picoscope devices. Closely wraps the C API provided by picoscope.
 **/
template<typename T>
concept PicoscopeImplementationLike = requires(T picoScopeImpl, PICO_STATUS status, int16_t i16, uint32_t ui32, int32_t i32) {
    { T::enumerateUnits(&i16, std::declval<std::int8_t*>(), &i16) } -> std::same_as<PICO_STATUS>;
    { T::N_DIGITAL_CHANNELS } -> std::convertible_to<const std::size_t>;
    { T::ratioNone } -> std::convertible_to<const typename T::RatioModeType&>;
    { picoScopeImpl.openUnit(std::string{}) } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.changePowerSource(status) } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.closeUnit() } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.convertSampleRateToTimebase(.0f) } -> std::same_as<std::expected<TimebaseResult, Error>>;
    { picoScopeImpl.convertTimeUnits(std::declval<TimeUnits>()) } -> std::same_as<typename T::TimeUnitsType>;
    { picoScopeImpl.convertToThresholdDirection(std::declval<TriggerDirection>()) } -> std::same_as<std::expected<typename T::ThresholdDirectionType, Error>>;
    // TODO: These are currently not used and not provided (natively) by ps6000
    //{ picoScopeImpl.extTriggerMaxValue() } -> std::same_as<int>;
    //{ picoScopeImpl.extTriggerMinValue() } -> std::same_as<int>;
    //{ picoScopeImpl.extTriggerMaxValueVoltage() } -> std::same_as<float>;
    //{ picoScopeImpl.extTriggerMinValueVoltage() } -> std::same_as<float>;
    { picoScopeImpl.getNoOfCaptures(&ui32) } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.getUnitInfo(std::declval<std::int8_t*>(), i16, &i16, std::declval<PICO_INFO>()) } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.maxChannel() } -> std::same_as<int>;
    { picoScopeImpl.maximumValue(&i16) } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.memorySegments(ui32, &i32) } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.runStreaming(&ui32, std::declval<typename T::TimeUnitsType>(), ui32, ui32, i16, ui32, std::declval<typename T::RatioModeType>(), ui32) } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.driverStop() } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.setChannel(std::declval<typename T::ChannelType>(), i16, std::declval<typename T::CouplingType>(), std::declval<typename T::RangeType>(), .0f) } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.setDataBuffer(std::declval<typename T::ChannelType>(), &i16, i32, std::declval<typename T::RatioModeType>()) } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.setDataBuffer(std::declval<typename T::ChannelType>(), &i16, i32, ui32, std::declval<typename T::RatioModeType>()) } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.setNoOfCaptures(ui32) } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.setSimpleTrigger(i16, std::declval<typename T::ChannelType>(), i16, std::declval<typename T::ThresholdDirectionType>(), ui32, i16) } -> std::same_as<PICO_STATUS>;
    { picoScopeImpl.uncertainty() } -> std::same_as<float>;
    requires requires {
        T::N_DIGITAL_CHANNELS == 0 || requires {
            { picoScopeImpl.setDigitalPorts() } -> std::same_as<PICO_STATUS>;
            { picoScopeImpl.setTriggerDigitalPort(0, std::declval<TriggerDirection>()) } -> std::same_as<PICO_STATUS>;
        };
    };
};

struct DeviceInformation {
    std::string model;
    std::string serial;
    std::string hardwareVersion;
};

struct ChannelConfig {
    bool               enable   = false;
    AnalogChannelRange range    = AnalogChannelRange::ps5V;
    float              offset   = 0.0f;
    Coupling           coupling = Coupling::DC;

    bool operator==(const ChannelConfig&) const = default;
};

struct TriggerConfig {
    using TriggerSource = std::variant<std::monostate, ChannelName, uint>;
    TriggerSource    source{};
    TriggerDirection direction       = TriggerDirection::Rising;
    int16_t          threshold       = 0U;
    uint32_t         delay           = 0U;
    int16_t          auto_trigger_ms = 0u;
    bool             modified        = true;

    bool operator==(const TriggerConfig& other) const = default;
};

/**
 * A simple C++ wrapper that abstracts away the picoscope-specific API and provides a simple C++ native API to be used in gnuradio4 blocks.
 * Also includes proper error handling and retry machinery to recover from temporary failures and prevent blocking calls.
 */
template<PicoscopeImplementationLike TPSImpl>
class PicoscopeWrapper {
    using HandlerT = std::optional<std::function<void(std::span<std::span<const std::int16_t>>, std::int16_t)>>;
    struct OpeningContext {
        PicoscopeWrapper& scope;
        std::int16_t      status   = 0;
        std::int16_t      progress = 0;
        std::int16_t      complete = 0;

        explicit OpeningContext(PicoscopeWrapper<TPSImpl>& _scope) : scope{_scope} { std::ignore = poll(); }

        OpeningContext(OpeningContext&)            = delete;
        OpeningContext& operator=(OpeningContext&) = delete;

        [[nodiscard]] bool ready() const { return complete == 1; }

        std::expected<bool, Error> poll() {
            if (status != 1) { // Trigger Async Open
                if (const PICO_STATUS ret = scope.instance.openUnitAsync(&status, scope.serial); ret != PICO_OK && ret != PICO_OPEN_OPERATION_IN_PROGRESS) {
                    return std::unexpected{Error{ret}};
                }
            }
            if (status == 1 && complete != 1) { // Check AsyncOpen progress
                if (const PICO_STATUS ret = scope.instance.openUnitProgress(&progress, &complete); ret != PICO_OK && ret != PICO_OPEN_OPERATION_IN_PROGRESS) {
                    if (ret == PICO_POWER_SUPPLY_NOT_CONNECTED) {
                        if (const PICO_STATUS ret2 = scope.instance.changePowerSource(ret); ret2 != PICO_OK) {
                            return std::unexpected(Error{ret2});
                        }
                        return false;
                    }
                    return std::unexpected(Error{ret});
                }
                if (complete == 1) { // update picoscope metainformation
                    std::array<int8_t, 50> string{};
                    int16_t                len;
                    if (const PICO_STATUS res = scope.instance.getUnitInfo(string.data(), static_cast<int16_t>(string.size()), &len, PICO_VARIANT_INFO); res == PICO_OK) {
                        scope.info.model = {reinterpret_cast<char*>(string.data()), static_cast<std::size_t>(len - 1)};
                    } else {
                        return std::unexpected(Error(res));
                    }
                    if (const PICO_STATUS res = scope.instance.getUnitInfo(string.data(), static_cast<int16_t>(string.size()), &len, PICO_HARDWARE_VERSION); res == PICO_OK) {
                        scope.info.hardwareVersion = {reinterpret_cast<char*>(string.data()), static_cast<std::size_t>(len - 1)};
                    } else {
                        return std::unexpected(Error(res));
                    }
                    if (const PICO_STATUS res = scope.instance.getUnitInfo(string.data(), static_cast<int16_t>(string.size()), &len, PICO_BATCH_AND_SERIAL); res == PICO_OK) {
                        scope.info.serial = {reinterpret_cast<char*>(string.data()), static_cast<std::size_t>(len - 1)};
                    } else {
                        return std::unexpected(Error(res));
                    }
                }
            }
            return status == 1 && complete == 1;
        }
    };

    struct StreamingAcquisitionContext {
        PicoscopeWrapper&         scope;
        bool                      started = false;
        float                     freq;
        float                     actualFreq    = 0.0f;
        bool                      enableDigital = false;
        std::size_t               samples       = 0UZ;
        std::vector<std::int16_t> dataDigital{};

        explicit StreamingAcquisitionContext(PicoscopeWrapper<TPSImpl>& _scope, const float _freq, const bool _enableDigital = false) : scope{_scope}, freq{_freq}, enableDigital{_enableDigital} {}

        StreamingAcquisitionContext(StreamingAcquisitionContext&)            = delete;
        StreamingAcquisitionContext& operator=(StreamingAcquisitionContext&) = delete;

        std::expected<void, Error> poll(const HandlerT& dataHandler) {
            if (scope.restartAcquisition) {
                if (auto res = restart(); !res.has_value()) {
                    return res;
                }
                scope.restartAcquisition = false;
            }
            if (!started) {
                if (auto res = start(); !res.has_value()) {
                    return res;
                }
            }
            struct Ctx {
                StreamingAcquisitionContext& ctx;
                const HandlerT&              handler;
            } valueContext{*this, dataHandler};
            auto streamingReadyCallback = static_cast<typename TPSImpl::StreamingReadyType>([](int16_t /*handle*/, typename TPSImpl::NSamplesType noOfSamples, uint32_t startIndex, int16_t overflow, uint32_t /*triggerAt*/, int16_t /*triggered*/, int16_t /*autoStop*/, void* vobj) {
                auto                                           dataContext = static_cast<Ctx*>(vobj);
                constexpr std::size_t                          channels    = TPSImpl::N_ANALOG_CHANNELS + (TPSImpl::N_DIGITAL_CHANNELS > 0UZ ? 1UZ : 0UZ);
                std::array<std::span<const int16_t>, channels> acquisitionData;
                auto                                           activeChannels        = static_cast<std::size_t>(std::ranges::count_if(dataContext->ctx.scope.channel_config, [](const auto& ch) { return ch.second.enable; }));
                auto                                           activeChannelsDigital = dataContext->ctx.enableDigital ? 2UZ : 0UZ; // The digital ports use 2 buffers for the lower and higher 8 bit
                const std::span<const int16_t>                 dataBuffer            = dataContext->ctx.scope.data;
                const std::size_t                              segmentSize           = dataBuffer.size() / (activeChannels + activeChannelsDigital);
                for (std::size_t i = 0; i < activeChannels; i++) {
                    acquisitionData[i] = dataBuffer.subspan(i * segmentSize + startIndex, static_cast<std::size_t>(noOfSamples));
                }
                if constexpr (TPSImpl::N_DIGITAL_CHANNELS > 0) {
                    if (dataContext->ctx.enableDigital) {
                        const auto lowerBits  = dataBuffer.subspan(activeChannels * segmentSize + startIndex, static_cast<std::size_t>(noOfSamples));
                        const auto higherBits = dataBuffer.subspan((activeChannels + 1) * segmentSize + startIndex, static_cast<std::size_t>(noOfSamples));
                        dataContext->ctx.dataDigital.clear();
                        dataContext->ctx.dataDigital.resize(static_cast<std::size_t>(noOfSamples));
                        for (std::size_t i = 0; i < dataContext->ctx.dataDigital.size(); i++) {
                            dataContext->ctx.dataDigital[i] = static_cast<int16_t>((lowerBits[i] & 0xFF) | ((higherBits[i] << 8) & 0xFF00));
                        }
                        acquisitionData[activeChannels] = std::span{dataContext->ctx.dataDigital};
                        ++activeChannels;
                    }
                }
                if (dataContext->handler) {
                    dataContext->handler.value()(std::span(acquisitionData).subspan(0, activeChannels), overflow);
                }
            });
            if (const PICO_STATUS res = scope.instance.getStreamingLatestValues(streamingReadyCallback, &valueContext); res != PICO_OK) {
                return std::unexpected(Error(res));
            }
            return {};
        }

        std::expected<void, Error> stop() {
            if (started) {
                if (const PICO_STATUS res = scope.instance.driverStop(); res != PICO_OK) {
                    return std::unexpected(Error(res));
                }
                started = false;
            }
            return {};
        }

        std::expected<void, Error> start() {
            if (!started) {
                auto activeChannels = static_cast<std::size_t>(std::ranges::count_if(scope.channel_config, [](const auto& ch) { return ch.second.enable; }));
                activeChannels += enableDigital ? 2 : 0; // The digital ports use 2 buffers for the lower and higher 8 bit
                if (scope.verbose) {
                    std::println("starting streaming: active channels: {}, enableDigital: {}", activeChannels, enableDigital);
                }
                if (activeChannels == 0) { // early return if no channels are active
                    return std::unexpected{Error{"No channels configured"}};
                }
                scope.data.resize(65536);                                           // todo: figure out better buffer size strategy
                const std::size_t segmentSize = scope.data.size() / activeChannels; // TODO: maybe resize this according to sample rate and number of channels
                std::size_t       j           = 0;
                for (const auto& [output, chan] : std::views::zip(TPSImpl::outputs, scope.channel_config | std::views::values)) {
                    if (chan.enable) {
                        auto channel = output.second;
                        ;
                        auto bufferStart = scope.data.data() + j * segmentSize;
                        if (const PICO_STATUS res = scope.instance.setDataBuffer(channel, bufferStart, static_cast<int32_t>(segmentSize), TPSImpl::ratioNone); res != PICO_OK) {
                            return std::unexpected(Error(res));
                        }
                        j++;
                    }
                }
                if constexpr (TPSImpl::N_DIGITAL_CHANNELS > 0) {
                    if (enableDigital || std::holds_alternative<unsigned int>(scope.trigger_config.source)) {
                        const std::int16_t digital_threshold = std::holds_alternative<unsigned int>(scope.trigger_config.source) ? scope.trigger_config.threshold : static_cast<std::int16_t>(1.5f / 5.0f * 32767.0f);
                        if (scope.verbose) {
                            std::println("enable digital ports with threshold of {}", digital_threshold);
                        }
                        if (const auto status = scope.instance.setDigitalPorts(true, digital_threshold); status != PICO_OK) {
                            return std::unexpected(Error{status});
                        }
                    } else {
                        if (const auto status = scope.instance.setDigitalPorts(false, 0U); status != PICO_OK) {
                            return std::unexpected(Error{status});
                        }
                    }
                    if (enableDigital) {
                        for (std::size_t i = 0; i < 2; i++) {
                            auto bufferStart = scope.data.data() + j * segmentSize;
                            if (const PICO_STATUS res = scope.instance.setDataBuffer(static_cast<typename TPSImpl::ChannelType>(TPSImpl::DIGI_PORT_0 + i), bufferStart, static_cast<int32_t>(segmentSize), TPSImpl::ratioNone); res != PICO_OK) {
                                return std::unexpected(Error(res));
                            }
                            j++;
                        }
                    } else {
                        std::vector<std::int16_t>{}.swap(dataDigital); // free the memory used to store the digital data
                    }
                }
                if (const PICO_STATUS res = scope.instance.maximumValue(&scope.maxValue); res != PICO_OK) {
                    return std::unexpected(Error(res));
                }
                auto              timeInterval = detail::convertSampleRateToTimeInterval(freq);
                const PICO_STATUS res          = scope.instance.runStreaming( //
                    &timeInterval.interval,                          // in: desired interval, out: actual interval
                    TPSImpl::convertTimeUnits(timeInterval.unit),    // time unit of the interval
                    0, static_cast<uint32_t>(segmentSize),           // pre-trigger-samples (unused) and post-trigger-samples
                    false,                                           // autoStop
                    1, scope.instance.ratioNone,                     // downsampling ratio and mode
                    static_cast<uint32_t>(segmentSize));             // the size of the overview buffers
                if (res != PICO_OK) {
                    return std::unexpected(Error(res));
                }
                actualFreq = detail::convertTimeIntervalToSampleRate(timeInterval);
                started    = true;
            }
            return {};
        }

        std::expected<void, Error> restart() {
            if (const auto res = stop(); !res.has_value()) {
                return res;
            }
            if (const auto res = start(); !res.has_value()) {
                return res;
            }
            return {};
        }

        ~StreamingAcquisitionContext() {
            if (const auto ret = stop(); !ret) {
                scope.lastError = ret.error();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{1}); // Wait for the driver to actually stop
            // This is quite ugly. However, since the driver callback only takes a void* ptr there is no way to manage the lifetime for anything to communicate
            // to the running thread that the picoscope is already deconstructed and the buffer does not exist any more.
        }
    };

    struct TriggeredAcquisitionContext {
        PicoscopeWrapper&         scope;
        bool                      started     = false;
        bool                      initialised = false;
        float                     freq;
        float                     actualFreq         = 0.0f;
        std::uint32_t             pre                = 0U;
        std::uint32_t             post               = 1024U;
        std::uint32_t             nCaptures          = 1U;
        bool                      enableDigital      = false;
        std::uint32_t             nCapturesCompleted = 0U;
        std::uint32_t             nCapturesProcessed = 0U;
        std::int16_t              ready              = false;
        std::function<void()>     callback;
        std::vector<std::int16_t> dataDigital{};

        explicit TriggeredAcquisitionContext(PicoscopeWrapper<TPSImpl>& _scope, const float _freq, const std::uint32_t _pre, const std::uint32_t _post, const std::uint32_t _n_captures, std::function<void()> _fn, const bool _enableDigital) : scope{_scope}, freq{_freq}, pre{_pre}, post{_post}, nCaptures{_n_captures}, enableDigital{_enableDigital}, callback{std::move(_fn)} {}

        TriggeredAcquisitionContext(TriggeredAcquisitionContext&)            = delete;
        TriggeredAcquisitionContext& operator=(TriggeredAcquisitionContext&) = delete;

        std::expected<void, Error> poll(const HandlerT& dataHandler) {
            if (scope.restartAcquisition) {
                std::ignore              = restart();
                scope.restartAcquisition = false;
            }
            if (!started) {
                if (scope.running) {
                    if (auto result = start(); !result) {
                        return result;
                    }
                } else {
                    return {};
                }
            }
            if (const PICO_STATUS res = scope.instance.isReady(&ready); res != PICO_OK) {
                return std::unexpected(Error(res));
            }
            if (ready && scope.verbose) {
                std::println("\nready: {}", ready);
            }
            if (ready || !scope.running) { // acquisition has terminated naturally or was requested to be ended
                std::uint32_t noOfSamples = pre + post;
                std::int16_t  overflow    = 0;
                if (!scope.running) { // was requested to be closed -> abort running acquisition
                    if (scope.verbose) {
                        std::println("stopping active acquisition");
                    }
                    if (const PICO_STATUS res = scope.instance.driverStop(); res != PICO_OK) {
                        return std::unexpected(Error(res));
                    };
                }
                if (const PICO_STATUS res = scope.instance.getNoOfCaptures(&nCapturesCompleted); res != PICO_OK) {
                    return std::unexpected(Error(res));
                }
                if (const PICO_STATUS res = scope.instance.getNoOfProcessedCaptures(&nCapturesProcessed); res != PICO_OK) {
                    return std::unexpected(Error(res));
                }
                if (!scope.running) {
                    if (scope.verbose) {
                        std::println("stopped active acquisition: {} captures completed, {} captures processed", nCapturesCompleted, nCapturesProcessed);
                    }
                    if (nCapturesCompleted == 0U) {
                        return {};
                    }
                }
                if (const PICO_STATUS res = scope.instance.getValuesBulk(&noOfSamples, 0U, nCapturesCompleted - 1, 1U, TPSImpl::ratioNone, &overflow); res != PICO_OK) {
                    if (scope.verbose) {
                        std::println("Error: nCapturesCompleted: {}, nCapturesProcessed: {}, noOfSamples: {}, Error: {}: {}", nCapturesCompleted, nCapturesProcessed, noOfSamples, detail::statusToString(res), detail::statusToStringVerbose(res));
                    }
                    return std::unexpected(Error(res));
                };
                constexpr std::size_t                          channels = TPSImpl::N_ANALOG_CHANNELS + (TPSImpl::N_DIGITAL_CHANNELS > 0UZ ? 1UZ : 0UZ);
                std::array<std::span<const int16_t>, channels> acquisitionData;
                for (std::size_t i = 0UZ; i < nCapturesCompleted; i++) {
                    std::size_t j = 0;
                    for (auto& chan : scope.channel_config | std::views::values) {
                        if (chan.enable) {
                            acquisitionData[j] = std::span(scope.data).subspan(i * noOfSamples + j * (pre + post), pre + post);
                            ++j;
                        }
                    }
                    if constexpr (TPSImpl::N_DIGITAL_CHANNELS > 0) {
                        if (enableDigital) {
                            const auto lowerBits  = std::span{scope.data}.subspan(j * (pre + post), noOfSamples);
                            const auto higherBits = std::span{scope.data}.subspan((j + 1) * (pre + post), noOfSamples);
                            dataDigital.clear();
                            dataDigital.resize(noOfSamples);
                            for (std::size_t k = 0; k < dataDigital.size(); k++) {
                                dataDigital[k] = static_cast<int16_t>((lowerBits[k] & 0xFF) | ((higherBits[k] << 8) & 0xFF00));
                            }
                            acquisitionData[j] = std::span{dataDigital}.subspan(0, noOfSamples);
                            ++j;
                        }
                    }
                    if (dataHandler) {
                        dataHandler.value()(std::span(acquisitionData).subspan(0, j), overflow);
                    }
                }
                started            = false; // getValuesBulk automatically stops any acquisition that would still be in progress
                ready              = false;
                nCapturesCompleted = 0U;
                nCapturesProcessed = 0U;
                if (scope.running) {
                    std::ignore = start(); // re-arm for next acquisition
                }
            }
            return {};
        }

        std::expected<void, Error> stop() {
            if (started) {
                if (const PICO_STATUS res = scope.instance.driverStop(); res != PICO_OK) {
                    return std::unexpected(Error(res));
                }
                started     = false;
                initialised = false;
            }
            return {};
        }

        std::expected<void, Error> start() {
            if (!started) {
                if (scope.verbose) {
                    std::println("starting triggered acquisition, enableDigital: {}", enableDigital);
                }
                if (!initialised) {
                    auto activeChannels = static_cast<std::size_t>(std::ranges::count_if(scope.channel_config, [](const auto& ch) { return ch.second.enable; }));
                    activeChannels += enableDigital ? 2 : 0; // The digital ports use 2 buffers for the lower and higher 8 bit
                    if (activeChannels == 0) {               // early return if no channels are active
                        return std::unexpected{Error{"No channels configured"}};
                    }
                    int32_t maxSamples = 0;
                    if (const auto status = scope.instance.memorySegments(nCaptures, &maxSamples); status != PICO_OK) {
                        return std::unexpected(Error(status));
                    }
                    if (const auto status = scope.instance.setNoOfCaptures(nCaptures); status != PICO_OK) {
                        return std::unexpected(Error(status));
                    }
                    // initialise data buffers to copy to
                    const std::size_t subsegmentSize = pre + post;
                    if (subsegmentSize > static_cast<std::size_t>(maxSamples)) {
                        return std::unexpected{Error{std::format("configured acquisition size(pre+post={}) does not fit into the available memory ({}) for {} captures", subsegmentSize, maxSamples, nCaptures)}};
                    }
                    const std::size_t segmentSize = subsegmentSize * nCaptures;
                    scope.data.resize(segmentSize * activeChannels);
                    std::size_t j = 0;
                    for (const auto& [i, chan] : std::views::zip(std::views::iota(0UZ), scope.channel_config | std::views::values)) {
                        if (chan.enable) {
                            for (std::uint32_t segment = 0; segment < nCaptures; segment++) {
                                auto channel     = TPSImpl::outputs[i].second;
                                auto bufferStart = scope.data.data() + j * segmentSize + segment * subsegmentSize;
                                auto ratio       = TPSImpl::ratioNone;
                                if (const PICO_STATUS res = scope.instance.setDataBuffer(channel, bufferStart, static_cast<int32_t>(subsegmentSize), segment, ratio); res != PICO_OK) {
                                    return std::unexpected(Error(res));
                                }
                            }
                            j++;
                        }
                    }
                    if constexpr (TPSImpl::N_DIGITAL_CHANNELS > 0) {
                        if (enableDigital || std::holds_alternative<unsigned int>(scope.trigger_config.source)) {
                            const std::int16_t digital_threshold = std::holds_alternative<unsigned int>(scope.trigger_config.source) ? scope.trigger_config.threshold : static_cast<std::int16_t>(1.5f / 5.0f * 32767.0f);
                            if (scope.verbose) {
                                std::println("enable digital ports with threshold of {}", digital_threshold);
                            }
                            if (const auto status = scope.instance.setDigitalPorts(true, digital_threshold); status != PICO_OK) {
                                return std::unexpected(Error{status});
                            }
                        } else {
                            if (const auto status = scope.instance.setDigitalPorts(false, 0U); status != PICO_OK) {
                                return std::unexpected(Error{status});
                            }
                        }
                        if (enableDigital) {
                            for (std::size_t i = 0; i < 2; i++) {
                                auto bufferStart = scope.data.data() + j * segmentSize;
                                if (const PICO_STATUS res = scope.instance.setDataBuffer(static_cast<typename TPSImpl::ChannelType>(TPSImpl::DIGI_PORT_0 + i), bufferStart, static_cast<int32_t>(segmentSize), TPSImpl::ratioNone); res != PICO_OK) {
                                    return std::unexpected(Error(res));
                                }
                                j++;
                            }
                        } else {
                            std::vector<std::int16_t>{}.swap(dataDigital); // free the memory used to store the digital data
                        }
                    }
                    // update maximum value
                    if (const PICO_STATUS res = scope.instance.maximumValue(&scope.maxValue); res != PICO_OK) {
                        return std::unexpected(Error(res));
                    }
                    initialised = true;
                }
                const auto timebaseRes = scope.instance.convertSampleRateToTimebase(freq);
                if (!timebaseRes) {
                    return std::unexpected(timebaseRes.error());
                }
                int32_t           timeIndisposedMs;
                static auto       redirector = [](int16_t, PICO_STATUS, void* vobj) { static_cast<decltype(this)>(vobj)->callback(); };
                const PICO_STATUS res        = scope.instance.runBlock(                    //
                    static_cast<int32_t>(pre), static_cast<int32_t>(post),          // pre- and post-samples to capture around the trigger
                    timebaseRes->timebase,                                          //
                    &timeIndisposedMs,                                              // returns the time spent in acquisition without waiting for timers and delays
                    0,                                                              // starting segment index
                    static_cast<typename TPSImpl::BlockReadyType>(redirector), this // callback to be called once acquisition has completed and the void* pointer passed to it
                );
                if (res != PICO_OK) {
                    return std::unexpected(Error(res));
                }
                actualFreq = timebaseRes->actualFreq;
                started    = true;
                if (scope.verbose) {
                    std::println("started triggered acquisition");
                }
            }
            return {};
        }

        std::expected<void, Error> restart() {
            if (const auto res = stop(); !res.has_value()) {
                return res;
            }
            if (const auto res = start(); !res.has_value()) {
                return res;
            }
            return {};
        }

        ~TriggeredAcquisitionContext() {
            if (const auto ret = stop(); !ret) {
                scope.lastError = ret.error();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{1}); // Wait for the driver to actually stop
            // This is quite ugly. However, since the driver callback only takes a void* ptr there is no way to manage the lifetime for anything to communicate
            // to the running thread that the picoscope is already deconstructed and the buffer does not exist any more.
        }
    };

    using ContextVariant = std::variant<std::monostate, StreamingAcquisitionContext, TriggeredAcquisitionContext>;
    std::string          serial;
    bool                 verbose = false;
    OpeningContext       openingContext{};
    ContextVariant       activeContext{std::monostate{}};
    TPSImpl              instance; // mainly stores the handle
    std::vector<int16_t> data;     // data buffer used by the picoscope to store acquisition data
    AcquisitionMode      acquisitionMode = AcquisitionMode::Streaming;

    using ChannelConfigType = std::array<std::pair<bool, ChannelConfig>, TPSImpl::N_ANALOG_CHANNELS>;
    ChannelConfigType channel_config{};
    TriggerConfig     trigger_config;
    bool              restartAcquisition = false;
    bool              running            = true;

    std::size_t                           maxErrors   = 3;                              // maximum number of retries before resetting the scope completely
    std::chrono::milliseconds             retryPeriod = std::chrono::milliseconds(500); // how long to wait between retries
    std::size_t                           errorCount  = 0;                              // store the performed retries in case of function failures
    std::chrono::steady_clock::time_point lastTry;                                      // saves the timestamp of the last failed attempt.
    std::optional<Error>                  lastError{};                                  // stores the last error when talking to the driver

    std::int16_t maxValue = std::numeric_limits<std::int16_t>::max();

    DeviceInformation info;

    std::expected<void, Error> setChannel(const std::size_t id, ChannelConfig config) {
        if (!openingContext.ready() || std::holds_alternative<std::monostate>(activeContext)) {
            return std::unexpected(Error("Trying to set channel on picoscope that is not yet started"));
        }
        if (id >= TPSImpl::outputs.size()) {
            return std::unexpected(Error(std::format("Invalid channel id {}", id)));
        }
        auto       chan     = TPSImpl::outputs[id].second;
        const auto coupling = TPSImpl::convertToCoupling(config.coupling);
        if (!coupling) {
            return std::unexpected(coupling.error());
        }
        typename TPSImpl::RangeType rng = toRangeEnum(config.range).value();
        if (const PICO_STATUS status = instance.setChannel(chan, config.enable, coupling.value(), rng, config.offset); status != PICO_OK) {
            std::print("error setting channel: {}, {}, {}: {}", magic_enum::enum_name(TPSImpl::outputs[id].first), magic_enum::enum_name(config.range), detail::statusToString(status), detail::statusToStringVerbose(status));
            return std::unexpected(Error{status});
        }
        return {};
    }

    std::expected<void, Error> setTrigger(TriggerConfig config) {
        if (!openingContext.ready()) {
            return std::unexpected(Error("Trying to set trigger on picoscope that is not yet started"));
        }
        if (std::holds_alternative<ChannelName>(config.source)) { // analog trigger
            auto chan = toChannelEnum(std::get<ChannelName>(config.source));
            if (!chan.has_value()) {
                return std::unexpected(chan.error());
            }
            const auto direction = TPSImpl::convertToThresholdDirection(config.direction);
            if (!direction) {
                return std::unexpected(direction.error());
            }
            if (const PICO_STATUS res = instance.setSimpleTrigger(true, chan.value(), config.threshold, direction.value(), config.delay, config.auto_trigger_ms); res != PICO_OK) {
                return std::unexpected(Error{res});
            }
            if (verbose) {
                std::println("configured analog trigger: {} {} {} {} {}", magic_enum::enum_name(std::get<ChannelName>(config.source)), magic_enum::enum_name(config.direction), config.threshold, config.delay, config.auto_trigger_ms);
            }
            return {};
        }
        if (std::holds_alternative<uint>(config.source)) { // digital trigger
            if constexpr (TPSImpl::N_DIGITAL_CHANNELS > 0UZ) {
                uint digitalPin = std::get<uint>(config.source);
                if (verbose) {
                    std::println("setting up digital trigger for D{}", digitalPin);
                }
                if (digitalPin >= TPSImpl::N_DIGITAL_CHANNELS) {
                    if (verbose) {
                        std::println("Configured digital trigger pin {} out of range [0, {}]", digitalPin, TPSImpl::N_DIGITAL_CHANNELS);
                    }
                    return std::unexpected(Error{std::format("Configured digital trigger pin {} out of range [0, {}]", digitalPin, TPSImpl::N_DIGITAL_CHANNELS)});
                }
                if (const PICO_STATUS res = instance.setTriggerDigitalPort(digitalPin, config.direction); res != PICO_OK) {
                    if (verbose) {
                        std::println("error setting up digital trigger for D{}: {}: {}", digitalPin, detail::statusToString(res), detail::statusToStringVerbose((res)));
                    }
                    return std::unexpected(Error{res});
                }
                if (verbose) {
                    std::println("configured digital trigger");
                }
                return {};
            } else {
                if (verbose) {
                    std::println("tried to set up digital trigger but scope has no digital inputs");
                }
                return std::unexpected(Error{"Picoscope model does not support digital triggers"});
            }
        }
        // disable all triggers
        if (verbose) {
            std::println("clearing all triggers");
        }
        const auto direction = instance.convertToThresholdDirection(TriggerDirection::Rising);
        if (!direction) {
            return std::unexpected(direction.error());
        }
        if (const auto status = instance.setSimpleTrigger(false, TPSImpl::outputs[0].second, 0, direction.value(), 0 /* delay */, 0 /* auto trigger */); status != PICO_OK) {
            return std::unexpected(Error{status});
        }
        return {};
    }

public:
    static std::expected<typename TPSImpl::ChannelType, Error> toChannelEnum(ChannelName chan) {
        for (auto& [globalEnum, deviceEnum] : TPSImpl::outputs) {
            if (globalEnum == chan) {
                return deviceEnum;
            }
        }
        return std::unexpected(Error(std::format("Channel not found ({})", chan)));
    }

    static std::expected<typename TPSImpl::RangeType, Error> toRangeEnum(AnalogChannelRange range) {
        for (auto& [globalEnum, deviceEnum] : TPSImpl::ranges) {
            if (globalEnum == range) {
                return deviceEnum;
            }
        }
        return std::unexpected(Error(std::format("Range {} is not supported by device", magic_enum::enum_name(range))));
    }

    explicit PicoscopeWrapper(const std::string_view _serial, bool _verbose) : serial{_serial}, verbose{_verbose}, openingContext{*this} {}

    ~PicoscopeWrapper() {
        activeContext.template emplace<std::monostate>(); // stop currently running acquisition
        if (const PICO_STATUS ret = instance.closeUnit(); ret != PICO_OK) {
            lastError = Error{ret};
        }
    }

    void startStreamingAcquisition(float freq, bool enableDigital = false) { activeContext.template emplace<StreamingAcquisitionContext>(*this, freq, enableDigital); }

    void startTriggeredAcquisition(float freq, std::size_t pre, std::size_t post, std::size_t n_captures, const std::function<void()>& callback, bool enableDigital = false) { activeContext.template emplace<TriggeredAcquisitionContext>(*this, freq, pre, post, n_captures, callback, enableDigital); }

    void stopAcquisition() { activeContext.template emplace<std::monostate>(); }

    std::expected<void, Error> handleError(const Error& error) {
        lastError = error;
        ++errorCount;
        lastTry = std::chrono::steady_clock::now();
        if (verbose) {
            std::println("error occurred({}): {}:{}", errorCount, lastError->getError(), lastError->getDescription());
        }
        return {}; // only report errors after the retries have been used up.
    }

    std::expected<void, Error> poll(const HandlerT& fn = std::nullopt) {
        if (errorCount > maxErrors) {
            return std::unexpected(lastError.value_or(Error("unknown error")));
        }
        if (lastTry + retryPeriod > std::chrono::steady_clock::now()) {
            return {}; // wait for some time before retrying operation
        }
        if (auto result = openingContext.poll(); !result.has_value()) {
            return handleError(result.error()); // error checking opening progress
        } else if (!*result) {
            errorCount = 0;
            return {}; // scope has not yet finished opening
        }
        std::size_t i = 0UZ;
        for (auto& [modified, chan] : channel_config) {
            if (modified) {
                if (auto result = setChannel(i, chan); !result.has_value()) {
                    return handleError(result.error());
                }
                modified           = false;
                restartAcquisition = true;
            }
            i++;
        }
        if (trigger_config.modified) {
            if (verbose) {
                std::println("configuring trigger");
            }
            if (auto result = setTrigger(trigger_config); !result.has_value()) {
                if (verbose) {
                    std::println("error configuring trigger");
                }
                return handleError(result.error());
            }
            trigger_config.modified = false;
            restartAcquisition      = true;
            if (verbose) {
                std::println("configured trigger");
            }
        }
        if (!openingContext.ready()) {
            errorCount = 0;
            return {}; // acquisition not yet started
        }
        if (std::holds_alternative<StreamingAcquisitionContext>(activeContext)) {
            const auto result = std::get<StreamingAcquisitionContext>(activeContext).poll(fn);
            if (!result.has_value()) {
                return handleError(result.error());
            }
            errorCount = 0;
            return result;
        }
        if (std::holds_alternative<TriggeredAcquisitionContext>(activeContext)) {
            const auto result = std::get<TriggeredAcquisitionContext>(activeContext).poll(fn);
            if (!result.has_value()) {
                return handleError(result.error());
            }
            errorCount = 0;
            return result;
        }
        errorCount = 0;
        return {};
    }

    const DeviceInformation& getDeviceInfo() { return info; }

    [[nodiscard]] const std::optional<Error>& getLastError() const { return lastError; };

    bool ready() { return openingContext.ready(); }

    std::vector<ChannelName> getChannelIds() {
        return TPSImpl::outputs                                                                                   //
               | std::views::transform([](const auto& out) -> ChannelName { return std::get<ChannelName>(out); }) //
               | std::views::take(TPSImpl::N_ANALOG_CHANNELS)                                                     //
               | std::ranges::to<std::vector>();
    }

    [[nodiscard]] const ChannelConfig& getChannelConfig(std::size_t id) const { return channel_config[id].second; };

    void configureChannel(std::size_t id, ChannelConfig chan) {
        bool modified      = channel_config[id].second != chan;
        channel_config[id] = {modified, chan};
    }

    [[nodiscard]] const TriggerConfig& getTriggerConfig() const { return trigger_config; };

    void configureTrigger(TriggerConfig config) {
        config.modified = trigger_config.modified;
        config.modified = trigger_config != config;
        trigger_config  = config;
    }

    void setPaused(const bool value) { running = !value; }

    std::size_t convertToOutputIndex(const std::string_view name) {
        for (std::size_t i = 0; i < TPSImpl::outputs.size(); i++) {
            if (TPSImpl::outputs[i].first == toChannel(name)) {
                return i;
            }
        }
        return std::numeric_limits<std::size_t>::max();
    }
};

/**
 * Type erased Wrapper (mainly used for interactive demo).
 */
class PicoscopeDynamicWrapperBase {
public:
    virtual ~PicoscopeDynamicWrapperBase()                                                                                                                                                                 = default;
    virtual std::expected<void, Error>                poll(std::optional<std::function<void(std::span<std::span<const std::int16_t>>, int16_t)>> fn)                                                       = 0;
    virtual std::vector<ChannelName>                  getChannelIds()                                                                                                                                      = 0;
    [[nodiscard]] virtual const ChannelConfig&        getChannelConfig(std::size_t) const                                                                                                                  = 0;
    virtual void                                      configureChannel(std::size_t, ChannelConfig)                                                                                                         = 0;
    [[nodiscard]] virtual const TriggerConfig&        getTriggerConfig() const                                                                                                                             = 0;
    virtual void                                      configureTrigger(TriggerConfig config)                                                                                                               = 0;
    [[nodiscard]] virtual const std::optional<Error>& getLastError() const                                                                                                                                 = 0;
    virtual bool                                      ready()                                                                                                                                              = 0;
    virtual const DeviceInformation&                  getDeviceInfo()                                                                                                                                      = 0;
    virtual void                                      startStreamingAcquisition(float freq, bool enableDigital)                                                                                            = 0;
    virtual void                                      startTriggeredAcquisition(float freq, std::size_t pre, std::size_t post, std::size_t n_captures, std::function<void()> callback, bool enableDigital) = 0;
    virtual void                                      stopAcquisition()                                                                                                                                    = 0;
};
template<PicoscopeImplementationLike TPSImplementation>
class PicoscopeDynamicWrapper final : public PicoscopeDynamicWrapperBase {
public:
    PicoscopeWrapper<TPSImplementation> instance;
    explicit PicoscopeDynamicWrapper(std::string_view _serial, bool verbose) : instance{_serial, verbose} {};
    ~PicoscopeDynamicWrapper() override = default;
    std::expected<void, Error>                poll(std::optional<std::function<void(std::span<std::span<const std::int16_t>>, int16_t)>> fn) override { return instance.poll(fn); }
    std::vector<ChannelName>                  getChannelIds() override { return instance.getChannelIds(); };
    [[nodiscard]] const ChannelConfig&        getChannelConfig(std::size_t id) const override { return instance.getChannelConfig(id); };
    void                                      configureChannel(std::size_t id, ChannelConfig config) override { return instance.configureChannel(id, config); };
    [[nodiscard]] const TriggerConfig&        getTriggerConfig() const override { return instance.getTriggerConfig(); };
    void                                      configureTrigger(TriggerConfig config) override { return instance.configureTrigger(config); };
    [[nodiscard]] const std::optional<Error>& getLastError() const override { return instance.getLastError(); };
    bool                                      ready() override { return instance.ready(); };
    const DeviceInformation&                  getDeviceInfo() override { return instance.getDeviceInfo(); }
    void                                      startStreamingAcquisition(float freq, bool enableDigital) override { instance.startStreamingAcquisition(freq, enableDigital); }
    void                                      startTriggeredAcquisition(float freq, std::size_t pre, std::size_t post, std::size_t n_captures, const std::function<void()> callback, bool enableDigital) override { instance.startTriggeredAcquisition(freq, pre, post, n_captures, callback, enableDigital); }
    void                                      stopAcquisition() override { instance.stopAcquisition(); };
};

} // namespace fair::picoscope
#endif // GR_DIGITIZERS_PICOSCOPEAPI_HPP
