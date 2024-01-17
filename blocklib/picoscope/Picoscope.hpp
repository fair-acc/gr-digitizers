#ifndef FAIR_PICOSCOPE_PICOSCOPE_HPP
#define FAIR_PICOSCOPE_PICOSCOPE_HPP

#include "StatusMessages.hpp"

#include <gnuradio-4.0/Block.hpp>

#include <fmt/format.h>

#include <functional>

// #define GR_PICOSCOPE_POLLER_THREAD 1

namespace fair::picoscope {

struct Error {
    PICO_STATUS code = PICO_OK;

    std::string
    message() const {
        return detail::getErrorMessage(code);
    }

    explicit constexpr
    operator bool() const noexcept {
        return code != PICO_OK;
    }
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

    bool
    isEnabled() const {
        return !source.empty();
    }

    bool
    isDigital() const {
        return isEnabled() && source == kTriggerDigitalSource;
    }

    bool
    isAnalog() const {
        return isEnabled() && source != kTriggerDigitalSource;
    }

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

    gr::property_map
    signalInfo() const {
        using namespace gr;
        static const auto kSignalName = std::string(tag::SIGNAL_NAME.key());
        static const auto kSignalUnit = std::string(tag::SIGNAL_UNIT.key());
        static const auto kSignalMin  = std::string(tag::SIGNAL_MIN.key());
        static const auto kSignalMax  = std::string(tag::SIGNAL_MAX.key());
        return {
            { kSignalName, settings.name }, { kSignalUnit, settings.unit }, { kSignalMin, static_cast<float>(settings.offset) }, { kSignalMax, static_cast<float>(settings.offset + settings.range) }
        };
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
    double          sample_rate              = 10000.;
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

inline AcquisitionMode
parseAcquisitionMode(std::string_view s) {
    using enum AcquisitionMode;
    if (s == "RapidBlock") return RapidBlock;
    if (s == "Streaming") return Streaming;
    throw std::invalid_argument(fmt::format("Unknown acquisition mode '{}'", s));
}

inline Coupling
parseCoupling(std::string_view s) {
    using enum Coupling;
    if (s == "DC_1M") return DC_1M;
    if (s == "AC_1M") return AC_1M;
    if (s == "DC_50R") return DC_50R;
    throw std::invalid_argument(fmt::format("Unknown coupling type '{}'", s));
}

inline TriggerDirection
parseTriggerDirection(std::string_view s) {
    using enum TriggerDirection;
    if (s == "Rising") return Rising;
    if (s == "Falling") return Falling;
    if (s == "Low") return Low;
    if (s == "High") return High;
    throw std::invalid_argument(fmt::format("Unknown trigger direction '{}'", s));
}

inline ChannelMap
channelSettings(std::span<const std::string> ids, std::span<const std::string> names, std::span<const std::string> units, std::span<const double> ranges, std::span<const double> offsets,
                std::span<const std::string> couplings) {
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

template<typename T, typename TPSImpl>
struct Picoscope : public gr::Block<TPSImpl, gr::BlockingIO<true>, gr::SupportedTypes<int16_t, float, double>> {
    A<std::string, "serial number">   serial_number;
    A<double, "sample rate", Visible> sample_rate = 10000.;
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

    detail::State<T>                                                  ps_state;
    detail::Settings                                                  ps_settings;

    ~Picoscope() { stop(); }

    void
    settingsChanged(const gr::property_map & /*old_settings*/, const gr::property_map & /*new_settings*/) {
        const auto wasStarted = ps_state.started;
        if (wasStarted) {
            stop();
        }
        try {
            auto s = detail::Settings{
                .serial_number            = serial_number,
                .sample_rate              = sample_rate,
                .acquisition_mode         = detail::parseAcquisitionMode(acquisition_mode),
                .pre_samples              = pre_samples,
                .post_samples             = post_samples,
                .rapid_block_nr_captures  = rapid_block_nr_captures,
                .trigger_once             = trigger_once,
                .streaming_mode_poll_rate = streaming_mode_poll_rate,
                .auto_arm                 = auto_arm,
                .enabled_channels         = detail::channelSettings(channel_ids.value, channel_names.value, channel_units.value, channel_ranges, channel_offsets, channel_couplings.value),
                .trigger = detail::TriggerSetting{ .source = trigger_source, .threshold = trigger_threshold, .direction = detail::parseTriggerDirection(trigger_direction), .pin_number = trigger_pin }
            };
            std::swap(ps_settings, s);

            ps_state.channels.reserve(ps_settings.enabled_channels.size());
            std::size_t channelIdx = 0;
            for (const auto &[id, settings] : ps_settings.enabled_channels) {
                ps_state.channels.emplace_back(detail::Channel<T>{ .id            = id,
                                                                   .settings      = settings,
                                                                   .driver_buffer = std::vector<int16_t>(detail::kDriverBufferSize),
                                                                   .data_writer   = self().analog_out[channelIdx].streamWriter().buffer().new_writer(),
                                                                   .tag_writer    = self().analog_out[channelIdx].tagWriter().buffer().new_writer() });
                channelIdx++;
            }
        } catch (const std::exception &e) {
            // TODO add errors properly
            fmt::println(std::cerr, "Could not apply settings: {}", e.what());
        }
        if (wasStarted) {
            start();
        }
    }

    gr::work::Result
    workImpl() noexcept {
        using enum gr::work::Status;

        if (ps_state.channels.empty()) {
            return { 0, 0, ERROR };
        }

        if (const auto errors_available = ps_state.errors.reader.available(); errors_available > 0) {
            auto errors = ps_state.errors.reader.get(errors_available);
            std::ignore = ps_state.errors.reader.consume(errors_available);
            return { 0, 0, ERROR };
        }

        if (ps_state.data_finished) {
            std::atomic_store_explicit(&this->state, gr::lifecycle::State::STOPPED, std::memory_order_release);
            this->state.notify_all();
            this->publishTag({ { gr::tag::END_OF_STREAM, true } }, 0);
            return { 0, 0, DONE };
        }

#ifndef GR_PICOSCOPE_POLLER_THREAD
        if (ps_settings.acquisition_mode == AcquisitionMode::Streaming) {
            if (const auto ec = self().driver_poll()) {
                // TODO tolerate or return ERROR
                reportError(ec);
                fmt::println(std::cerr, "poll failed");
            }
        }
#endif

        return { 0, 0, OK };
    }

    // TODO only for debugging, maybe remove
    std::size_t
    producedWorker() const {
        return ps_state.produced_worker;
    }

    void
    start() noexcept {
        if (ps_state.started) {
            return;
        }

        try {
            initialize();
            configure();
            if (ps_settings.auto_arm) {
                arm();
            }
            if (ps_settings.acquisition_mode == AcquisitionMode::Streaming) {
                startPollThread();
            }
            ps_state.started = true;
        } catch (const std::exception &e) {
            fmt::println(std::cerr, "{}", e.what());
        }
    }

    void
    stop() noexcept {
        if (!ps_state.started) {
            return;
        }

        ps_state.started = false;

        if (!ps_state.initialized) {
            return;
        }

        disarm();
        close();

        if (ps_settings.acquisition_mode == AcquisitionMode::Streaming) {
            stopPollThread();
        }
    }

    void
    startPollThread() {
        if (ps_state.poller.joinable()) {
            return;
        }

        if (ps_state.poller_state == detail::PollerState::Exit) {
            ps_state.poller_state = detail::PollerState::Idle;
        }
#ifdef GR_PICOSCOPE_POLLER_THREAD
        const auto pollDuration = std::chrono::seconds(1) * ps_settings.streaming_mode_poll_rate;

        state.poller            = std::thread([this, pollDuration] {
            while (state.poller_state != detail::poller_state_t::Exit) {
                if (state.poller_state == detail::poller_state_t::Idle) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }

                const auto pollStart = std::chrono::high_resolution_clock::now();

                if (const auto ec = self().driver_poll()) {
                    fmt::println(std::cerr, "poll failed: {}", ec.message());
                }
                // Substract the time each iteration itself took in order to get closer to
                // the desired poll duration
                const auto elapsedPollDuration = std::chrono::high_resolution_clock::now() - pollStart;
                if (elapsedPollDurationuration < pollDuration) {
                    std::this_thread::sleep_for(pollDuration - elapsedPollDuration);
                }
            }
        });
#endif
    }

    void
    stopPollThread() {
        ps_state.poller_state = detail::PollerState::Exit;
        if (ps_state.poller.joinable()) {
            ps_state.poller.join();
        }
    }

    std::string
    driverVersion() const {
        return self().driver_driverVersion();
    }

    std::string
    hardwareVersion() const {
        return self().driver_hardwareVersion();
    }

    void
    initialize() {
        if (ps_state.initialized) {
            return;
        }

        if (const auto ec = self().driver_initialize()) {
            throw std::runtime_error(fmt::format("Initialization failed: {}", ec.message()));
        }

        ps_state.initialized = true;
    }

    void
    close() {
        self().driver_close();
        ps_state.closed      = true;
        ps_state.initialized = false;
        ps_state.configured  = false;
    }

    void
    configure() {
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

    void
    arm() {
        if (ps_state.armed) {
            return;
        }

        if (const auto ec = self().driver_arm()) {
            throw std::runtime_error(fmt::format("Arming failed: {}", ec.message()));
        }

        ps_state.armed = true;
        if (ps_settings.acquisition_mode == AcquisitionMode::Streaming) {
            ps_state.poller_state = detail::PollerState::Running;
        }
    }

    void
    disarm() noexcept {
        if (!ps_state.armed) {
            return;
        }

        if (ps_settings.acquisition_mode == AcquisitionMode::Streaming) {
            ps_state.poller_state = detail::PollerState::Idle;
        }

        if (const auto ec = self().driver_disarm()) {
            fmt::println(std::cerr, "disarm failed: {}", ec.message());
        }

        ps_state.armed = false;
    }

    void
    processDriverData(std::size_t nrSamples, std::size_t offset) {
        std::vector<std::size_t> triggerOffsets;

        using ChannelOutputRange = decltype(ps_state.channels[0].data_writer.reserve_output_range(1));
        std::vector<ChannelOutputRange> channelOutputs;
        channelOutputs.reserve(ps_state.channels.size());

        for (std::size_t channelIdx = 0; channelIdx < ps_state.channels.size(); ++channelIdx) {
            auto &channel = ps_state.channels[channelIdx];

            channelOutputs.push_back(channel.data_writer.reserve_output_range(nrSamples));
            auto      &output     = channelOutputs[channelIdx];

            const auto driverData = std::span(channel.driver_buffer).subspan(offset, nrSamples);

            if constexpr (std::is_same_v<T, int16_t>) {
                std::copy(driverData.begin(), driverData.end(), output.begin());
            } else {
                const auto voltageMultiplier = static_cast<T>(channel.settings.range / ps_state.max_value);
                // TODO use SIMD
                for (std::size_t i = 0; i < nrSamples; ++i) {
                    output[i] = voltageMultiplier * driverData[i];
                }
            }

            if (channel.id == ps_settings.trigger.source) {
                triggerOffsets = findAnalogTriggers(channel, output);
            }
        }

        const auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());

        // TODO wait (block) here for timing messages if trigger count > timing message count
        // TODO pair up trigger offsets with timing messages

        std::vector<gr::Tag> triggerTags;
        triggerTags.reserve(triggerOffsets.size());

        for (const auto triggerOffset : triggerOffsets) {
            // raw index is index - 1
            triggerTags.emplace_back(static_cast<int64_t>(ps_state.produced_worker + triggerOffset - 1), gr::property_map{ // TODO use data from timing message
                                                                                                                           { gr::tag::TRIGGER_NAME, "PPS" },
                                                                                                                           { gr::tag::TRIGGER_TIME, static_cast<uint64_t>(now.count()) } });
        }

        for (auto &channel : ps_state.channels) {
            const auto writeSignalInfo = !channel.signal_info_written;
            const auto tagsToWrite     = triggerTags.size() + (writeSignalInfo ? 1 : 0);
            if (tagsToWrite == 0) {
                continue;
            }
            auto writeTags = channel.tag_writer.reserve_output_range(tagsToWrite);
            if (writeSignalInfo) {
                // raw index is index - 1
                writeTags[0].index            = static_cast<int64_t>(ps_state.produced_worker - 1);
                writeTags[0].map              = channel.signalInfo();
                static const auto kSampleRate = std::string(gr::tag::SAMPLE_RATE.key());
                writeTags[0].map[kSampleRate] = static_cast<float>(sample_rate);
                channel.signal_info_written   = true;
            }
            std::copy(triggerTags.begin(), triggerTags.end(), writeTags.begin() + (writeSignalInfo ? 1 : 0));
            writeTags.publish(writeTags.size());
        }

        // once all tags have been written, publish the data
        for (auto &output : channelOutputs) {
            output.publish(nrSamples);
        }

        ps_state.produced_worker += nrSamples;
    }

    void
    streamingCallback(int32_t nrSamplesSigned, uint32_t startIndex, int16_t overflow) {
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

        processDriverData(nrSamples, startIndex);
    }

    void
    rapidBlockCallback(Error ec) {
        if (ec) {
            reportError(ec);
            return;
        }
        const auto samples = ps_settings.pre_samples + ps_settings.post_samples;
        for (std::size_t capture = 0; capture < ps_settings.rapid_block_nr_captures; ++capture) {
            const auto getValuesResult = self().driver_rapidBlockGetValues(capture, samples);
            if (getValuesResult.error) {
                reportError(ec);
                return;
            }

            // TODO handle overflow for individual channels?
            processDriverData(getValuesResult.samples, 0);
        }

        if (ps_settings.trigger_once) {
            ps_state.data_finished = true;
        }
    }

    std::vector<std::size_t>
    findAnalogTriggers(const detail::Channel<T> &triggerChannel, std::span<const T> samples) {
        if (samples.empty()) {
            return {};
        }

        std::vector<std::size_t> triggerOffsets; // relative offset of detected triggers
        const auto               band              = triggerChannel.settings.range / 100.;
        const auto               voltageMultiplier = triggerChannel.settings.range / ps_state.max_value;

        const auto               toDouble          = [&voltageMultiplier](T raw) {
            if constexpr (std::is_same_v<T, float>) {
                return static_cast<double>(raw);
            } else if constexpr (std::is_same_v<T, double>) {
                return raw;
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

    void
    reportError(Error ec) {
        auto out = ps_state.errors.writer.reserve_output_range(1);
        out[0]   = { ps_state.produced_worker, ec };
        out.publish(1);
    }

    [[nodiscard]] constexpr auto &
    self() noexcept {
        return *static_cast<TPSImpl *>(this);
    }

    [[nodiscard]] constexpr const auto &
    self() const noexcept {
        return *static_cast<const TPSImpl *>(this);
    }
};

} // namespace fair::picoscope

#endif
