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

    std::string message() const { return detail::getErrorMessage(code); }

    explicit constexpr operator bool() const noexcept { return code != PICO_OK; }
};

enum class AcquisitionMode { Streaming, RapidBlock };

enum class Coupling {
    DC_1M,  ///< DC, 1 MOhm
    AC_1M,  ///< AC, 1 MOhm
    DC_50R, ///< DC, 50 Ohm
};

enum class TriggerDirection { Rising, Falling, Low, High };

struct GetValuesResult {
    Error       error;
    std::size_t samples;
    int16_t     overflow;
};

namespace detail {

constexpr std::size_t kDriverBufferSize = 65536;

enum class PollerState { Idle, Running, Exit };

struct ChannelSetting {
    std::string name;
    std::string unit     = "V";
    double      range    = 2.;
    double      offset   = 0.;
    Coupling    coupling = Coupling::AC_1M;
};

using ChannelMap = std::map<std::string, ChannelSetting, std::less<>>;

struct TriggerSetting {
    static constexpr std::string_view kTriggerDigitalSource = "DI"; // DI is as well used as "AUX" for p6000 scopes

    bool isEnabled() const { return !source.empty(); }

    bool isDigital() const { return isEnabled() && source == kTriggerDigitalSource; }

    bool isAnalog() const { return isEnabled() && source != kTriggerDigitalSource; }

    std::string      source;
    double           threshold  = 0; // AI only
    TriggerDirection direction  = TriggerDirection::Rising;
    int              pin_number = 0; // DI only
};

template<typename T>
struct Channel {
    using ValueWriterType = decltype(std::declval<gr::CircularBuffer<T>>().new_writer());
    using TagWriterType   = decltype(std::declval<gr::CircularBuffer<gr::Tag>>().new_writer());
    std::string          id;
    ChannelSetting       settings;
    std::vector<int16_t> driver_buffer;
    ValueWriterType      data_writer;
    TagWriterType        tag_writer;
    bool                 signal_info_written = false;

    gr::property_map signalInfo() const {
        using namespace gr;
        static const auto kSignalName = std::string(tag::SIGNAL_NAME.shortKey());
        static const auto kSignalUnit = std::string(tag::SIGNAL_UNIT.shortKey());
        static const auto kSignalMin  = std::string(tag::SIGNAL_MIN.shortKey());
        static const auto kSignalMax  = std::string(tag::SIGNAL_MAX.shortKey());
        return {{kSignalName, settings.name}, {kSignalUnit, settings.unit}, {kSignalMin, static_cast<float>(settings.offset)}, {kSignalMax, static_cast<float>(settings.offset + settings.range)}};
    }
};

struct ErrorWithSample {
    std::size_t sample;
    Error       error;
};

template<typename T, std::size_t initialSize = 1>
struct BufferHelper {
    gr::CircularBuffer<T>         buffer = gr::CircularBuffer<T>(initialSize);
    decltype(buffer.new_reader()) reader = buffer.new_reader();
    decltype(buffer.new_writer()) writer = buffer.new_writer();
};

struct Settings {
    std::string     serial_number;
    float           sample_rate              = 10000.f;
    AcquisitionMode acquisition_mode         = AcquisitionMode::Streaming;
    std::size_t     pre_samples              = 1000;
    std::size_t     post_samples             = 9000;
    std::size_t     rapid_block_nr_captures  = 1;
    bool            trigger_once             = false;
    double          streaming_mode_poll_rate = 0.001;
    bool            auto_arm                 = true;
    ChannelMap      enabled_channels;
    TriggerSetting  trigger;
};

template<typename T>
struct State {
    std::vector<Channel<T>>       channels;
    std::atomic<bool>             data_finished      = false;
    bool                          initialized        = false;
    bool                          configured         = false;
    bool                          closed             = false;
    bool                          armed              = false;
    bool                          started            = false; // TODO transitional until nodes have state handling
    int8_t                        trigger_state      = 0;
    int16_t                       handle             = -1; ///< picoscope handle
    int16_t                       overflow           = 0;  ///< status returned from getValues
    int16_t                       max_value          = 0;  ///< maximum ADC count used for ADC conversion
    double                        actual_sample_rate = 0;
    std::thread                   poller;
    BufferHelper<ErrorWithSample> errors;
    std::atomic<PollerState>      poller_state    = PollerState::Idle;
    std::size_t                   produced_worker = 0; // poller/callback thread
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

inline ChannelMap channelSettings(std::span<const std::string> ids, std::span<const std::string> names, std::span<const std::string> units, std::span<const double> ranges, std::span<const double> offsets, std::span<const std::string> couplings) {
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
        if (i < ranges.size()) {
            channel.range = ranges[i];
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
    using type = gr::Block<TPSImpl, gr::SupportedTypes<int16_t, float, double>>;
};

template<typename TPSImpl>
struct PicoscopeBlockingHelper<TPSImpl, true> {
    using type = gr::Block<TPSImpl, gr::SupportedTypes<int16_t, float, double>, gr::BlockingIO<false>>;
};

template<PicoscopeOutput T, AcquisitionMode acquisitionMode, typename TPSImpl>
class Picoscope : public PicoscopeBlockingHelper<TPSImpl, acquisitionMode == AcquisitionMode::RapidBlock>::type {
public:
    using super_t = typename PicoscopeBlockingHelper<TPSImpl, acquisitionMode == AcquisitionMode::RapidBlock>::type;

    Picoscope(gr::property_map props) : super_t(std::move(props)) {}

    A<std::string, "serial number">  serial_number;
    A<float, "sample rate", Visible> sample_rate = 10000.f;
    // TODO any way to get custom enums into pmtv??
    A<std::string, "acquisition mode", Visible>                       acquisition_mode         = std::string("Streaming");
    A<std::size_t, "pre-samples">                                     pre_samples              = 1000;
    A<std::size_t, "post-samples">                                    post_samples             = 9000;
    A<std::size_t, "no. captures (rapid block mode)">                 rapid_block_nr_captures  = 1;
    A<bool, "trigger once (rapid block mode)">                        trigger_once             = false;
    A<double, "poll rate (streaming mode)">                           streaming_mode_poll_rate = 0.001;
    A<bool, "auto-arm">                                               auto_arm                 = true;
    A<std::vector<std::string>, "IDs of enabled channels">            channel_ids;
    A<std::vector<std::string>, "Names of enabled channels">          channel_names;
    A<std::vector<std::string>, "Units of enabled channels">          channel_units;
    std::vector<double>                                               channel_ranges;
    std::vector<double>                                               channel_offsets;
    A<std::vector<std::string>, "Coupling modes of enabled channels"> channel_couplings;
    A<std::string, "trigger channel/port ID">                         trigger_source;
    A<double, "trigger threshold, analog only">                       trigger_threshold = 0.f;
    A<std::string, "trigger direction">                               trigger_direction = std::string("Rising");
    A<int, "trigger pin, digital only">                               trigger_pin       = 0;

    detail::State<T> ps_state;
    detail::Settings ps_settings;

private:
    std::mutex                   g_init_mutex;
    std::size_t                  streamingSamples = 0Z;
    std::queue<gr::property_map> timingMessages;

public:
    ~Picoscope() { stop(); }

    void settingsChanged(const gr::property_map& /*old_settings*/, const gr::property_map& /*new_settings*/) {
        const auto wasStarted = ps_state.started;
        if (wasStarted) {
            stop();
        }
        try {
            auto s = detail::Settings{.serial_number = serial_number, .sample_rate = sample_rate, .acquisition_mode = detail::parseAcquisitionMode(acquisition_mode), .pre_samples = pre_samples, .post_samples = post_samples, .rapid_block_nr_captures = rapid_block_nr_captures, .trigger_once = trigger_once, .streaming_mode_poll_rate = streaming_mode_poll_rate, .auto_arm = auto_arm, .enabled_channels = detail::channelSettings(channel_ids.value, channel_names.value, channel_units.value, channel_ranges, channel_offsets, channel_couplings.value), .trigger = detail::TriggerSetting{.source = trigger_source, .threshold = trigger_threshold, .direction = detail::parseTriggerDirection(trigger_direction), .pin_number = trigger_pin}};
            std::swap(ps_settings, s);

            ps_state.channels.reserve(ps_settings.enabled_channels.size());
            std::size_t channelIdx = 0;
            for (const auto& [id, settings] : ps_settings.enabled_channels) {
                ps_state.channels.emplace_back(detail::Channel<T>{.id = id, .settings = settings, .driver_buffer = std::vector<int16_t>(detail::kDriverBufferSize), .data_writer = self().analog_out[channelIdx].streamWriter().buffer().new_writer(), .tag_writer = self().analog_out[channelIdx].tagWriter().buffer().new_writer()});
                channelIdx++;
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
            if (acquisitionMode == AcquisitionMode::Streaming) {
                startPollThread();
            }
            ps_state.started = true;
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

        if (acquisitionMode == AcquisitionMode::Streaming) {
            stopPollThread();
        }
    }

    void startPollThread() {
        if (ps_state.poller.joinable()) {
            return;
        }

        if (ps_state.poller_state == detail::PollerState::Exit) {
            ps_state.poller_state = detail::PollerState::Idle;
        }
    }

    void stopPollThread() {
        ps_state.poller_state = detail::PollerState::Exit;
        if (ps_state.poller.joinable()) {
            ps_state.poller.join();
        }
    }

    std::string driverVersion() const { return self().driver_driverVersion(); }

    std::string hardwareVersion() const { return self().driver_hardwareVersion(); }

    void initialize() {
        if (ps_state.initialized) {
            return;
        }

        if (const auto ec = self().driver_initialize()) {
            throw std::runtime_error(fmt::format("Initialization failed: {}", ec.message()));
        }

        ps_state.initialized = true;
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
        if (acquisitionMode == AcquisitionMode::Streaming) {
            ps_state.poller_state = detail::PollerState::Running;
        }
    }

    void disarm() noexcept {
        if (!ps_state.armed) {
            return;
        }

        if (acquisitionMode == AcquisitionMode::Streaming) {
            ps_state.poller_state = detail::PollerState::Idle;
        }

        if (const auto ec = self().driver_disarm()) {
            fmt::println(std::cerr, "disarm failed: {}", ec.message());
        }

        ps_state.armed = false;
    }

    void processDriverData(std::size_t nrSamples, std::size_t offset, auto& outputs) {
        std::vector<std::size_t> triggerOffsets;

        using ChannelOutputRange = decltype(ps_state.channels[0].data_writer.reserve(1));
        std::vector<ChannelOutputRange> channelOutputs;
        channelOutputs.reserve(ps_state.channels.size());

        for (std::size_t channelIdx = 0; channelIdx < ps_state.channels.size(); ++channelIdx) {
            auto& channel = ps_state.channels[channelIdx];

            // channelOutputs.push_back(channel.data_writer.reserve(nrSamples));
            // auto      &output     = channelOutputs[channelIdx];
            auto& output = outputs[channelIdx];

            const auto driverData = std::span(channel.driver_buffer).subspan(offset, nrSamples);

            if constexpr (std::is_same_v<T, int16_t>) {
                std::copy(driverData.begin(), driverData.end(), output.begin());
            } else {
                const auto voltageMultiplier = static_cast<float>(channel.settings.range / ps_state.max_value);
                // TODO use SIMD
                for (std::size_t i = 0; i < nrSamples; ++i) {
                    if constexpr (std::is_same_v<T, float>) {
                        output[i] = voltageMultiplier * driverData[i];
                    } else if constexpr (std::is_same_v<T, gr::UncertainValue<float>>) {
                        output[i] = gr::UncertainValue(voltageMultiplier * driverData[i], self().uncertainty());
                    }
                }
            }

            if (channel.id == ps_settings.trigger.source) {
                triggerOffsets = findAnalogTriggers(channel, output);
            }
        }

        const auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());

        // TODO wait (block) here for timing messages if trigger count > timing message count

        std::vector<gr::Tag> triggerTags;
        triggerTags.reserve(triggerOffsets.size());

        for (const auto triggerOffset : triggerOffsets) {
            gr::property_map timing;
            if (timingMessages.empty()) {
                // fallback to ad-hoc timing message
                timing = gr::property_map{{gr::tag::TRIGGER_NAME, "PPS"}, {gr::tag::TRIGGER_TIME, static_cast<uint64_t>(now.count())}};
            } else {
                // use timing message that we received over the message port if any
                timing = timingMessages.front();
                timingMessages.pop();
            }

            // raw index is index - 1
            triggerTags.emplace_back(static_cast<int64_t>(ps_state.produced_worker + triggerOffset - 1), timing);
        }

        for (auto& channel : ps_state.channels) {
            const auto writeSignalInfo = !channel.signal_info_written;
            const auto tagsToWrite     = triggerTags.size() + (writeSignalInfo ? 1 : 0);
            if (tagsToWrite == 0) {
                continue;
            }
            auto writeTags = channel.tag_writer.reserve(tagsToWrite);
            if (writeSignalInfo) {
                // raw index is index - 1
                writeTags[0].index            = static_cast<int64_t>(ps_state.produced_worker - 1);
                writeTags[0].map              = channel.signalInfo();
                static const auto kSampleRate = std::string(gr::tag::SAMPLE_RATE.shortKey());
                writeTags[0].map[kSampleRate] = pmtv::pmt(static_cast<float>(sample_rate));
                channel.signal_info_written   = true;
            }
            std::copy(triggerTags.begin(), triggerTags.end(), writeTags.begin() + (writeSignalInfo ? 1 : 0));
            writeTags.publish(writeTags.size());
        }

        // once all tags have been written, publish the data
        for (auto& output : outputs) {
            output.publish(nrSamples);
        }

        ps_state.produced_worker += nrSamples;
    }

    void streamingCallback(int32_t nrSamplesSigned, uint32_t, int16_t overflow)
    requires(acquisitionMode == AcquisitionMode::Streaming)
    {
        streamingSamples = static_cast<std::size_t>(nrSamplesSigned);
        assert(nrSamplesSigned >= 0);
        const auto nrSamples = static_cast<std::size_t>(nrSamplesSigned);

        // According to well informed sources, the driver indicates the buffer overrun by
        // setting all the bits of the overflow argument to true.
        if (static_cast<uint16_t>(overflow) == 0xffff) {
            fmt::println(std::cerr, "Buffer overrun detected, continue...");
        }

        if (nrSamples == 0) {
            return;
        }
    }

    void rapidBlockCallback(Error ec)
    requires(acquisitionMode == AcquisitionMode::RapidBlock)
    {
        if (ec) {
            reportError(ec);
            return;
        }
        this->invokeWork();
    }

    template<gr::PublishableSpan TOutSpan>
    gr::work::Status processBulk(std::span<TOutSpan>& output) {
        if constexpr (acquisitionMode == AcquisitionMode::Streaming) {
            if (const auto ec = self().driver_poll()) {
                reportError(ec);
                fmt::println(std::cerr, "poll failed");
                return gr::work::Status::ERROR;
            }
            processDriverData(streamingSamples, 0, output);
            streamingSamples = 0;
        } else {
            const auto samples = ps_settings.pre_samples + ps_settings.post_samples;
            for (std::size_t capture = 0; capture < ps_settings.rapid_block_nr_captures; ++capture) {
                const auto getValuesResult = self().driver_rapidBlockGetValues(capture, samples);
                if (getValuesResult.error) {
                    reportError(getValuesResult.error);
                    return gr::work::Status::ERROR;
                }

                // TODO handle overflow for individual channels?
                processDriverData(getValuesResult.samples, 0, output);
            }
        }

        if (ps_settings.trigger_once) {
            ps_state.data_finished = true;
        }

        if (ps_state.channels.empty()) {
            return gr::work::Status::OK;
        }

        if (const auto errors_available = ps_state.errors.reader.available(); errors_available > 0) {
            auto errors = ps_state.errors.reader.get(errors_available);
            std::ignore = errors.consume(errors.size());
            return gr::work::Status::ERROR;
        }

        if (ps_state.data_finished) {
            return gr::work::Status::DONE;
        }

        return gr::work::Status::OK;
    }

    void processMessages(gr::MsgPortInNamed<"__Builtin">&, std::span<const gr::Message> messages) {
        for (auto& msg : messages) {
            if (msg.data.has_value()) {
                // store timing messages for later use when triggers occur
                timingMessages.push(msg.data.value());
            }
        }
    }

    std::vector<std::size_t> findAnalogTriggers(const detail::Channel<T>& triggerChannel, std::span<const T> samples) {
        if (samples.empty()) {
            return {};
        }

        std::vector<std::size_t> triggerOffsets; // relative offset of detected triggers
        const auto               band              = triggerChannel.settings.range / 100.;
        const auto               voltageMultiplier = triggerChannel.settings.range / ps_state.max_value;

        const auto toDouble = [&voltageMultiplier](T raw) {
            if constexpr (std::is_same_v<T, float>) {
                return static_cast<double>(raw);
            } else if constexpr (std::is_same_v<T, double>) {
                return raw;
            } else if constexpr (std::is_same_v<T, gr::UncertainValue<float>>) {
                return voltageMultiplier * static_cast<double>(raw.value);
            } else {
                return voltageMultiplier * raw;
            }
        };

        if (ps_settings.trigger.direction == TriggerDirection::Rising || ps_settings.trigger.direction == TriggerDirection::High) {
            for (std::size_t i = 0; i < samples.size(); i++) {
                const auto value = toDouble(samples[i]);
                if (ps_state.trigger_state == 0 && value >= ps_settings.trigger.threshold) {
                    ps_state.trigger_state = 1;
                    triggerOffsets.push_back(i);
                } else if (ps_state.trigger_state == 1 && value <= ps_settings.trigger.threshold - band) {
                    ps_state.trigger_state = 0;
                }
            }
        } else if (ps_settings.trigger.direction == TriggerDirection::Falling || ps_settings.trigger.direction == TriggerDirection::Low) {
            for (std::size_t i = 0; i < samples.size(); i++) {
                const auto value = toDouble(samples[i]);
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

    void reportError(Error ec) {
        auto out = ps_state.errors.writer.reserve(1);
        out[0]   = {ps_state.produced_worker, ec};
        out.publish(1);
    }

    constexpr void validateDesiredActualFrequency(double desiredFreq, double actualFreq) {
        // In order to prevent exceptions/exit due to rounding errors, we dont directly
        // compare actual_freq to desired_freq, but instead allow a difference up to 0.001%
        constexpr double kMaxDiffPercentage = 0.001;
        const double     diff               = actualFreq / desiredFreq - 1;
        if (std::abs(diff) > kMaxDiffPercentage) {
            throw std::runtime_error(fmt::format("Desired and actual frequency do not match. desired: {} actual: {}", desiredFreq, actualFreq));
        }
    }

    constexpr int16_t convertVoltageToRawLogicValue(double value) {
        constexpr double kMaxLogicalVoltage = 5.0;

        if (value > kMaxLogicalVoltage) {
            throw std::invalid_argument(fmt::format("Maximum logical level is: {}, received: {}.", kMaxLogicalVoltage, value));
        }
        // Note max channel value not provided with PicoScope API, we use ext max value
        return static_cast<int16_t>(value / kMaxLogicalVoltage * self().maxVoltage());
    }

    /*!
     * Note this function has to be called after the call to the ps3000aSetChannel function,
     * that is just befor the arm!!!
     */
    constexpr uint32_t convertFrequencyToTimebase(int16_t handle, double desiredFreq, double& actualFreq) {
        // It is assumed that the timebase is calculated like this:
        // (timebase–2) / 125,000,000
        // e.g. timeebase == 3 --> 8ns sample interval
        //
        // Note, for some devices, the above formula might be wrong! To overcome this
        // limitation we use the ps3000aGetTimebase2 function to find the closest possible
        // timebase. The below timebase estimate is therefore used as a fallback only.
        auto     timeIntervalNS   = 1000000000.0 / desiredFreq;
        uint32_t timebaseEstimate = (static_cast<uint32_t>(timeIntervalNS) / 8) + 2;

        // In order to cover for all possible 30000 series devices, we use ps3000aGetTimebase2
        // function to get step size in ns between timebase 3 and 4. Based on that the actual
        // timebase is calculated.
        int32_t              dummy;
        std::array<float, 2> timeIntervalNS_34;

        for (std::size_t i = 0; i < 2; i++) {
            auto status = self().getTimebase2(handle, 3 + static_cast<uint32_t>(i), 1024, &timeIntervalNS_34[i], &dummy, 0);
            if (status != PICO_OK) {
#ifdef PROTO_PORT_DISABLED
                d_logger->notice("timebase cannot be obtained: {}", get_error_message(status));
                d_logger->notice("    estimated timebase will be used...");
#endif

                float timeIntervalNS_tmp;
                status = self().getTimebase2(handle, timebaseEstimate, 1024, &timeIntervalNS_tmp, &dummy, 0);
                if (status != PICO_OK) {
                    throw std::runtime_error(fmt::format("Local time {}. Error: {}", timebaseEstimate, detail::getErrorMessage(status)));
                }

                actualFreq = 1000000000.0 / static_cast<double>(timeIntervalNS_tmp);
                this->validateDesiredActualFrequency(desiredFreq, actualFreq);
                return timebaseEstimate;
            }
        }

        // Calculate steps between timebase 3 and 4 and correct start_timebase estimate based
        // on that
        auto step        = static_cast<double>(timeIntervalNS_34[1] - timeIntervalNS_34[0]);
        timebaseEstimate = static_cast<uint32_t>((timeIntervalNS - static_cast<double>(timeIntervalNS_34[0])) / step) + 3;

        // The below code iterates trought the neighbouring timebases in order to find the
        // best match. In principle we could check only timebases on the left and right but
        // since first three timebases are in most cases special we make search space a bit
        // bigger.
        const int                      searchSpace = 8;
        std::array<float, searchSpace> timebases;
        std::array<float, searchSpace> errorEstimates;

        uint32_t startTimebase = timebaseEstimate > (searchSpace / 2) ? timebaseEstimate - (searchSpace / 2) : 0;

        for (std::size_t i = 0; i < searchSpace; i++) {
            float obtained_time_interval_ns;
            auto  status = self().getTimebase2(handle, startTimebase + static_cast<uint32_t>(i), 1024, &obtained_time_interval_ns, &dummy, 0);
            if (status != PICO_OK) {
                // this timebase can't be used, lets set error estimate to something big
                timebases[i]      = -1;
                errorEstimates[i] = 10000000000.0;
            } else {
                timebases[i]      = obtained_time_interval_ns;
                errorEstimates[i] = static_cast<float>(std::abs(timeIntervalNS - static_cast<double>(obtained_time_interval_ns)));
            }
        }

        auto it       = std::min_element(&errorEstimates[0], &errorEstimates[0] + errorEstimates.size());
        auto distance = std::distance(&errorEstimates[0], it);

        assert(distance < searchSpace);

        // update actual update rate and return timebase number
        actualFreq = 1000000000.0 / static_cast<double>(timebases[static_cast<std::size_t>(distance)]);
        this->validateDesiredActualFrequency(desiredFreq, actualFreq);
        return static_cast<uint32_t>(startTimebase + distance);
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

            status = self().setChannel(this->ps_state.handle, *idx, true, coupling, static_cast<TPSImpl::ChannelRangeType>(range), static_cast<float>(channel.settings.offset));
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

        // In order to validate desired frequency before startup
        double actual_freq;
        convertFrequencyToTimebase(this->ps_state.handle, this->ps_settings.sample_rate, actual_freq);

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
            uint32_t timebase = this->convertFrequencyToTimebase(this->ps_state.handle, this->ps_settings.sample_rate, this->ps_state.actual_sample_rate);

            static auto redirector = [](int16_t, PICO_STATUS status, void* vobj) { static_cast<decltype(this)>(vobj)->rapidBlockCallback({status}); };

            auto status = self().runBlock(this->ps_state.handle, static_cast<int32_t>(this->ps_settings.pre_samples), static_cast<int32_t>(this->ps_settings.post_samples),
                timebase, // timebase
                nullptr,  // time indispossed
                0,        // segment index
                static_cast<TPSImpl::BlockReadyType>(redirector), this);
            if (status != PICO_OK) {
                fmt::println(std::cerr, "RunBlock: {}", detail::getErrorMessage(status));
                return {status};
            }
        } else {
            using fair::picoscope::detail::kDriverBufferSize;
            self().setBuffers(kDriverBufferSize, 0);

            auto unit_int = self().convertFrequencyToTimeUnitsAndInterval(this->ps_settings.sample_rate, this->ps_state.actual_sample_rate);

            auto status = self().runStreaming(this->ps_state.handle,
                &unit_int.interval, // sample interval
                unit_int.unit,      // time unit of sample interval
                0,                  // pre-triggersamples (unused)
                static_cast<uint32_t>(kDriverBufferSize), false,
                1, // downsampling factor // TODO reconsider if we need downsampling support
                self().ratioNone(), static_cast<uint32_t>(kDriverBufferSize));

            if (status != PICO_OK) {
                fmt::println(std::cerr, "RunStreaming: {}", detail::getErrorMessage(status));
                return {status};
            }
        }

        return {};
    }

    Error driver_poll() {
        static auto redirector = [](int16_t handle, int32_t noOfSamples, uint32_t startIndex, int16_t overflow, uint32_t triggerAt, int16_t triggered, int16_t autoStop, void* vobj) {
            std::ignore = handle;
            std::ignore = triggerAt;
            std::ignore = triggered;
            std::ignore = autoStop;
            static_cast<decltype(this)>(vobj)->streamingCallback(noOfSamples, startIndex, overflow);
        };

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
        std::array<int8_t, 40> line;
        int16_t                requiredSize;

        auto status = self().getUnitInfo(handle, line.data(), line.size(), &requiredSize, info);
        if (status == PICO_OK) {
            return std::string(reinterpret_cast<char*>(line.data()), static_cast<std::size_t>(requiredSize));
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
