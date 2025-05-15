#ifndef FAIR_PICOSCOPE_PICOSCOPE_HPP
#define FAIR_PICOSCOPE_PICOSCOPE_HPP

#include "StatusMessages.hpp"

#include <PicoConnectProbes.h>
#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/algorithm/dataset/DataSetUtils.hpp>

#include <format>

#include <chrono>
#include <functional>
#include <queue>

namespace fair::picoscope {

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

namespace detail {

constexpr std::size_t kDriverBufferSize = 65536;

[[nodiscard]] inline bool isDigitalTrigger(std::string_view source) { return !source.empty() && source.starts_with("DI"); }
[[nodiscard]] inline bool isAnalogTrigger(std::string_view source) { return !source.empty() && !source.starts_with("DI"); }

[[nodiscard]] std::expected<int, gr::Error> parseDigitalTriggerSource(std::string_view triggerSrc) {
    if (!triggerSrc.starts_with("DI")) {
        return std::unexpected(gr::Error(std::format("Cannot parse digital trigger source (`{}`): it must start with `DI`.", triggerSrc)));
    }
    std::string_view numberPart = triggerSrc.substr(2);
    if (numberPart.empty()) {
        return std::unexpected(gr::Error(std::format("Cannot parse digital trigger source (`{}`): No digital channel number.", triggerSrc)));
    }
    int value{-1};
    auto [_, ec] = std::from_chars(numberPart.data(), numberPart.data() + numberPart.size(), value);
    if (ec != std::errc()) {
        return std::unexpected(gr::Error(std::format("Cannot parse digital trigger source (`{}`): Error parsing digital channel number.", triggerSrc)));
    }

    if (value < 0 || value > 15) {
        return std::unexpected(gr::Error(std::format("Cannot parse digital trigger source (`{}`): channel number is out of range [0, 15].", triggerSrc)));
    }

    return value;
}

struct Channel {
    std::string          id;
    std::vector<int16_t> driverBuffer;
    bool                 signalInfoTagPublished = false;

    // settings
    std::string name;
    float       sampleRate;
    std::string unit         = "V";
    std::string quantity     = "Voltage";
    float       range        = 2.f;
    float       analogOffset = 0.f;
    float       scale        = 1.f;
    float       offset       = 0.f;
    Coupling    coupling     = Coupling::DC;

    [[nodiscard]] gr::property_map toTagMap() const {
        return {{std::string(gr::tag::SIGNAL_NAME.shortKey()), name},      //
            {std::string(gr::tag::SAMPLE_RATE.shortKey()), sampleRate},    //
            {std::string(gr::tag::SIGNAL_QUANTITY.shortKey()), quantity},  //
            {std::string(gr::tag::SIGNAL_UNIT.shortKey()), unit},          //
            {std::string(gr::tag::SIGNAL_MIN.shortKey()), offset - range}, //
            {std::string(gr::tag::SIGNAL_MAX.shortKey()), offset + range}};
    }
};

struct TriggerNameAndCtx {
    std::string triggerName{};
    std::string ctx{};
    bool        isCtxSet{true}; // This is needed to differentiate whether an empty ("") context is set.
};

[[nodiscard]] inline TriggerNameAndCtx createTriggerNameAndCtx(const std::string& triggerNameAndCtx) {
    if (triggerNameAndCtx.empty()) {
        return {};
    }
    const std::size_t pos = triggerNameAndCtx.find('/');
    if (pos != std::string::npos) { // trigger_name and ctx
        return {triggerNameAndCtx.substr(0, pos), (pos < triggerNameAndCtx.size() - 1) ? triggerNameAndCtx.substr(pos + 1) : "", true};
    } else { // only trigger_name
        return {triggerNameAndCtx, "", false};
    }
}

[[nodiscard]] inline bool tagContainsTrigger(const gr::Tag& tag, const TriggerNameAndCtx& triggerNameAndCtx) {
    if (triggerNameAndCtx.isCtxSet) { // trigger_name and ctx
        if (tag.map.contains(gr::tag::TRIGGER_NAME.shortKey()) && tag.map.contains(gr::tag::CONTEXT.shortKey())) {
            const std::string tagTriggerName = std::get<std::string>(tag.map.at(gr::tag::TRIGGER_NAME.shortKey()));
            return !tagTriggerName.empty() && tagTriggerName == triggerNameAndCtx.triggerName && std::get<std::string>(tag.map.at(gr::tag::CONTEXT.shortKey())) == triggerNameAndCtx.ctx;
        }
    } else { // only trigger_name
        if (tag.map.contains(gr::tag::TRIGGER_NAME.shortKey())) {
            const std::string tagTriggerName = std::get<std::string>(tag.map.at(gr::tag::TRIGGER_NAME.shortKey()));
            return !tagTriggerName.empty() && tagTriggerName == triggerNameAndCtx.triggerName;
        }
    }
    return false;
}

template<typename TEnum>
[[nodiscard]] inline constexpr TEnum convertToEnum(std::string_view strEnum) {
    auto enumType = magic_enum::enum_cast<TEnum>(strEnum, magic_enum::case_insensitive);
    if (!enumType.has_value()) {
        throw std::invalid_argument(std::format("Unknown value. Cannot convert string '{}' to enum '{}'", strEnum, gr::meta::type_name<TEnum>()));
    }
    return enumType.value();
}
} // namespace detail

// optional shortening
template<typename T, gr::meta::fixed_string description = "", typename... Arguments>
using A = gr::Annotated<T, description, Arguments...>;

using gr::Visible;

/**
 * We allow only a small set of sample types (SampleType) to be used as the output type for the Picoscope block:
 * - `std::int16_t`: Outputs raw values as-is, without any scaling. Also used for 8-bit native ADC.
 * - `float`: Outputs the physically gain-scaled values of the measurements.
 * - `gr::UncertainValue<float>`: Similar to `float`, but also includes an estimated measurement error as an additional component.
 *
 * The output type also determines the acquisition mode:
 * - For `DataSet<SampleType>`, the acquisition mode is **RapidBlock**.
 * - For `SampleType`, the acquisition mode is **Streaming**.
 */

template<typename T>
concept PicoscopeOutput = std::disjunction_v<std::is_same<T, std::int16_t>, std::is_same<T, float>, std::is_same<T, gr::UncertainValue<float>>, //
    std::is_same<T, gr::DataSet<std::int16_t>>, std::is_same<T, gr::DataSet<float>>, std::is_same<T, gr::DataSet<gr::UncertainValue<float>>>>;

// helper struct to conditionally enable BlockingIO at compile time
template<typename TPSImpl, bool>
struct PicoscopeBlockingHelper;

template<typename TPSImpl>
struct PicoscopeBlockingHelper<TPSImpl, false> { // Streaming mode
    using type = gr::Block<TPSImpl, gr::SupportedTypes<int16_t, float, gr::UncertainValue<float>>>;
};

template<typename TPSImpl>
struct PicoscopeBlockingHelper<TPSImpl, true> { // RapidBlock mode
    using type = gr::Block<TPSImpl, gr::SupportedTypes<gr::DataSet<int16_t>, gr::DataSet<float>, gr::DataSet<gr::UncertainValue<float>>>, gr::BlockingIO<false>>;
};

template<PicoscopeOutput T, typename TPSImpl>
struct Picoscope : public PicoscopeBlockingHelper<TPSImpl, gr::DataSetLike<T>>::type {
    using super_t                                    = typename PicoscopeBlockingHelper<TPSImpl, gr::DataSetLike<T>>::type;
    static constexpr AcquisitionMode acquisitionMode = gr::DataSetLike<T> ? AcquisitionMode::RapidBlock : AcquisitionMode::Streaming;

    Picoscope(gr::property_map props) : super_t(std::move(props)) {}

    A<std::string, "serial number">                                               serial_number;
    A<float, "sample rate", Visible>                                              sample_rate              = 10000.f;
    A<gr::Size_t, "pre-samples">                                                  pre_samples              = 1000;  // RapidBlock mode only
    A<gr::Size_t, "post-samples">                                                 post_samples             = 1000;  // RapidBlock mode only
    A<gr::Size_t, "no. captures (rapid block mode)">                              n_captures               = 1;     // RapidBlock mode only
    A<bool, "trigger once (rapid block mode)">                                    trigger_once             = false; // RapidBlock mode only
    A<float, "poll rate (streaming mode)">                                        streaming_mode_poll_rate = 0.001; // TODO, not used for the moment
    A<bool, "do arm at start?">                                                   auto_arm                 = true;
    A<std::vector<std::string>, "IDs of enabled channels: `A`, `B`, `C` etc.">    channel_ids;
    A<std::vector<float>, "Voltage range of enabled channels">                    channel_ranges;         // PS channel setting
    A<std::vector<float>, "Voltage offset of enabled channels">                   channel_analog_offsets; // PS channel setting
    A<std::vector<std::string>, "Coupling modes of enabled channels">             channel_couplings;
    A<std::vector<std::string>, "Signal names of enabled channels">               signal_names;
    A<std::vector<std::string>, "Signal units of enabled channels">               signal_units;
    A<std::vector<std::string>, "Signal quantity of enabled channels">            signal_quantities;
    A<std::vector<float>, "Signal scales of the enabled channels">                signal_scales;  // only for floats and UncertainValues
    A<std::vector<float>, "Signal offset of the enabled channels">                signal_offsets; // only for floats and UncertainValues
    A<std::string, "trigger channel (A, B, C, ... or DI1, DI2, DI3, ...)">        trigger_source;
    A<float, "trigger threshold, analog only">                                    trigger_threshold          = 0.f;
    A<std::string, "trigger direction">                                           trigger_direction          = std::string("Rising");
    A<std::string, "trigger filter: `<trigger_name>[/<ctx>]`">                    trigger_filter             = "";
    A<std::string, "arm trigger: `<trigger_name>[/<ctx>]`, if empty not used">    trigger_arm                = ""; // RapidBlock mode only
    A<std::string, "disarm trigger: `<trigger_name>[/<ctx>]`, if empty not used"> trigger_disarm             = ""; // RapidBlock mode only
    A<gr::Size_t, "time between two systemtime tags in ms">                       systemtime_interval        = 1000UZ;
    A<int16_t, "digital port threshold (ADC: –32767 (–5 V) to 32767 (+5 V))">     digital_port_threshold     = 0;     // only used if digital ports are available: 3000a, 5000a series
    A<bool, "invert digital port output">                                         digital_port_invert_output = false; // only used if digital ports are available: 3000a, 5000a series

    gr::PortIn<std::uint8_t, gr::Async> timingIn;

    using TDigitalOutput = std::conditional<gr::DataSetLike<T>, gr::DataSet<uint16_t>, uint16_t>::type;
    gr::PortOut<TDigitalOutput> digitalOut;

    int16_t     _handle            = -1; // identifier for the scope device
    float       _actualSampleRate  = 0;
    std::size_t _nSamplesPublished = 0; // for debugging purposes

    detail::TriggerNameAndCtx _armTriggerNameAndCtx; // store parsed information to optimise performance
    detail::TriggerNameAndCtx _disarmTriggerNameAndCtx;

    int _digitalChannelNumber = -1; // used only if digital trigger is set

    GR_MAKE_REFLECTABLE(Picoscope, timingIn, digitalOut, serial_number, sample_rate, pre_samples, post_samples, n_captures, streaming_mode_poll_rate,                                 //
        auto_arm, trigger_once, channel_ids, signal_names, signal_units, signal_quantities, channel_ranges, channel_analog_offsets, signal_scales, signal_offsets, channel_couplings, //
        trigger_source, trigger_threshold, trigger_direction, digital_port_threshold, digital_port_invert_output, trigger_arm, trigger_disarm, systemtime_interval);

private:
    std::atomic<std::size_t>                       _streamingSamples = 0UZ;
    std::atomic<std::size_t>                       _streamingOffset  = 0UZ;
    std::queue<gr::property_map>                   _timingMessages;
    std::atomic<bool>                              _isArmed = false; // for RapidBlock mode only
    std::vector<detail::Channel>                   _channels;
    int8_t                                         _triggerState   = 0;
    int16_t                                        _maxValue       = 0; // maximum ADC count used for ADC conversion
    std::chrono::high_resolution_clock::time_point _nextSystemtime = std::chrono::high_resolution_clock::now();

    using ReaderType              = decltype(timingIn.buffer().tagBuffer.new_reader());
    ReaderType _tagReaderInternal = timingIn.buffer().tagBuffer.new_reader();

public:
    ~Picoscope() { stop(); }

    template<typename = void>
    gr::work::Result work(std::size_t requestedWork = std::numeric_limits<std::size_t>::max()) noexcept {
        if constexpr (acquisitionMode == AcquisitionMode::Streaming) {
            return this->workInternal(requestedWork);
        } else { // AcquisitionMode::RapidBlock

            const bool blockIsActive = gr::lifecycle::isActive(this->state());
            if (!blockIsActive) {
                this->ioLastWorkStatus.exchange(gr::work::Status::DONE, std::memory_order_relaxed);
            } else {

                if (timingIn.isConnected() && _tagReaderInternal.available() > 0) {
                    gr::ReaderSpanLike auto tagData = _tagReaderInternal.get();

                    if (!trigger_arm.value.empty() || !trigger_disarm.value.empty()) {
                        // find last arm trigger or last disarm trigger
                        const std::size_t IndexNotSet            = std::numeric_limits<std::size_t>::max();
                        std::size_t       lastArmTriggerIndex    = IndexNotSet;
                        std::size_t       lastDisarmTriggerIndex = IndexNotSet;

                        for (int i = static_cast<int>(tagData.size()) - 1; i >= 0; i--) {
                            const auto     iSizeT = static_cast<std::size_t>(i);
                            const gr::Tag& tag    = tagData[iSizeT];

                            if (lastArmTriggerIndex == IndexNotSet && detail::tagContainsTrigger(tag, _armTriggerNameAndCtx)) {
                                lastArmTriggerIndex = iSizeT;
                            }
                            if (lastDisarmTriggerIndex == IndexNotSet && detail::tagContainsTrigger(tag, _disarmTriggerNameAndCtx)) {
                                lastDisarmTriggerIndex = iSizeT;
                            }
                            // both arm/disarm triggers were found
                            if (lastArmTriggerIndex != IndexNotSet && lastDisarmTriggerIndex != IndexNotSet) {
                                break;
                            }
                        }

                        if (lastArmTriggerIndex != IndexNotSet && lastDisarmTriggerIndex == IndexNotSet) { // only arm
                            arm();
                        }

                        if (lastArmTriggerIndex == IndexNotSet && lastDisarmTriggerIndex != IndexNotSet) { // only disarm
                            disarm();
                        }

                        if (lastArmTriggerIndex != IndexNotSet && lastDisarmTriggerIndex != IndexNotSet) { // both arm and disarm
                            disarm();
                            if (lastArmTriggerIndex > lastDisarmTriggerIndex) { // disarm before arm
                                arm();
                            }
                        }
                    } // arm/disarm triggers
                    std::ignore = tagData.consume(tagData.size());
                } // Tags are available

                if (auto_arm) {
                    arm();
                }
            }
            const auto& [accumulatedRequestedWork, performedWork] = this->ioWorkDone.getAndReset();
            return {accumulatedRequestedWork, performedWork, this->ioLastWorkStatus.load()};
        }
    }

    [[nodiscard]] bool isOpened() const { return _handle > 0; }

    void settingsChanged(const gr::property_map& /*oldSettings*/, const gr::property_map& newSettings) {
        _channels.clear();
        _channels.resize(channel_ids.value.size());

        for (std::size_t i = 0; i < channel_ids.value.size(); ++i) {
            detail::Channel& ch = _channels[i];
            ch.id               = channel_ids.value[i];
            ch.sampleRate       = sample_rate;

            if (i < signal_names.value.size()) {
                ch.name = signal_names.value[i];
            } else {
                ch.name = std::format("signal {} ({})", i, channel_ids.value[i]);
            }
            if (i < signal_units.value.size()) {
                ch.unit = std::string(signal_units.value[i]);
            }
            if (i < signal_quantities.value.size()) {
                ch.quantity = std::string(signal_quantities.value[i]);
            }
            if (i < channel_ranges.value.size()) {
                ch.range = channel_ranges.value[i];
            }
            if (i < channel_analog_offsets.value.size()) {
                ch.analogOffset = channel_analog_offsets.value[i];
            }
            if (i < signal_scales.value.size()) {
                ch.scale = signal_scales.value[i];
            }
            if (i < signal_offsets.value.size()) {
                ch.offset = signal_offsets.value[i];
            }
            if (i < channel_couplings.value.size()) {
                ch.coupling = detail::convertToEnum<Coupling>(channel_couplings.value[i]);
            }

            ch.driverBuffer.resize(detail::kDriverBufferSize);
        }

        if (newSettings.contains("trigger_arm") || newSettings.contains("trigger_disarm")) {
            _armTriggerNameAndCtx    = detail::createTriggerNameAndCtx(trigger_arm);
            _disarmTriggerNameAndCtx = detail::createTriggerNameAndCtx(trigger_disarm);

            if (!trigger_arm.value.empty() && !trigger_disarm.value.empty() && trigger_arm == trigger_disarm) {
                this->emitErrorMessage(std::format("{}::settingsChanged()", this->name), gr::Error("Ill-formed settings: `trigger_arm` == `trigger_disarm`"));
            }
        }

        if (newSettings.contains("trigger_source") && detail::isDigitalTrigger(trigger_source)) {
            const auto parseRes = detail::parseDigitalTriggerSource(trigger_source);
            if (parseRes.has_value()) {
                _digitalChannelNumber = parseRes.value();
            } else {
                this->emitErrorMessage(std::format("{}::settingsChanged()", this->name), parseRes.error());
            }
        }

        const bool needsReinit = newSettings.contains("sample_rate") || newSettings.contains("pre_samples") || newSettings.contains("post_samples")                  //
                                 || newSettings.contains("n_captures") || newSettings.contains("streaming_mode_poll_rate") || newSettings.contains("auto_arm")       //
                                 || newSettings.contains("channel_ids") || newSettings.contains("channel_ranges") || newSettings.contains("channel_analog_offsets")  //
                                 || newSettings.contains("channel_couplings") || newSettings.contains("trigger_source") || newSettings.contains("trigger_threshold") //
                                 || newSettings.contains("trigger_direction") || newSettings.contains("digital_port_threshold");

        if (needsReinit) {
            initialize();
            if (auto_arm) {
                disarm();
                arm();
            }
        }
    }

    void start() noexcept {
        _tagReaderInternal = timingIn.buffer().tagBuffer.new_reader();

        if (isOpened()) {
            return;
        }

        try {
            open();
            initialize();
            if (auto_arm) {
                arm();
            }
            serial_number = serialNumber();
            std::println("Picoscope serial number: {}", serial_number);
            std::println("Picoscope device variant: {}", deviceVariant());
        } catch (const std::exception& e) {
            std::println(std::cerr, "{}", e.what());
        }
    }

    void stop() noexcept {
        disarm();
        close();
    }

    void disarm() {
        if (!isOpened()) {
            return;
        }

        if (const auto status = self().driverStop(_handle); status != PICO_OK) {
            this->emitErrorMessage(std::format("{}::disarm()", this->name), gr::Error(detail::getErrorMessage(status)));
        }
        _isArmed.store(false, std::memory_order_release);
    }

    void open() {
        if (isOpened()) {
            return;
        }

        if (const auto status = self().openUnit(serial_number); status == PICO_POWER_SUPPLY_NOT_CONNECTED || status == PICO_USB3_0_DEVICE_NON_USB3_0_PORT) {
            if (const auto statusPower = self().changePowerSource(_handle, status); statusPower != PICO_OK) {
                this->emitErrorMessage(std::format("{}::open() changePowerSource", this->name), gr::Error(detail::getErrorMessage(statusPower)));
            }
        }
    }

    void close() {
        if (!isOpened()) {
            return;
        }
        if (const auto status = self().closeUnit(_handle); status != PICO_OK) {
            this->emitErrorMessage(std::format("{}::close()", this->name), gr::Error(detail::getErrorMessage(status)));
        } else {
            _handle = -1;
        }
    }

    void initialize() {
        if (!isOpened()) {
            return;
        }

        // maximum value is used for conversion to volts
        if (const auto status = self().maximumValue(_handle, &(_maxValue)); status != PICO_OK) {
            self().closeUnit(_handle);
            this->emitErrorMessage(std::format("{}::initialize() maximumValue", this->name), gr::Error(detail::getErrorMessage(status)));
        }

        // configure memory segments and number of capture fo RapidBlock mode
        if constexpr (acquisitionMode == AcquisitionMode::RapidBlock) {
            int32_t maxSamples;
            if (const auto status = self().memorySegments(_handle, static_cast<uint32_t>(n_captures), &maxSamples); status != PICO_OK) {
                this->emitErrorMessage(std::format("{}::initialize() MemorySegments", this->name), gr::Error(detail::getErrorMessage(status)));
            }

            if (const auto status = self().setNoOfCaptures(_handle, static_cast<uint32_t>(n_captures)); status != PICO_OK) {
                this->emitErrorMessage(std::format("{}::initialize() SetNoOfCaptures", this->name), gr::Error(detail::getErrorMessage(status)));
            }
        }

        // configure analog channels
        for (const auto& channel : _channels) {
            const auto channelEnum = self().convertToChannel(channel.id);
            assert(channelEnum);
            const auto coupling = self().convertToCoupling(channel.coupling);
            const auto range    = self().convertToRange(channel.range);

            const auto status = self().setChannel(_handle, *channelEnum, true, coupling, static_cast<TPSImpl::ChannelRangeType>(range), static_cast<float>(channel.analogOffset));
            if (status != PICO_OK) {
                this->emitErrorMessage(std::format("{}::initialize() SetChannel", this->name), gr::Error(detail::getErrorMessage(status)));
            }
        }

        // configure digital ports
        if (const auto status = self().setDigitalPorts(); status != PICO_OK) {
            this->emitErrorMessage(std::format("{}::initialize() setDigitalPorts", this->name), gr::Error(detail::getErrorMessage(status)));
        }

        // apply trigger configuration
        if (detail::isAnalogTrigger(trigger_source) && acquisitionMode == AcquisitionMode::RapidBlock) {
            const auto channelEnum = self().convertToChannel(trigger_source);
            assert(channelEnum != std::nullopt);
            const auto    direction    = self().convertToThresholdDirection(detail::convertToEnum<TriggerDirection>(trigger_direction));
            const int16_t thresholdADC = convertVoltageToADCCount(trigger_threshold);

            const auto status = self().setSimpleTrigger(_handle, true, channelEnum.value(), thresholdADC, direction, 0 /* delay */, 0 /* auto trigger */);
            if (status != PICO_OK) {
                this->emitErrorMessage(std::format("{}::initialize() setSimpleTrigger", this->name), gr::Error(detail::getErrorMessage(status)));
            }
        }
        if (detail::isDigitalTrigger(trigger_source) && acquisitionMode == AcquisitionMode::RapidBlock) {
            const auto status = self().SetTriggerDigitalPort(_handle, _digitalChannelNumber, detail::convertToEnum<TriggerDirection>(trigger_direction));
            if (status != PICO_OK) {
                this->emitErrorMessage(std::format("{}::initialize() SetTriggerDigitalPort", this->name), gr::Error(detail::getErrorMessage(status)));
            }

        } else {
            // Disable any trigger conditions, so captures occur IMMEDIATELY without waiting for any event
            // Note: To prevent any trigger events, one needs to set the threshold for all channels to the maximum value
            // const int16_t thresholdADC = self().maxADCCount() - 255; // 255 seems to be a magic number, most probably it is used for additional meta information
            // const auto status = self().setSimpleTrigger(_handle, true, channelEnum.value(), thresholdADC, direction, 0, 0);
            for (std::size_t i = 0; i < static_cast<std::size_t>(self().maxChannel()); i++) {
                const auto channelEnum = self().convertToChannel(i);
                assert(channelEnum != std::nullopt);
                const auto    direction    = self().convertToThresholdDirection(detail::convertToEnum<TriggerDirection>("Rising"));
                const int16_t thresholdADC = 0;

                const auto status = self().setSimpleTrigger(_handle, false, channelEnum.value(), thresholdADC, direction, 0 /* delay */, 0 /* auto trigger */);
                if (status != PICO_OK) {
                    this->emitErrorMessage(std::format("{}::initialize() setSimpleTrigger", this->name), gr::Error(detail::getErrorMessage(status)));
                }
            }
        }
    }

    void arm() {
        if (!isOpened()) {
            return;
        }

        if constexpr (acquisitionMode == AcquisitionMode::RapidBlock) {
            bool expected = false;
            if (_isArmed.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
                const auto timebaseRes = self().convertSampleRateToTimebase(_handle, sample_rate);
                _actualSampleRate      = timebaseRes.actualFreq;

                static auto    redirector = [](int16_t, PICO_STATUS status, void* vobj) { static_cast<decltype(this)>(vobj)->rapidBlockCallback({status}); };
                int32_t        timeIndisposedMs;
                const uint32_t segmentIndex = 0; // only one segment for streaming mode

                const auto status = self().runBlock(_handle, static_cast<int32_t>(pre_samples), static_cast<int32_t>(post_samples), timebaseRes.timebase, &timeIndisposedMs, segmentIndex, static_cast<TPSImpl::BlockReadyType>(redirector), this);

                if (status != PICO_OK) {
                    this->emitErrorMessage(std::format("{}::arm() RunBlock", this->name), gr::Error(detail::getErrorMessage(status)));
                }
            }
        } else {
            using fair::picoscope::detail::kDriverBufferSize;
            if (const auto ec = self().setBuffers(kDriverBufferSize); ec) {
                this->emitErrorMessage(std::format("{}::arm() setBuffers", this->name), ec.message());
            }

            TimeInterval timeInterval = convertSampleRateToTimeInterval(sample_rate);

            const auto status = self().runStreaming(        //
                _handle,                                    // identifier for the scope device
                &timeInterval.interval,                     // in: desired interval, out: actual interval
                self().convertTimeUnits(timeInterval.unit), // time unit of interval
                0,                                          // pre-trigger-samples (unused)
                static_cast<uint32_t>(kDriverBufferSize),   // post-trigger-samples
                false,                                      // autoStop
                1,                                          // downsampling ratio
                self().ratioNone(),                         // downsampling ratio mode
                static_cast<uint32_t>(kDriverBufferSize));  // the size of the overview buffers

            if (status != PICO_OK) {
                this->emitErrorMessage(std::format("{}::arm() RunStreaming", this->name), gr::Error(detail::getErrorMessage(status)));
            }
            _actualSampleRate = convertTimeIntervalToSampleRate(timeInterval);
        }
    }

    void streamingCallback(int32_t nrSamplesSigned, uint32_t idx, int16_t overflow)
    requires(acquisitionMode == AcquisitionMode::Streaming)
    {
        _streamingSamples = static_cast<std::size_t>(nrSamplesSigned);
        _streamingOffset  = static_cast<std::size_t>(idx);
        // NOTE: according to the programmer's guide the data should be copied-out inside the callback function. Not sure if the driver might overwrite the buffer with new data after the callback has returned
        assert(nrSamplesSigned >= 0);

        // According to well-informed sources, the driver indicates the buffer overrun by setting all the bits of the overflow argument to true.
        if (static_cast<uint16_t>(overflow) == 0xffff) {
            // TODO: send tag
            std::println(std::cerr, "Buffer overrun detected, continue...");
        }
    }

    void rapidBlockCallback(Error ec)
    requires(acquisitionMode == AcquisitionMode::RapidBlock)
    {
        if (ec) {
            if (ec.code == PICO_CANCELLED) {
                _isArmed.store(false, std::memory_order_release);
                return;
            } else {
                this->emitErrorMessage(std::format("{}::rapidBlockCallback", this->name), ec.message());
                _isArmed.store(false, std::memory_order_release);
                return;
            }
        }
        this->invokeWork();
        _isArmed.store(false, std::memory_order_release);
    }

    template<gr::OutputSpanLike TOutSpan>
    gr::work::Status processBulk(gr::InputSpanLike auto& timingInSpan, gr::OutputSpanLike auto& digitalOutSpan, std::span<TOutSpan>& outputs) {
        if constexpr (acquisitionMode == AcquisitionMode::Streaming) {
            if (_streamingSamples == 0) {
                for (auto& output : outputs) {
                    output.publish(0);
                }
                if (const auto ec = self().streamingPoll()) {
                    this->emitErrorMessage(std::format("{}::processBulk", this->name), ec.message());
                    return gr::work::Status::ERROR;
                }
            } else {
                processDriverDataStreaming(_streamingSamples, _streamingOffset, timingInSpan, digitalOutSpan, outputs);
                _streamingSamples = 0;
            }
        } else {
            const auto nSamples = pre_samples + post_samples;

            uint32_t nCompletedCaptures = 1;

            if (const auto status = self().getNoOfCaptures(_handle, &nCompletedCaptures); status != PICO_OK) {
                this->emitErrorMessage(std::format("{}::processBulk getNofCaptures", this->name), detail::getErrorMessage(status));
                return gr::work::Status::ERROR;
            }

            nCompletedCaptures                  = std::min(static_cast<gr::Size_t>(nCompletedCaptures), n_captures.value);
            const std::size_t availableCaptures = calculateAvailableOutputs(nCompletedCaptures, digitalOutSpan, outputs);

            for (std::size_t iCapture = 0; iCapture < availableCaptures; iCapture++) {
                const auto getValuesResult = self().rapidBlockGetValues(iCapture, nSamples);
                if (nSamples != getValuesResult.nSamples) {
                    this->emitErrorMessage(std::format("{}::processBulk", this->name), std::format("Number of retrieved samples ({}) doesn't equal to required samples: pre_samples + post_samples ({})", getValuesResult.nSamples, nSamples));
                }
                if (getValuesResult.error) {
                    this->emitErrorMessage(std::format("{}::processBulk", this->name), getValuesResult.error.message());
                    return gr::work::Status::ERROR;
                }
                processDriverDataRapidBlock(iCapture, getValuesResult.nSamples, timingInSpan, digitalOutSpan, outputs);
            }

            // if RapidBlock OR trigger_source is not set then consume all input tags
            if (acquisitionMode == AcquisitionMode::RapidBlock || self().convertToOutputIndex(trigger_source) == std::nullopt) {
                const std::size_t nInputs = timingInSpan.size();
                timingInSpan.consume(nInputs);
                timingInSpan.consumeTags(nInputs);
            }

            for (std::size_t i = 0; i < outputs.size(); i++) {
                if (availableCaptures < nCompletedCaptures) {
                    outputs[i].publishTag(gr::property_map{{gr::tag::N_DROPPED_SAMPLES.shortKey(), nSamples * (nCompletedCaptures - availableCaptures)}}, 0);
                }
                outputs[i].publish(nCompletedCaptures);
            }
            digitalOutSpan.publish(nCompletedCaptures);
        }

        if (_channels.empty()) {
            return gr::work::Status::OK;
        }

        if (trigger_once) {
            return gr::work::Status::DONE;
        }

        return gr::work::Status::OK;
    }

    template<gr::OutputSpanLike TOutSpan>
    [[nodiscard]] std::size_t calculateAvailableOutputs(std::size_t nSamples, gr::OutputSpanLike auto& digitalOutSpan, std::span<TOutSpan>& outputs) const {
        std::size_t availableOutputs = std::numeric_limits<std::size_t>::max();
        for (std::size_t channelIdx = 0; channelIdx < _channels.size(); ++channelIdx) {
            availableOutputs = std::min(availableOutputs, outputs[channelIdx].size());
        }
        availableOutputs = std::min(availableOutputs, digitalOutSpan.size());

        const std::size_t availableSamples = std::min(availableOutputs, nSamples);
        if (availableSamples < nSamples) {
            std::println(std::cerr, "Picoscope::processDriverDataStreaming: {} samples will be lost due to insufficient space in output buffers (require {}, available {})", nSamples - availableSamples, nSamples, availableOutputs);
        }

        return availableSamples;
    }

    template<gr::OutputSpanLike TOutSpan>
    void processDriverDataStreaming(std::size_t nSamples, std::size_t offset, gr::InputSpanLike auto& timingInSpan, gr::OutputSpanLike auto& digitalOutSpan, std::span<TOutSpan>& outputs)
    requires(acquisitionMode == AcquisitionMode::Streaming)
    {
        const std::size_t availableSamples = calculateAvailableOutputs(nSamples, digitalOutSpan, outputs);

        for (std::size_t channelIdx = 0; channelIdx < _channels.size(); channelIdx++) {
            processSamplesOneChannel<T>(availableSamples, offset, _channels[channelIdx], outputs[channelIdx]);
        }

        std::vector<gr::Tag> triggerTags = processTimingTriggers(availableSamples, timingInSpan, digitalOutSpan, outputs);

        // publish tags
        for (std::size_t channelIdx = 0; channelIdx < _channels.size(); ++channelIdx) {
            auto&      channel     = _channels[channelIdx];
            auto&      output      = outputs[channelIdx];
            const auto tagsToWrite = triggerTags.size() + (channel.signalInfoTagPublished ? 0 : 1);
            if (tagsToWrite == 0) {
                continue;
            }
            if (!channel.signalInfoTagPublished) {
                output.publishTag(channel.toTagMap(), 0);
                channel.signalInfoTagPublished = true;
            }
            if (availableSamples < nSamples) {
                output.publishTag(gr::property_map{{gr::tag::N_DROPPED_SAMPLES.shortKey(), nSamples - availableSamples}}, 0);
            }
            for (auto& tag : triggerTags) {
                output.publishTag(tag.map, tag.index);
            }
        }

        self().copyDigitalBuffersToOutput(digitalOutSpan, availableSamples);
        digitalOutSpan.publish(availableSamples);

        // publish samples
        for (std::size_t i = 0; i < outputs.size(); i++) {
            // TODO: The issue is that the Block class does not properly handle optional ports such that sample limits are calculated wrongly.
            // Therefore, we must connect all output ports of the picoscope and ensure that samples are published for all ports.
            // Otherwise, tags will not be propagated correctly.
            // const std::size_t nSamplesToPublish = i < _state.channels.size() ? availableSamples : 0UZ;
            const std::size_t nSamplesToPublish = availableSamples;
            outputs[i].publish(nSamplesToPublish);
        }

        _nSamplesPublished += availableSamples;
    }

    constexpr T createDataset(detail::Channel& channel, std::size_t nSamples)
    requires(acquisitionMode == AcquisitionMode::RapidBlock)
    {
        T ds{};
        ds.timestamp = 0;

        ds.axis_names = {channel.name};
        ds.axis_units = {channel.unit};

        ds.extents           = {static_cast<int32_t>(nSamples)};
        ds.layout            = gr::LayoutRight{};
        ds.signal_names      = {channel.name};
        ds.signal_units      = {channel.unit};
        ds.signal_quantities = {channel.quantity};

        ds.signal_values.resize(nSamples);
        ds.signal_ranges.resize(1);
        ds.timing_events.resize(1);
        ds.axis_values.resize(1);
        ds.axis_values[0].resize(nSamples);

        // generate time axis
        using TSample            = T::value_type;
        int         i            = 0;
        const auto  pre          = static_cast<int>(pre_samples);
        const float samplePeriod = 1.0f / sample_rate;
        std::ranges::generate(ds.axis_values[0], [&i, pre, samplePeriod]() {
            if constexpr (std::is_same_v<TSample, float> || std::is_same_v<TSample, gr::UncertainValue<float>>) {
                float t = static_cast<float>(i - pre) * samplePeriod;
                ++i;
                return TSample{t};
            } else if constexpr (std::is_same_v<TSample, std::int16_t>) {
                return static_cast<std::int16_t>(i++);
            }
        });

        ds.meta_information.resize(1);
        ds.meta_information[0] = channel.toTagMap();

        return ds;
    }

    constexpr TDigitalOutput createDatasetDigital(std::size_t nSamples)
    requires(acquisitionMode == AcquisitionMode::RapidBlock)
    {
        TDigitalOutput ds{};
        ds.extents = {1, static_cast<int32_t>(nSamples)};
        ds.layout  = gr::LayoutRight{};

        ds.signal_values.resize(nSamples);
        ds.signal_ranges.resize(1);
        ds.timing_events.resize(1);

        return ds;
    }

    template<gr::OutputSpanLike TOutSpan>
    void processDriverDataRapidBlock(std::size_t iCapture, std::size_t nSamples, gr::InputSpanLike auto& timingInSpan, gr::OutputSpanLike auto& digitalOutSpan, std::span<TOutSpan>& outputs)
    requires(acquisitionMode == AcquisitionMode::RapidBlock)
    {
        using TSample = T::value_type;
        for (std::size_t channelIdx = 0; channelIdx < _channels.size(); channelIdx++) {
            outputs[channelIdx][iCapture] = createDataset(_channels[channelIdx], nSamples);
            processSamplesOneChannel<TSample>(nSamples, 0, _channels[channelIdx], outputs[channelIdx][iCapture].signal_values);
            if constexpr (std::is_same_v<TSample, float> || std::is_same_v<TSample, std::int16_t>) {
                gr::dataset::updateMinMax<TSample>(outputs[channelIdx][iCapture]);
            } else {
                // TODO: fix UncertainValue, it requires changes in GR4
            }
        }

        // TODO: currently we assume that we have only trigger tags, and each trigger tag corresponds to iCapture
        for (std::size_t channelIdx = 0; channelIdx < _channels.size(); ++channelIdx) {
            if (iCapture < timingInSpan.rawTags.size()) {
                const auto&       tag      = timingInSpan.rawTags[iCapture];
                const std::size_t tagIndex = static_cast<std::size_t>(pre_samples);
                outputs[channelIdx][iCapture].timing_events[0].emplace_back(tagIndex, tag.map);
            }
        }

        digitalOutSpan[iCapture] = createDatasetDigital(nSamples);
        self().copyDigitalBuffersToOutput(digitalOutSpan[iCapture].signal_values, nSamples);

        _nSamplesPublished++;
    }

    template<typename TSample>
    void processSamplesOneChannel(std::size_t availableSamples, std::size_t offset, detail::Channel& channel, std::span<TSample> output) {

        const auto driverData = std::span(channel.driverBuffer).subspan(offset, availableSamples);

        if constexpr (std::is_same_v<TSample, int16_t>) {
            std::ranges::copy(driverData, output.begin());
        } else {
            const float voltageMultiplier = channel.range / static_cast<float>(_maxValue);
            // TODO use SIMD
            for (std::size_t i = 0; i < availableSamples; ++i) {
                if constexpr (std::is_same_v<TSample, float>) {
                    output[i] = channel.offset + channel.scale * voltageMultiplier * static_cast<float>(driverData[i]);
                } else if constexpr (std::is_same_v<TSample, gr::UncertainValue<float>>) {
                    output[i] = gr::UncertainValue(channel.offset + channel.scale * voltageMultiplier * static_cast<float>(driverData[i]), self().uncertainty());
                }
            }
        }
    }

    template<gr::OutputSpanLike TOutSpan>
    std::vector<gr::Tag> processTimingTriggers(std::size_t availableSamples, gr::InputSpanLike auto& timingInSpan, gr::OutputSpanLike auto& digitalOutSpan, std::span<TOutSpan>& outputs) {
        // Note: reference for `gr::InputSpanLike auto& timingInSpan` is needed for proper work of consumeTags method
        std::vector<std::size_t> triggerOffsets;
        if (detail::isAnalogTrigger(trigger_source)) {
            const auto triggerSourceIndex = self().convertToOutputIndex(trigger_source);
            if (triggerSourceIndex == std::nullopt) {
                return std::vector<gr::Tag>{};
            }
            std::span samples(outputs[triggerSourceIndex.value()].data(), availableSamples);
            triggerOffsets = findAnalogTriggers(_channels[triggerSourceIndex.value()], samples);
        } else if (detail::isDigitalTrigger(trigger_source)) {
            if (_digitalChannelNumber < 0 || _digitalChannelNumber > 15) {
                return std::vector<gr::Tag>{};
            }
            triggerOffsets = findDigitalTriggers(_digitalChannelNumber, digitalOutSpan);
        }

        const auto nowStamp = std::chrono::high_resolution_clock::now();
        const auto now      = std::chrono::duration_cast<std::chrono::nanoseconds>(nowStamp.time_since_epoch());

        std::size_t consumeTags = 0UZ;
        for (const auto& tag : timingInSpan.rawTags) {
            const bool isArmDisarmTrigger = detail::tagContainsTrigger(tag, _armTriggerNameAndCtx) || detail::tagContainsTrigger(tag, _disarmTriggerNameAndCtx);
            if (!tag.map.contains(gr::tag::CONTEXT.shortKey()) || isArmDisarmTrigger) { // only consider timing tags
                consumeTags++;
                continue;
            }
            // TODO: Allow to match multiple events to a single trigger here and allow to timeout on events where the trigger pulse was somehow lost
            if (triggerOffsets.size() <= _timingMessages.size()) { // all triggers have already been found
                break;
            }
            _timingMessages.emplace(tag.map);
            consumeTags++;
        }
        timingInSpan.consumeTags(consumeTags);
        timingInSpan.consume(consumeTags);
        std::vector<gr::Tag> triggerTags;
        triggerTags.reserve(triggerOffsets.size() + 1);
        for (const auto triggerOffset : triggerOffsets) {
            gr::property_map timing;
            if (_timingMessages.empty()) {
                timing = gr::property_map{{gr::tag::TRIGGER_NAME.shortKey(), "UnknownTrigger"}, {gr::tag::TRIGGER_TIME.shortKey(), static_cast<std::uint64_t>(now.count())}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}};
                // TODO: stop processing here instead in case the tag arrives later and only publish unknown trigger tag after timeout
            } else {
                timing = _timingMessages.front();
                _timingMessages.pop();
            }
            triggerTags.emplace_back(static_cast<int64_t>(triggerOffset), timing);
            _nextSystemtime = nowStamp + std::chrono::milliseconds(systemtime_interval);
        }
        // add an independent software timestamp with the localtime of the system to the last sample of each chunk
        if (nowStamp > _nextSystemtime) {
            triggerTags.emplace_back(static_cast<int64_t>(availableSamples), gr::property_map{{gr::tag::TRIGGER_NAME.shortKey(), "systemtime"}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {gr::tag::CONTEXT.shortKey(), "NO-CONTEXT"}, {gr::tag::TRIGGER_TIME.shortKey(), static_cast<uint64_t>(now.count())}});
            _nextSystemtime += std::chrono::milliseconds(systemtime_interval);
        }
        return triggerTags;
    }

    template<typename TSample>
    std::vector<std::size_t> findAnalogTriggers(const detail::Channel& triggerChannel, std::span<TSample> samples) {
        if (samples.empty()) {
            return {};
        }

        std::vector<std::size_t> triggerOffsets; // relative offset of detected triggers
        const float              band              = triggerChannel.range / 100.f;
        const float              voltageMultiplier = triggerChannel.range / static_cast<float>(_maxValue);

        const auto toFloat = [&voltageMultiplier](TSample raw) {
            if constexpr (std::is_same_v<TSample, float>) {
                return raw;
            } else if constexpr (std::is_same_v<TSample, int16_t>) {
                return static_cast<float>(raw);
            } else if constexpr (std::is_same_v<TSample, gr::UncertainValue<float>>) {
                return raw.value;
            } else {
                static_assert(gr::meta::always_false<TSample>, "This type is not supported.");
            }
        };

        const auto triggerDirectionEnum = detail::convertToEnum<TriggerDirection>(trigger_direction);
        if (triggerDirectionEnum == TriggerDirection::Rising || triggerDirectionEnum == TriggerDirection::High) {
            for (std::size_t i = 0; i < samples.size(); i++) {
                const float value = toFloat(samples[i]);
                if (_triggerState == 0 && value >= trigger_threshold) {
                    _triggerState = 1;
                    triggerOffsets.push_back(i);
                } else if (_triggerState == 1 && value <= trigger_threshold - band) {
                    _triggerState = 0;
                }
            }
        } else if (triggerDirectionEnum == TriggerDirection::Falling || triggerDirectionEnum == TriggerDirection::Low) {
            for (std::size_t i = 0; i < samples.size(); i++) {
                const float value = toFloat(samples[i]);
                if (_triggerState == 1 && value <= trigger_threshold) {
                    _triggerState = 0;
                    triggerOffsets.push_back(i);
                } else if (_triggerState == 0 && value >= trigger_threshold + band) {
                    _triggerState = 1;
                }
            }
        }
        return triggerOffsets;
    }

    std::vector<std::size_t> findDigitalTriggers(int digitalChannelNumber, std::span<std::uint16_t> samples) {
        if (samples.empty()) {
            return {};
        }
        const auto mask = static_cast<uint16_t>(1U << digitalChannelNumber);

        std::vector<std::size_t> triggerOffsets;

        const auto triggerDirectionEnum = detail::convertToEnum<TriggerDirection>(trigger_direction);
        if (triggerDirectionEnum == TriggerDirection::Rising || triggerDirectionEnum == TriggerDirection::High) {
            for (std::size_t i = 0; i < samples.size(); i++) {
                if (!_triggerState && (samples[i] & mask)) {
                    _triggerState = 1;
                    triggerOffsets.push_back(i);
                } else if (_triggerState && !(samples[i] & mask)) {
                    _triggerState = 0;
                }
            }
        } else if (triggerDirectionEnum == TriggerDirection::Falling || triggerDirectionEnum == TriggerDirection::Low) {
            for (std::size_t i = 0; i < samples.size(); i++) {
                if (_triggerState && !(samples[i] & mask)) {
                    _triggerState = 0;
                    triggerOffsets.push_back(i);
                } else if (!_triggerState && (samples[i] & mask)) {
                    _triggerState = 1;
                }
            }
        }

        return triggerOffsets;
    }

    [[nodiscard]] constexpr int16_t convertVoltageToADCCount(float value) {
        value = std::clamp(value, self().extTriggerMinValueVoltage(), self().extTriggerMaxValueVoltage());
        return static_cast<int16_t>((value / self().extTriggerMaxValueVoltage()) * static_cast<float>(self().extTriggerMaxValue()));
    }

    [[nodiscard]] Error streamingPoll() {
        static auto redirector = [](int16_t /*handle*/, TPSImpl::NSamplesType noOfSamples, uint32_t startIndex, int16_t overflow, uint32_t /*triggerAt*/, int16_t /*triggered*/, int16_t /*autoStop*/, void* vobj) { static_cast<decltype(this)>(vobj)->streamingCallback(noOfSamples, startIndex, overflow); };

        const auto status = self().getStreamingLatestValues(_handle, static_cast<TPSImpl::StreamingReadyType>(redirector), this);
        if (status == PICO_BUSY || status == PICO_DRIVER_FUNCTION) {
            return {};
        }
        return {status};
    }

    [[nodiscard]] fair::picoscope::GetValuesResult rapidBlockGetValues(std::size_t iCapture, std::size_t nSamples) {
        if (const auto ec = self().setBuffers(nSamples, static_cast<uint32_t>(iCapture)); ec) {
            return {ec, 0, 0};
        }

        const uint32_t offset     = 0;
        auto           nSamples32 = static_cast<uint32_t>(nSamples);
        int16_t        overflow   = 0;
        const auto     status     = self().getValues(_handle, offset, &nSamples32, 1, self().ratioNone(), static_cast<uint32_t>(iCapture), &overflow);

        if (status != PICO_OK) {
            std::println(std::cerr, "GetValues: {}", detail::getErrorMessage(status));
            return {{status}, 0, 0};
        }
        return {{}, static_cast<std::size_t>(nSamples32), overflow};
    }

    [[nodiscard]] Error setBuffers(size_t nSamples, uint32_t segmentIndex = 0UZ) {
        for (auto& channel : _channels) {
            const auto channelEnum = self().convertToChannel(channel.id);
            assert(channelEnum);

            channel.driverBuffer.resize(std::max(nSamples, channel.driverBuffer.size()));
            if constexpr (acquisitionMode == AcquisitionMode::Streaming) {
                const auto status = self().setDataBuffer(_handle, *channelEnum, channel.driverBuffer.data(), static_cast<int32_t>(nSamples), self().ratioNone());
                if (status != PICO_OK) {
                    std::println(std::cerr, "setDataBufferStreaming (chan {}): {}", static_cast<std::size_t>(*channelEnum), detail::getErrorMessage(status));
                    return {status};
                }
            } else {
                const auto status = self().setDataBufferForSegment(_handle, *channelEnum, channel.driverBuffer.data(), static_cast<int32_t>(nSamples), segmentIndex, self().ratioNone());
                if (status != PICO_OK) {
                    std::println(std::cerr, "setDataBufferRapidBlock (chan {}): {}", static_cast<std::size_t>(*channelEnum), detail::getErrorMessage(status));
                    return {status};
                }
            }
        }

        if (const auto status = self().setDigitalBuffers(nSamples, segmentIndex); status != PICO_OK) {
            std::println(std::cerr, "setDigitalBuffers: {}", detail::getErrorMessage(status));
            return {status};
        }

        return {};
    }

    [[nodiscard]] std::string getUnitInfoTopic(int16_t handle, PICO_INFO info) const {
        std::array<int8_t, 40> line{};
        int16_t                requiredSize;

        auto status = self().getUnitInfo(handle, line.data(), line.size(), &requiredSize, info);
        if (status == PICO_OK) {
            return {reinterpret_cast<char*>(line.data()), static_cast<std::size_t>(requiredSize)};
        }

        return {};
    }

    [[nodiscard]] std::string driverVersion() const {
        const std::string prefix  = "Picoscope Linux Driver, ";
        auto              version = getUnitInfoTopic(_handle, PICO_DRIVER_VERSION);

        if (auto i = version.find(prefix); i != std::string::npos) {
            version.erase(i, prefix.length());
        }
        return version;
    }

    [[nodiscard]] std::string hardwareVersion() const { return getUnitInfoTopic(_handle, PICO_HARDWARE_VERSION); }

    [[nodiscard]] std::string serialNumber() const { return getUnitInfoTopic(_handle, PICO_BATCH_AND_SERIAL); }

    [[nodiscard]] std::string deviceVariant() const { return getUnitInfoTopic(_handle, PICO_VARIANT_INFO); }

    [[nodiscard]] constexpr auto& self() noexcept { return *static_cast<TPSImpl*>(this); }

    [[nodiscard]] constexpr const auto& self() const noexcept { return *static_cast<const TPSImpl*>(this); }
};

} // namespace fair::picoscope

#endif
