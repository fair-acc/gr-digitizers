#ifndef FAIR_PICOSCOPE_PICOSCOPE_HPP
#define FAIR_PICOSCOPE_PICOSCOPE_HPP

#include "StatusMessages.hpp"

#include <PicoConnectProbes.h>
#include <gnuradio-4.0/Block.hpp>

#include <fmt/format.h>

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
    DC_1M,  ///< DC, 1 MOhm
    AC_1M,  ///< AC, 1 MOhm
    DC_50R, ///< DC, 50 Ohm
};

enum class TriggerDirection { Rising, Falling, Low, High };

enum class TimeUnits { FS, PS, NS, US, MS, S };

struct GetValuesResult {
    Error       error;
    std::size_t samples;
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
        unit   = TimeUnits::PS;
        factor = 1e12;
    } else if (intervalSec < 0.001) {
        unit   = TimeUnits::NS;
        factor = 1e9;
    } else if (intervalSec < 0.1) {
        unit   = TimeUnits::US;
        factor = 1e6;
    } else {
        unit   = TimeUnits::MS;
        factor = 1e3;
    }
    return {unit, static_cast<std::uint32_t>(factor / static_cast<double>(sampleRate))};
}

[[nodiscard]] inline constexpr float convertTimeIntervalToSampleRate(TimeInterval timeInterval) {
    double factor;
    switch (timeInterval.unit) {
    case TimeUnits::FS: factor = 1e15; break;
    case TimeUnits::PS: factor = 1e12; break;
    case TimeUnits::NS: factor = 1e9; break;
    case TimeUnits::US: factor = 1e6; break;
    case TimeUnits::MS: factor = 1e3; break;
    default: factor = 1.; break;
    }
    return timeInterval.interval == 0 ? 1.f : static_cast<float>(factor / static_cast<double>(timeInterval.interval));
}

namespace detail {

constexpr std::size_t kDriverBufferSize = 65536;

struct ChannelSetting {
    std::string name;
    std::string unit         = "V";
    std::string quantity     = "Voltage";
    float       range        = 2.f;
    float       analogOffset = 0.f;
    float       scale        = 1.f;
    float       offset       = 0.f;
    Coupling    coupling     = Coupling::AC_1M;
};

using ChannelMap = std::map<std::string, ChannelSetting, std::less<>>;

struct TriggerSetting {
    static constexpr std::string_view kTriggerDigitalSource = "DI"; // DI is as well-used as "AUX" for p6000 scopes

    bool isEnabled() const { return !source.empty(); }

    bool isDigital() const { return isEnabled() && source == kTriggerDigitalSource; }

    bool isAnalog() const { return isEnabled() && source != kTriggerDigitalSource; }

    std::string      source;
    float            threshold  = 0; // AI only
    TriggerDirection direction  = TriggerDirection::Rising;
    int              pin_number = 0; // DI only
};

template<typename T>
struct Channel {
    std::string          id;
    ChannelSetting       settings;
    std::vector<int16_t> driver_buffer;
    bool                 signal_info_written = false;

    gr::property_map toTagMap() const {
        using namespace gr;
        static const auto kSignalName     = std::string(tag::SIGNAL_NAME.shortKey());
        static const auto kSignalQuantity = std::string(tag::SIGNAL_QUANTITY.shortKey());
        static const auto kSignalUnit     = std::string(tag::SIGNAL_UNIT.shortKey());
        static const auto kSignalMin      = std::string(tag::SIGNAL_MIN.shortKey());
        static const auto kSignalMax      = std::string(tag::SIGNAL_MAX.shortKey());
        return {{kSignalName, settings.name}, {kSignalQuantity, settings.quantity}, {kSignalUnit, settings.unit}, {kSignalMin, settings.offset}, {kSignalMax, settings.offset + settings.range}};
    }
};

struct Settings {
    std::string     serial_number;
    float           sample_rate              = 10000.f;
    AcquisitionMode acquisition_mode         = AcquisitionMode::Streaming;
    std::size_t     pre_samples              = 1000;
    std::size_t     post_samples             = 9000;
    std::size_t     rapid_block_nr_captures  = 1;
    bool            trigger_once             = false;
    float           streaming_mode_poll_rate = 0.001;
    bool            auto_arm                 = true;
    ChannelMap      enabled_channels;
    TriggerSetting  trigger;
};

template<typename T>
struct State {
    std::vector<Channel<T>> channels;
    std::atomic<bool>       data_finished      = false;
    bool                    initialized        = false;
    bool                    configured         = false;
    bool                    closed             = false;
    bool                    armed              = false;
    bool                    started            = false; // TODO transitional until nodes have state handling
    int8_t                  trigger_state      = 0;
    int16_t                 handle             = -1; ///< picoscope handle
    int16_t                 overflow           = 0;  ///< status returned from getValues
    int16_t                 max_value          = 0;  ///< maximum ADC count used for ADC conversion
    float                   actual_sample_rate = 0;
    std::size_t             produced_worker    = 0; // poller/callback thread
};

inline AcquisitionMode parseAcquisitionMode(std::string_view s) {
    using enum AcquisitionMode;
    if (s == "RapidBlock") {
        return RapidBlock;
    }
    if (s == "Streaming") {
        return Streaming;
    }
    throw std::invalid_argument(fmt::format("Unknown acquisition mode '{}'", s));
}

inline Coupling parseCoupling(std::string_view s) {
    using enum Coupling;
    if (s == "DC_1M") {
        return DC_1M;
    }
    if (s == "AC_1M") {
        return AC_1M;
    }
    if (s == "DC_50R") {
        return DC_50R;
    }
    throw std::invalid_argument(fmt::format("Unknown coupling type '{}'", s));
}

inline TriggerDirection parseTriggerDirection(std::string_view s) {
    using enum TriggerDirection;
    if (s == "Rising") {
        return Rising;
    }
    if (s == "Falling") {
        return Falling;
    }
    if (s == "Low") {
        return Low;
    }
    if (s == "High") {
        return High;
    }
    throw std::invalid_argument(fmt::format("Unknown trigger direction '{}'", s));
}

inline ChannelMap channelSettings(std::span<std::string> ids, std::span<std::string> names, std::span<std::string> units, std::span<std::string> quantities, std::span<float> ranges, std::span<float> analogOffsets, std::span<float> scales, std::span<float> offsets, std::span<std::string> couplings) {
    ChannelMap r;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        ChannelSetting channel;
        if (i < names.size()) {
            channel.name = names[i];
        } else {
            channel.name = fmt::format("signal {} ({})", i, ids[i]);
        }
        if (i < units.size()) {
            channel.unit = std::string(units[i]);
        }
        if (i < quantities.size()) {
            channel.quantity = std::string(quantities[i]);
        }
        if (i < ranges.size()) {
            channel.range = ranges[i];
        }
        if (i < analogOffsets.size()) {
            channel.analogOffset = analogOffsets[i];
        }
        if (i < scales.size()) {
            channel.scale = scales[i];
        }
        if (i < offsets.size()) {
            channel.offset = offsets[i];
        }
        if (i < couplings.size()) {
            channel.coupling = parseCoupling(couplings[i]);
        }

        r[std::string(ids[i])] = channel;
    }
    return r;
}

} // namespace detail

// optional shortening
template<typename T, gr::meta::fixed_string description = "", typename... Arguments>
using A = gr::Annotated<T, description, Arguments...>;

using gr::Visible;

/**
 * We only allow a small set of types to be used as the output type for the Picoscope Block:
 *
 * - std::int16_t
 *   This outputs the raw values as-is without any scaling.
 *
 * - float
 *   The physical gain-scaled output of the measured values
 *
 * - gr::UncertainValue<float>
 *   Same as "float", except it also contains the estimated measurement error as an additional component.
 **/
template<typename T>
concept PicoscopeOutput = std::disjunction_v<std::is_same<T, std::int16_t>, std::is_same<T, float>, std::is_same<T, gr::UncertainValue<float>>>;

// helper struct to conditionally enable BlockingIO at compile time
template<typename TPSImpl, bool>
struct PicoscopeBlockingHelper;

template<typename TPSImpl>
struct PicoscopeBlockingHelper<TPSImpl, false> {
    using type = gr::Block<TPSImpl, gr::SupportedTypes<int16_t, float, gr::UncertainValue<float>>>;
};

template<typename TPSImpl>
struct PicoscopeBlockingHelper<TPSImpl, true> {
    using type = gr::Block<TPSImpl, gr::SupportedTypes<int16_t, float, gr::UncertainValue<float>>, gr::BlockingIO<false>>;
};

template<PicoscopeOutput T, AcquisitionMode acquisitionMode, typename TPSImpl>
class Picoscope : public PicoscopeBlockingHelper<TPSImpl, acquisitionMode == AcquisitionMode::RapidBlock>::type {
public:
    using super_t = typename PicoscopeBlockingHelper<TPSImpl, acquisitionMode == AcquisitionMode::RapidBlock>::type;

    Picoscope(gr::property_map props) : super_t(std::move(props)) {}

    A<std::string, "serial number">  serial_number;
    A<float, "sample rate", Visible> sample_rate = 10000.f;
    // TODO any way to get custom enums into pmtv??
    A<std::string, "acquisition mode", Visible>                        acquisition_mode         = std::string("Streaming");
    A<gr::Size_t, "pre-samples">                                       pre_samples              = 1000;
    A<gr::Size_t, "post-samples">                                      post_samples             = 9000;
    A<gr::Size_t, "no. captures (rapid block mode)">                   rapid_block_nr_captures  = 1;
    A<bool, "trigger once (rapid block mode)">                         trigger_once             = false;
    A<float, "poll rate (streaming mode)">                             streaming_mode_poll_rate = 0.001;
    A<bool, "auto-arm">                                                auto_arm                 = true;
    A<std::vector<std::string>, "IDs of enabled channels">             channel_ids;
    A<std::vector<float>, "Voltage range of enabled channels">         channel_ranges;         // PS channel setting
    A<std::vector<float>, "Voltage offset of enabled channels">        channel_analog_offsets; // PS channel setting
    A<std::vector<std::string>, "Coupling modes of enabled channels">  channel_couplings;
    A<std::vector<std::string>, "Signal names of enabled channels">    signal_names;
    A<std::vector<std::string>, "Signal units of enabled channels">    signal_units;
    A<std::vector<std::string>, "Signal quantity of enabled channels"> signal_quantities;
    A<std::vector<float>, "Signal scales of the enabled channels">     signal_scales;  // only for floats
    A<std::vector<float>, "Signal offset of the enabled channels">     signal_offsets; // only for floats
    A<std::string, "trigger channel/port ID">                          trigger_source;
    A<float, "trigger threshold, analog only">                         trigger_threshold = 0.f;
    A<std::string, "trigger direction">                                trigger_direction = std::string("Rising");
    A<int, "trigger pin, digital only">                                trigger_pin       = 0;

    A<std::size_t, "time without any tags that triggers a systemtime event"> systemtime_interval = 1000;

    detail::State<T> ps_state;
    detail::Settings ps_settings;

    gr::PortIn<std::uint8_t, gr::Async> timingIn;

    GR_MAKE_REFLECTABLE(Picoscope, timingIn, serial_number, sample_rate, pre_samples, post_samples, acquisition_mode, rapid_block_nr_captures, streaming_mode_poll_rate,              //
        auto_arm, trigger_once, channel_ids, signal_names, signal_units, signal_quantities, channel_ranges, channel_analog_offsets, signal_scales, signal_offsets, channel_couplings, //
        trigger_source, trigger_threshold, trigger_direction, trigger_pin);

private:
    std::mutex                   g_init_mutex;
    std::atomic<std::size_t>     streamingSamples = 0UZ;
    std::atomic<std::size_t>     streamingOffset  = 0UZ;
    std::queue<gr::property_map> timingMessages;

    std::chrono::high_resolution_clock::time_point nextSystemtime = std::chrono::high_resolution_clock::now();

public:
    ~Picoscope() { stop(); }

    void settingsChanged(const gr::property_map& /*old_settings*/, const gr::property_map& /*new_settings*/) {
        const auto wasStarted = ps_state.started;
        if (wasStarted) {
            stop();
        }
        try {
            auto s = detail::Settings{.serial_number = serial_number, //
                .sample_rate                         = sample_rate,
                .acquisition_mode                    = detail::parseAcquisitionMode(acquisition_mode),
                .pre_samples                         = pre_samples,
                .post_samples                        = post_samples,
                .rapid_block_nr_captures             = rapid_block_nr_captures,
                .trigger_once                        = trigger_once,
                .streaming_mode_poll_rate            = streaming_mode_poll_rate,
                .auto_arm                            = auto_arm,
                .enabled_channels                    = detail::channelSettings(channel_ids.value, signal_names.value, signal_units.value, signal_quantities.value, channel_ranges.value, channel_analog_offsets.value, signal_scales.value, signal_offsets.value, channel_couplings.value),
                .trigger                             = detail::TriggerSetting{.source = trigger_source, .threshold = trigger_threshold, .direction = detail::parseTriggerDirection(trigger_direction), .pin_number = trigger_pin}};
            std::swap(ps_settings, s);

            ps_state.channels.clear();
            ps_state.channels.reserve(ps_settings.enabled_channels.size());
            for (const auto& [id, settings] : ps_settings.enabled_channels) {
                ps_state.channels.emplace_back(detail::Channel<T>{.id = id, .settings = settings, .driver_buffer = std::vector<int16_t>(detail::kDriverBufferSize)});
            }
        } catch (const std::exception& e) {
            // TODO add errors properly
            fmt::println(std::cerr, "Could not apply settings: {}", e.what());
        }
        if (wasStarted) {
            start();
        }
    }

    // TODO only for debugging, maybe remove
    std::size_t producedWorker() const { return ps_state.produced_worker; }

    void start() noexcept {
        if (ps_state.started) {
            return;
        }

        try {
            initialize();
            configure();
            if (ps_settings.auto_arm) {
                arm();
            }
            ps_state.started = true;
            fmt::println("Picoscope serial number: {}", serial_number);
        } catch (const std::exception& e) {
            fmt::println(std::cerr, "{}", e.what());
        }
    }

    void stop() noexcept {
        if (!ps_state.started) {
            return;
        }

        ps_state.started = false;

        if (!ps_state.initialized) {
            return;
        }

        disarm();
        close();
    }

    std::string driverVersion() const { return self().driver_driverVersion(); }

    std::string hardwareVersion() const { return self().driver_hardwareVersion(); }

    std::string serialNumber() const { return self().driver_serialNumber(); }

    void initialize() {
        if (ps_state.initialized) {
            return;
        }

        if (const auto ec = self().driver_initialize()) {
            throw std::runtime_error(fmt::format("Initialization failed: {}", ec.message()));
        }

        ps_state.initialized = true;

        serial_number = serialNumber();
    }

    void close() {
        self().driver_close();
        ps_state.closed      = true;
        ps_state.initialized = false;
        ps_state.configured  = false;
    }

    void configure() {
        if (!ps_state.initialized) {
            throw std::runtime_error("Cannot configure without initialization");
        }

        if (ps_state.armed) {
            throw std::runtime_error("Cannot configure armed device");
        }

        if (const auto ec = self().driver_configure()) {
            throw std::runtime_error(fmt::format("Configuration failed: {}", ec.message()));
        }

        ps_state.configured = true;
    }

    void arm() {
        if (ps_state.armed) {
            return;
        }

        if (const auto ec = self().driver_arm()) {
            throw std::runtime_error(fmt::format("Arming failed: {}", ec.message()));
        }

        ps_state.armed = true;
    }

    void disarm() noexcept {
        if (!ps_state.armed) {
            return;
        }

        if (const auto ec = self().driver_disarm()) {
            fmt::println(std::cerr, "disarm failed: {}", ec.message());
        }

        ps_state.armed = false;
    }

    template<gr::OutputSpanLike TOutSpan>
    void processDriverData(std::size_t nrSamples, std::size_t offset, std::span<TOutSpan>& outputs, gr::InputSpanLike auto& timingInSpan) {
        std::vector<std::size_t> triggerOffsets;
        std::size_t              availableOutputs = std::numeric_limits<std::size_t>::max();
        for (std::size_t channelIdx = 0; channelIdx < ps_state.channels.size(); ++channelIdx) {
            availableOutputs = std::min(availableOutputs, outputs[channelIdx].size());
        }

        bool              publishNotEnoughSpaceInBuffer = false;
        const std::size_t availableSamples              = std::min(availableOutputs, nrSamples);
        if (availableSamples < nrSamples) {
            publishNotEnoughSpaceInBuffer = true;
            fmt::println(std::cerr, "Picoscope::processDriverData: {} samples will be lost due to insufficient space in output buffers (require {}, available {})", nrSamples - availableSamples, nrSamples, availableOutputs);
        }

        for (std::size_t channelIdx = 0; channelIdx < ps_state.channels.size(); ++channelIdx) {
            auto& channel = ps_state.channels[channelIdx];
            auto& output  = outputs[channelIdx];

            const auto driverData = std::span(channel.driver_buffer).subspan(offset, availableSamples);

            if constexpr (std::is_same_v<T, int16_t>) {
                std::copy(driverData.begin(), driverData.end(), output.begin());
            } else {
                const float voltageMultiplier = channel.settings.range / static_cast<float>(ps_state.max_value);
                // TODO use SIMD
                for (std::size_t i = 0; i < availableSamples; ++i) {
                    if constexpr (std::is_same_v<T, float>) {
                        output[i] = channel.settings.offset + channel.settings.scale * voltageMultiplier * driverData[i];
                    } else if constexpr (std::is_same_v<T, gr::UncertainValue<float>>) {
                        output[i] = gr::UncertainValue(channel.settings.offset + channel.settings.scale * voltageMultiplier * driverData[i], self().uncertainty());
                    }
                }
            }

            if (channel.id == ps_settings.trigger.source) {
                triggerOffsets = findAnalogTriggers(channel, std::span(output).subspan(0, availableSamples));
            }
        }

        const auto nowStamp = std::chrono::high_resolution_clock::now();
        const auto now      = std::chrono::duration_cast<std::chrono::nanoseconds>(nowStamp.time_since_epoch());

        std::size_t consumeTags = 0UZ;
        for (const auto& tag : timingInSpan.tags()) {
            if (!tag.second.contains(gr::tag::CONTEXT.shortKey())) { // only consider timing tags
                consumeTags++;
                continue;
            }
            // TODO: Allow to match multiple events to a single trigger here and allow to timeout on events where the trigger pulse was somehow lost
            if (triggerOffsets.size() <= timingMessages.size()) { // all triggers have already been found
                break;
            }
            timingMessages.emplace(tag.second);
            consumeTags++;
        }
        timingInSpan.consumeTags(consumeTags);
        timingInSpan.consume(consumeTags);

        std::vector<gr::Tag> triggerTags;
        triggerTags.reserve(triggerOffsets.size() + 1);
        for (const auto triggerOffset : triggerOffsets) {
            gr::property_map timing;
            if (timingMessages.empty()) {
                timing = gr::property_map{{gr::tag::TRIGGER_NAME.shortKey(), "UnknownTrigger"},   //
                                                                                                  //{gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f},        // fall back to ad-hoc timing message
                    {gr::tag::TRIGGER_TIME.shortKey(), static_cast<std::uint64_t>(now.count())}}; //
                // TODO: stop processing here instead in case the tag arrives later and only publish unknown trigger tag after timeout
            } else {
                // use timing message that we received over the message port if any
                timing = timingMessages.front();
                timingMessages.pop();
            }
            triggerTags.emplace_back(static_cast<int64_t>(triggerOffset), timing);
            nextSystemtime = nowStamp + std::chrono::milliseconds(systemtime_interval);
        }
        // add an independent software timestamp with the localtime of the system to the last sample of each chunk
        if (nowStamp > nextSystemtime) {
            triggerTags.emplace_back(static_cast<int64_t>(availableSamples), gr::property_map{{gr::tag::TRIGGER_NAME.shortKey(), "systemtime"}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {gr::tag::CONTEXT.shortKey(), "NO-CONTEXT"}, {gr::tag::TRIGGER_TIME.shortKey(), static_cast<uint64_t>(now.count())}});
            nextSystemtime += std::chrono::milliseconds(systemtime_interval);
        }

        for (std::size_t channelIdx = 0; channelIdx < ps_state.channels.size(); ++channelIdx) {
            auto&      channel         = ps_state.channels[channelIdx];
            auto&      output          = outputs[channelIdx];
            const auto writeSignalInfo = !channel.signal_info_written;
            const auto tagsToWrite     = triggerTags.size() + (writeSignalInfo ? 1 : 0);
            if (tagsToWrite == 0) {
                continue;
            }
            if (writeSignalInfo) {
                auto              map         = channel.toTagMap();
                static const auto kSampleRate = std::string(gr::tag::SAMPLE_RATE.shortKey());
                map[kSampleRate]              = pmtv::pmt(static_cast<float>(sample_rate));
                output.publishTag(map, 0);
                channel.signal_info_written = true;
            }
            if (publishNotEnoughSpaceInBuffer) {
                gr::property_map map = gr::property_map{{"pico_not_enough_space_in_buffer", nrSamples - availableSamples}};
                output.publishTag(map, 0);
            }
            for (auto& tag : triggerTags) {
                output.publishTag(tag.map, tag.index);
            }
        }

        // once all tags have been written, publish the data
        for (std::size_t i = 0; i < outputs.size(); i++) {
            // TODO: The issue is that the Block class does not properly handle optional ports such that sample limits are calculated wrongly.
            // Therefore, we must connect all output ports of the picoscope and ensure that samples are published for all ports.
            // Otherwise, tags will not be propagated correctly.
            // const std::size_t nSamplesToPublish = i < ps_state.channels.size() ? availableSamples : 0UZ;
            const std::size_t nSamplesToPublish = availableSamples;
            outputs[i].publish(nSamplesToPublish);
        }

        ps_state.produced_worker += availableSamples;
    }

    void streamingCallback(int32_t nrSamplesSigned, uint32_t idx, int16_t overflow)
    requires(acquisitionMode == AcquisitionMode::Streaming)
    {
        streamingSamples = static_cast<std::size_t>(nrSamplesSigned);
        streamingOffset  = static_cast<std::size_t>(idx);
        // NOTE: according to the programmer's guide the data should be copied-out inside the callback function. Not sure if the driver might overwrite the buffer with new data after the callback has returned
        assert(nrSamplesSigned >= 0);

        // According to well-informed sources, the driver indicates the buffer overrun by setting all the bits of the overflow argument to true.
        if (static_cast<uint16_t>(overflow) == 0xffff) {
            fmt::println(std::cerr, "Buffer overrun detected, continue...");
        }
    }

    void rapidBlockCallback(Error ec)
    requires(acquisitionMode == AcquisitionMode::RapidBlock)
    {
        if (ec) {
            this->emitErrorMessage("PicoscopeError", ec.message());
            return;
        }
        this->invokeWork();
    }

    template<gr::OutputSpanLike TOutSpan>
    gr::work::Status processBulk(gr::InputSpanLike auto& timingInSpan, std::span<TOutSpan>& outputs) {
        if constexpr (acquisitionMode == AcquisitionMode::Streaming) {
            if (streamingSamples == 0) {
                for (auto& output : outputs) {
                    output.publish(0);
                }
                if (const auto ec = self().driver_poll()) {
                    this->emitErrorMessage("PicoscopeError", ec.message());
                    return gr::work::Status::ERROR;
                }
            } else {
                processDriverData(streamingSamples, streamingOffset, outputs, timingInSpan);
                streamingSamples = 0;
            }
        } else {
            const auto samples = ps_settings.pre_samples + ps_settings.post_samples;
            for (std::size_t capture = 0; capture < ps_settings.rapid_block_nr_captures; ++capture) {
                const auto getValuesResult = self().driver_rapidBlockGetValues(capture, samples);
                if (getValuesResult.error) {
                    this->emitErrorMessage("PicoscopeError", getValuesResult.error.message());
                    return gr::work::Status::ERROR;
                }

                // TODO handle overflow for individual channels?
                processDriverData(getValuesResult.samples, 0, outputs, timingInSpan);
            }
        }
        if (ps_settings.trigger_once) {
            ps_state.data_finished = true;
        }

        if (ps_state.channels.empty()) {
            return gr::work::Status::OK;
        }

        if (ps_state.data_finished) {
            return gr::work::Status::DONE;
        }

        return gr::work::Status::OK;
    }

    std::vector<std::size_t> findAnalogTriggers(const detail::Channel<T>& triggerChannel, std::span<const T> samples) {
        if (samples.empty()) {
            return {};
        }

        std::vector<std::size_t> triggerOffsets; // relative offset of detected triggers
        const float              band              = triggerChannel.settings.range / 100.f;
        const float              voltageMultiplier = triggerChannel.settings.range / static_cast<float>(ps_state.max_value);

        const auto toFloat = [&voltageMultiplier, &triggerChannel](T raw) {
            if constexpr (std::is_same_v<T, float>) {
                return triggerChannel.settings.offset + triggerChannel.settings.scale * voltageMultiplier * raw;
            } else if constexpr (std::is_same_v<T, int16_t>) {
                return raw;
            } else if constexpr (std::is_same_v<T, int16_t>) {
                return static_cast<float>(raw);
            } else if constexpr (std::is_same_v<T, gr::UncertainValue<float>>) {
                return triggerChannel.settings.offset + triggerChannel.settings.scale * voltageMultiplier * raw.value;
            } else {
                static_assert(gr::meta::always_false<T>, "This type is not supported.");
            }
        };

        if (ps_settings.trigger.direction == TriggerDirection::Rising || ps_settings.trigger.direction == TriggerDirection::High) {
            for (std::size_t i = 0; i < samples.size(); i++) {
                const float value = toFloat(samples[i]);
                if (ps_state.trigger_state == 0 && value >= ps_settings.trigger.threshold) {
                    ps_state.trigger_state = 1;
                    triggerOffsets.push_back(i);
                } else if (ps_state.trigger_state == 1 && value <= ps_settings.trigger.threshold - band) {
                    ps_state.trigger_state = 0;
                }
            }
        } else if (ps_settings.trigger.direction == TriggerDirection::Falling || ps_settings.trigger.direction == TriggerDirection::Low) {
            for (std::size_t i = 0; i < samples.size(); i++) {
                const float value = toFloat(samples[i]);
                if (ps_state.trigger_state == 1 && value <= ps_settings.trigger.threshold) {
                    ps_state.trigger_state = 0;
                    triggerOffsets.push_back(i);
                } else if (ps_state.trigger_state == 0 && value >= ps_settings.trigger.threshold + band) {
                    ps_state.trigger_state = 1;
                }
            }
        }
        return triggerOffsets;
    }

    constexpr void validateDesiredActualFrequency(float desiredFreq, float actualFreq) {
        // In order to prevent exceptions/exit due to rounding errors, we don't directly compare actual_freq to desired_freq, but instead allow a difference up to 0.001%
        constexpr float kMaxDiffPercentage = 0.001f;
        const float     diff               = actualFreq / desiredFreq - 1.f;
        if (std::abs(diff) > kMaxDiffPercentage) {
            throw std::runtime_error(fmt::format("Desired and actual frequency do not match. desired: {} actual: {}", desiredFreq, actualFreq));
        }
    }

    constexpr int16_t convertVoltageToRawLogicValue(float value) {
        constexpr float kMaxLogicalVoltage = 5.0;

        if (value > kMaxLogicalVoltage) {
            throw std::invalid_argument(fmt::format("Maximum logical level is: {}, received: {}.", kMaxLogicalVoltage, value));
        }
        // Note max channel value not provided with PicoScope API, we use ext max value
        return static_cast<int16_t>(value / kMaxLogicalVoltage * self().maxVoltage());
    }

    Error driver_initialize() {
        PICO_STATUS status;

        // Required to force sequence execution of open unit calls...
        std::lock_guard initGuard{g_init_mutex};

        status = self().openUnit(this->ps_settings.serial_number);

        // ignore ext. power not connected error/warning
        if (status == PICO_POWER_SUPPLY_NOT_CONNECTED || status == PICO_USB3_0_DEVICE_NON_USB3_0_PORT) {
            status = self().changePowerSource(this->ps_state.handle, status);
            if (status == PICO_POWER_SUPPLY_NOT_CONNECTED || status == PICO_USB3_0_DEVICE_NON_USB3_0_PORT) {
                status = self().changePowerSource(this->ps_state.handle, status);
            }
        }

        if (status != PICO_OK) {
            fmt::println(std::cerr, "open unit failed: {} ", detail::getErrorMessage(status));
            return {status};
        }

        // maximum value is used for conversion to volts
        status = self().maximumValue(this->ps_state.handle, &this->ps_state.max_value);
        if (status != PICO_OK) {
            self().closeUnit(this->ps_state.handle);
            fmt::println(std::cerr, "maximumValue: {}", detail::getErrorMessage(status));
            return {status};
        }

        return {};
    }

    Error driver_close() {
        if (this->ps_state.handle == -1) {
            return {};
        }

        auto status           = self().closeUnit(this->ps_state.handle);
        this->ps_state.handle = -1;

        if (status != PICO_OK) {
            fmt::println(std::cerr, "closeUnit: {}", detail::getErrorMessage(status));
        }
        return {status};
    }

    Error driver_configure() {
        int32_t maxSamples;
        auto    status = self().memorySegments(this->ps_state.handle, static_cast<uint32_t>(this->ps_settings.rapid_block_nr_captures), &maxSamples);
        if (status != PICO_OK) {
            fmt::println(std::cerr, "MemorySegments: {}", detail::getErrorMessage(status));
            return {status};
        }

        if constexpr (acquisitionMode == AcquisitionMode::RapidBlock) {
            status = self().setNoOfCaptures(this->ps_state.handle, static_cast<uint32_t>(this->ps_settings.rapid_block_nr_captures));
            if (status != PICO_OK) {
                fmt::println(std::cerr, "SetNoOfCaptures: {}", detail::getErrorMessage(status));
                return {status};
            }
        }

        // configure analog channels
        for (const auto& channel : this->ps_state.channels) {
            const auto idx = self().convertToChannel(channel.id);
            assert(idx);
            const auto coupling = self().convertToCoupling(channel.settings.coupling);
            const auto range    = self().convertToRange(channel.settings.range);

            status = self().setChannel(this->ps_state.handle, *idx, true, coupling, static_cast<TPSImpl::ChannelRangeType>(range), static_cast<float>(channel.settings.analogOffset));
            if (status != PICO_OK) {
                fmt::println(std::cerr, "SetChannel (chan '{}'): {}", channel.id, detail::getErrorMessage(status));
                return {status};
            }
        }

        // apply trigger configuration
        if (this->ps_settings.trigger.isAnalog() && acquisitionMode == AcquisitionMode::RapidBlock) {
            const auto channel = self().convertToChannel(this->ps_settings.trigger.source);
            assert(channel);
            status = self().setSimpleTrigger(this->ps_state.handle,
                true, // enable
                *channel, convertVoltageToRawLogicValue(this->ps_settings.trigger.threshold), self().convertToThresholdDirection(this->ps_settings.trigger.direction),
                0,   // delay
                -1); // auto trigger
            if (status != PICO_OK) {
                fmt::println(std::cerr, "setSimpleTrigger: {}", detail::getErrorMessage(status));
                return {status};
            }
        } else {
            // disable triggers
            for (int i = 0; i < self().maxChannel(); i++) {
                typename TPSImpl::ConditionType cond;
                cond.source    = static_cast<TPSImpl::ChannelType>(i);
                cond.condition = self().conditionDontCare();
                status         = self().setTriggerChannelConditions(this->ps_state.handle, &cond, 1, self().conditionsInfoClear());
                if (status != PICO_OK) {
                    fmt::println(std::cerr, "SetTriggerChannelConditionsV2: {}", detail::getErrorMessage(status));
                    return {status};
                }
            }
        }

        return {};
    }

    Error driver_disarm() noexcept {
        if (const auto status = self().driver_stop(this->ps_state.handle); status != PICO_OK) {
            fmt::println(std::cerr, "Stop: {}", detail::getErrorMessage(status));
            return {status};
        }

        return {};
    }

    Error driver_arm() {
        if constexpr (acquisitionMode == AcquisitionMode::RapidBlock) {
            auto timebaseResult               = self().convertSampleRateToTimebase(this->ps_state.handle, this->ps_settings.sample_rate);
            this->ps_state.actual_sample_rate = timebaseResult.actualFreq;
            fmt::println("RapidBlock mode - desired sample rate:{}, actual sample rate:{}", this->ps_settings.sample_rate, this->ps_state.actual_sample_rate);

            static auto redirector = [](int16_t, PICO_STATUS status, void* vobj) { static_cast<decltype(this)>(vobj)->rapidBlockCallback({status}); };

            auto status = self().runBlock(                            //
                this->ps_state.handle,                                // identifier for the scope device
                static_cast<int32_t>(this->ps_settings.pre_samples),  // number of samples before trigger event
                static_cast<int32_t>(this->ps_settings.post_samples), // number of samples after trigger event
                timebaseResult.timebase,                              // timebase
                nullptr,                                              // time indispossed
                0,                                                    // segment index
                static_cast<TPSImpl::BlockReadyType>(redirector),     // BlockReady() callback
                this);

            if (status != PICO_OK) {
                fmt::println(std::cerr, "RunBlock: {}", detail::getErrorMessage(status));
                return {status};
            }
        } else {
            using fair::picoscope::detail::kDriverBufferSize;
            self().setBuffers(kDriverBufferSize, 0);

            TimeInterval timeInterval = convertSampleRateToTimeInterval(this->ps_settings.sample_rate);

            auto status = self().runStreaming(              //
                this->ps_state.handle,                      // identifier for the scope device
                &timeInterval.interval,                     // in: desired interval, out: actual interval
                self().convertTimeUnits(timeInterval.unit), // time unit of interval
                0,                                          // pre-trigger-samples (unused)
                static_cast<uint32_t>(kDriverBufferSize),   // post-trigger-samples
                false,                                      // autoStop
                1,                                          // downsampling ratio // TODO: reconsider if we need downsampling support
                self().ratioNone(),                         // downsampling ratio mode
                static_cast<uint32_t>(kDriverBufferSize));  // the size of the overview buffers

            if (status != PICO_OK) {
                fmt::println(std::cerr, "RunStreaming: {}", detail::getErrorMessage(status));
                return {status};
            }
            // calculate actual sample rate
            this->ps_state.actual_sample_rate = convertTimeIntervalToSampleRate(timeInterval);
            fmt::println("Streaming mode - desired sample rate:{}, actual sample rate:{}", this->ps_settings.sample_rate, this->ps_state.actual_sample_rate);
        }

        return {};
    }

    Error driver_poll() {
        static auto redirector = [](int16_t /*handle*/, int32_t noOfSamples, uint32_t startIndex, int16_t overflow, uint32_t /*triggerAt*/, int16_t /*triggered*/, int16_t /*autoStop*/, void* vobj) { static_cast<decltype(this)>(vobj)->streamingCallback(noOfSamples, startIndex, overflow); };

        const auto status = self().getStreamingLatestValues(this->ps_state.handle, static_cast<TPSImpl::StreamingReadyType>(redirector), this);
        if (status == PICO_BUSY || status == PICO_DRIVER_FUNCTION) {
            return {};
        }
        return {status};
    }

    fair::picoscope::GetValuesResult driver_rapidBlockGetValues(std::size_t capture, std::size_t samples) {
        if (const auto ec = self().setBuffers(samples, static_cast<uint32_t>(capture)); ec) {
            return {ec, 0, 0};
        }

        auto       nrSamples = static_cast<uint32_t>(samples);
        int16_t    overflow  = 0;
        const auto status    = self().getValues(this->ps_state.handle,
               0, // offset
               &nrSamples, 1, self().ratioNone(), static_cast<uint32_t>(capture), &overflow);
        if (status != PICO_OK) {
            fmt::println(std::cerr, "GetValues: {}", detail::getErrorMessage(status));
            return {{status}, 0, 0};
        }

        return {{}, static_cast<std::size_t>(nrSamples), overflow};
    }

    Error setBuffers(size_t samples, uint32_t blockNumber) {
        for (auto& channel : this->ps_state.channels) {
            const auto channelIndex = self().convertToChannel(channel.id);
            assert(channelIndex);

            channel.driver_buffer.resize(std::max(samples, channel.driver_buffer.size()));
            const auto status = self().setDataBuffer(this->ps_state.handle, *channelIndex, channel.driver_buffer.data(), static_cast<int32_t>(samples), blockNumber, self().ratioNone());

            if (status != PICO_OK) {
                fmt::println(std::cerr, "SetDataBuffer (chan {}): {}", static_cast<std::size_t>(*channelIndex), detail::getErrorMessage(status));
                return {status};
            }
        }

        return {};
    }

    std::string getUnitInfoTopic(int16_t handle, PICO_INFO info) const {
        std::array<int8_t, 40> line{};
        int16_t                requiredSize;

        auto status = self().getUnitInfo(handle, line.data(), line.size(), &requiredSize, info);
        if (status == PICO_OK) {
            return {reinterpret_cast<char*>(line.data()), static_cast<std::size_t>(requiredSize)};
        }

        return {};
    }

    std::string driver_driverVersion() const {
        const std::string prefix  = "Picoscope Linux Driver, ";
        auto              version = getUnitInfoTopic(this->ps_state.handle, PICO_DRIVER_VERSION);

        if (auto i = version.find(prefix); i != std::string::npos) {
            version.erase(i, prefix.length());
        }
        return version;
    }

    std::string driver_hardwareVersion() const {
        if (!this->ps_state.initialized) {
            return {};
        }
        return getUnitInfoTopic(this->ps_state.handle, PICO_HARDWARE_VERSION);
    }

    std::string driver_serialNumber() const {
        if (!this->ps_state.initialized) {
            return {};
        }
        return getUnitInfoTopic(this->ps_state.handle, PICO_BATCH_AND_SERIAL);
    }

    std::optional<std::size_t> driver_channelIdToIndex(std::string_view id) {
        const auto channel = self().convertToChannel(id);
        if (!channel) {
            return {};
        }
        return static_cast<std::size_t>(*channel);
    }

    [[nodiscard]] constexpr auto& self() noexcept { return *static_cast<TPSImpl*>(this); }

    [[nodiscard]] constexpr const auto& self() const noexcept { return *static_cast<const TPSImpl*>(this); }
};

} // namespace fair::picoscope

#endif
