#ifndef FAIR_PICOSCOPE_PICOSCOPE_HPP
#define FAIR_PICOSCOPE_PICOSCOPE_HPP

#include "status_messages.hpp"

#include <node.hpp>

#include <fmt/format.h>

#include <functional>

// #define GR_PICOSCOPE_POLLER_THREAD 1

namespace fair::picoscope {

namespace detail {

using streaming_callback_function_t = std::function<void(int32_t, uint32_t, int16_t)>;
/*!
 * \brief The state of the poller worker function.
 */
enum class poller_state_t { IDLE = 0, RUNNING, EXIT };

inline void
invoke_streaming_callback(int16_t handle, int32_t noOfSamples, uint32_t startIndex, int16_t overflow, uint32_t triggerAt, int16_t triggered, int16_t autoStop, void *parameter) {
    std::ignore = handle;
    std::ignore = triggerAt;
    std::ignore = triggered;
    std::ignore = autoStop;
    (*static_cast<streaming_callback_function_t *>(parameter))(noOfSamples, startIndex, overflow);
}

} // namespace detail

enum class acquisition_mode_t { STREAMING, RAPID_BLOCK };

enum class coupling_t {
    DC_1M,  ///< DC, 1 MOhm
    AC_1M,  ///< AC, 1 MOhm
    DC_50R, ///< DC, 50 Ohm
};

enum class trigger_direction_t { RISING, FALLING, LOW, HIGH };

struct GetValuesResult {
    std::error_code error;
    std::size_t     samples;
    int16_t         overflow;
};

namespace detail {

constexpr std::size_t driver_buffer_size = 65536;

struct channel_setting_t {
    std::string name;
    std::string unit     = "V";
    double      range    = 2.;
    float       offset   = 0.;
    coupling_t  coupling = coupling_t::AC_1M;
};

using ChannelMap = std::map<std::string, channel_setting_t, std::less<>>;

struct trigger_setting_t {
    static constexpr std::string_view TRIGGER_DIGITAL_SOURCE = "DI"; // DI is as well used as "AUX" for p6000 scopes

    bool
    is_enabled() const {
        return !source.empty();
    }

    bool
    is_digital() const {
        return is_enabled() && source == TRIGGER_DIGITAL_SOURCE;
    }

    bool
    is_analog() const {
        return is_enabled() && source != TRIGGER_DIGITAL_SOURCE;
    }

    std::string         source;
    float               threshold  = 0; // AI only
    trigger_direction_t direction  = trigger_direction_t::RISING;
    int                 pin_number = 0; // DI only
};

struct Channel {
    using WriterType    = decltype(std::declval<gr::circular_buffer<float>>().new_writer());
    using TagWriterType = decltype(std::declval<gr::circular_buffer<fair::graph::tag_t>>().new_writer());
    std::string          id;
    channel_setting_t    settings;
    std::vector<int16_t> driver_buffer;
    WriterType           data_writer;
    TagWriterType        tag_writer;
    WriterType           error_writer;
    bool                 signal_info_written = false;

    fair::graph::property_map
    signal_info() const {
        using namespace fair::graph;
        static const auto SIGNAL_NAME = std::string(tag::SIGNAL_NAME.key());
        static const auto SIGNAL_UNIT = std::string(tag::SIGNAL_UNIT.key());
        static const auto SIGNAL_MIN  = std::string(tag::SIGNAL_MIN.key());
        static const auto SIGNAL_MAX  = std::string(tag::SIGNAL_MAX.key());
        return { { SIGNAL_NAME, settings.name }, { SIGNAL_UNIT, settings.unit }, { SIGNAL_MIN, settings.offset }, { SIGNAL_MAX, settings.offset + static_cast<float>(settings.range) } };
    }
};

struct Error {
    std::size_t     sample;
    std::error_code error;
};

template<typename T, std::size_t InitialSize = 1>
struct BufferHelper {
    gr::circular_buffer<T>        buffer = gr::circular_buffer<T>(InitialSize);
    decltype(buffer.new_reader()) reader = buffer.new_reader();
    decltype(buffer.new_writer()) writer = buffer.new_writer();
};

struct Settings {
    std::string        serial_number;
    double             sample_rate              = 10000.;
    acquisition_mode_t acquisition_mode         = acquisition_mode_t::STREAMING;
    std::size_t        pre_samples              = 1000;
    std::size_t        post_samples             = 9000;
    std::size_t        rapid_block_nr_captures  = 1;
    bool               trigger_once             = false;
    double             streaming_mode_poll_rate = 0.001;
    bool               auto_arm                 = true;
    ChannelMap         enabled_channels;
    trigger_setting_t  trigger;
};

struct State {
    std::vector<Channel>        channels;
    std::atomic<std::size_t>    data_available     = 0;
    std::atomic<bool>           data_finished      = false;
    bool                        initialized        = false;
    bool                        configured         = false;
    bool                        closed             = false;
    bool                        armed              = false;
    bool                        started            = false; // TODO transitional until nodes have state handling
    int16_t                     handle             = -1;    ///< picoscope handle
    int16_t                     overflow           = 0;     ///< status returned from getValues
    int16_t                     max_value          = 0;     ///< maximum ADC count used for ADC conversion
    double                      actual_sample_rate = 0;
    std::thread                 poller;
    BufferHelper<Error>         errors;
    std::atomic<poller_state_t> poller_state = poller_state_t::IDLE;
    std::atomic<bool>           forced_quit  = false; // TODO transitional until we found out what
                                                      // goes wrong with multithreaded scheduler
    std::size_t produced_worker = 0;                  // poller/callback thread
};

// TODO replace by std::ranges::views::split once that works on all supported compilers
inline std::vector<std::string_view>
split(std::string_view s) {
    std::vector<std::string_view> segments;
    while (true) {
        const auto pos = s.find(",");
        if (pos == std::string_view::npos) {
            segments.push_back(s);
            return segments;
        }

        segments.push_back(s.substr(0, pos));
        s.remove_prefix(pos + 1);
    }
}

inline acquisition_mode_t
parse_acquisition_mode(std::string_view s) {
    using enum acquisition_mode_t;
    if (s == "RAPID_BLOCK") return RAPID_BLOCK;
    if (s == "STREAMING") return STREAMING;
    throw std::invalid_argument(fmt::format("Unknown acquisition mode '{}'", s));
}

inline coupling_t
parse_coupling(std::string_view s) {
    using enum coupling_t;
    if (s == "DC_1M") return DC_1M;
    if (s == "AC_1M") return AC_1M;
    if (s == "DC_50R") return DC_50R;
    throw std::invalid_argument(fmt::format("Unknown coupling type '{}'", s));
}

inline trigger_direction_t
parse_trigger_direction(std::string_view s) {
    using enum trigger_direction_t;
    if (s == "RISING") return RISING;
    if (s == "FALLING") return FALLING;
    if (s == "LOW") return LOW;
    if (s == "HIGH") return HIGH;
    throw std::invalid_argument(fmt::format("Unknown trigger direction '{}'", s));
}

inline ChannelMap
channel_settings(std::span<const std::string_view> ids, std::span<const std::string_view> names, std::span<const std::string_view> units, std::span<const double> ranges,
                 std::span<const float> offsets, std::span<const std::string_view> couplings) {
    ChannelMap r;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        channel_setting_t channel;
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
            channel.coupling = parse_coupling(couplings[i]);
        }

        r[std::string(ids[i])] = channel;
    }
    return r;
}

} // namespace detail

// optional shortening
template<typename T, fair::meta::fixed_string description = "", typename... Arguments>
using A = fair::graph::Annotated<T, description, Arguments...>;

using fair::graph::Visible;

template<typename PSImpl>
struct Picoscope : public fair::graph::node<PSImpl, fair::graph::BlockingIO<true>> {
    A<std::string, "serial number">   serial_number;
    A<double, "sample rate", Visible> sample_rate = 10000.;
    // TODO any way to get custom enums into pmtv??
    A<std::string, "acquisition mode", Visible>                           acquisition_mode         = std::string("STREAMING");
    A<std::size_t, "pre-samples">                                         pre_samples              = 1000;
    A<std::size_t, "post-samples">                                        post_samples             = 9000;
    A<std::size_t, "no. captures (rapid block mode)">                     rapid_block_nr_captures  = 1;
    A<bool, "trigger once (rapid block mode)">                            trigger_once             = false;
    A<double, "poll rate (streaming mode)">                               streaming_mode_poll_rate = 0.001;
    A<bool, "auto-arm">                                                   auto_arm                 = true;
    A<std::string, "IDs of enabled channels, comma-separated">            channel_ids;
    A<std::string, "Names of enabled channels, comma-separated">          channel_names;
    A<std::string, "Units of enabled channels, comma-separated">          channel_units;
    std::vector<double>                                                   channel_ranges;
    std::vector<float>                                                    channel_offsets;
    A<std::string, "Coupling modes of enabled channels, comma-separated"> channel_couplings;
    A<std::string, "trigger channel/port ID">                             trigger_source;
    A<float, "trigger threshold, analog only">                            trigger_threshold = 0.f;
    A<std::string, "trigger direction">                                   trigger_direction = std::string("RISING");
    A<int, "trigger pin, digital only">                                   trigger_pin       = 0;

    detail::State                                                         state;
    detail::Settings                                                      ps_settings;

    detail::streaming_callback_function_t _streaming_callback = [this](int32_t noSamples, uint32_t startIndex, int16_t overflow) { streaming_callback(noSamples, startIndex, overflow); };

    ~Picoscope() { stop(); }

    void
    settings_changed(const fair::graph::property_map & /*old_settings*/, const fair::graph::property_map & /*new_settings*/) {
        const auto was_started = state.started;
        if (was_started) {
            stop();
        }
        try {
            auto s = detail::Settings{ .serial_number            = serial_number,
                                       .sample_rate              = sample_rate,
                                       .acquisition_mode         = detail::parse_acquisition_mode(acquisition_mode),
                                       .pre_samples              = pre_samples,
                                       .post_samples             = post_samples,
                                       .rapid_block_nr_captures  = rapid_block_nr_captures,
                                       .trigger_once             = trigger_once,
                                       .streaming_mode_poll_rate = streaming_mode_poll_rate,
                                       .auto_arm                 = auto_arm,
                                       .enabled_channels         = detail::channel_settings(detail::split(channel_ids), detail::split(channel_names), detail::split(channel_units), channel_ranges,
                                                                                            channel_offsets, detail::split(channel_couplings)),
                                       .trigger                  = detail::trigger_setting_t{
                                                                .source = trigger_source, .threshold = trigger_threshold, .direction = detail::parse_trigger_direction(trigger_direction), .pin_number = trigger_pin } };
            std::swap(ps_settings, s);

            state.channels.reserve(ps_settings.enabled_channels.size());
            std::size_t channel_idx = 0;
            for (const auto &[id, settings] : ps_settings.enabled_channels) {
                state.channels.emplace_back(detail::Channel{ .id            = id,
                                                             .settings      = settings,
                                                             .driver_buffer = std::vector<int16_t>(detail::driver_buffer_size),
                                                             .data_writer   = self().values[channel_idx].streamWriter().buffer().new_writer(),
                                                             .tag_writer    = self().values[channel_idx].tagWriter().buffer().new_writer(),
                                                             .error_writer  = self().errors[channel_idx].streamWriter().buffer().new_writer() });
                channel_idx++;
            }
        } catch (const std::exception &e) {
            // TODO add errors properly
            fmt::println(std::cerr, "Could not apply settings: {}", e.what());
        }
        if (was_started) {
            start();
        }
    }

    fair::graph::work_return_t
    work_impl() noexcept {
        using enum fair::graph::work_return_status_t;
        start(); // TODO should be done by scheduler

        if (state.channels.empty()) {
            return { 0, 0, ERROR };
        }

        if (const auto errors_available = state.errors.reader.available(); errors_available > 0) {
            auto errors = state.errors.reader.get(errors_available);
            std::ignore = state.errors.reader.consume(errors_available);
            return { 0, 0, ERROR };
        }

        if (state.forced_quit) {
            return { 0, 0, DONE };
        }

        if (state.data_finished) {
            return { 0, 0, DONE };
        }

#ifndef GR_PICOSCOPE_POLLER_THREAD
        if (ps_settings.acquisition_mode == acquisition_mode_t::STREAMING) {
            if (auto ec = self().driver_poll()) {
                // TODO tolerate or return ERROR
                report_error(ec);
                fmt::println(std::cerr, "poll failed");
            }
        }
#endif

        return { 0, 0, OK };
    }

    // TODO only for debugging, maybe remove
    std::size_t
    produced_worker() const {
        return state.produced_worker;
    }

    void
    force_quit() {
        state.forced_quit = true;
    }

    void
    start() noexcept {
        if (state.started) {
            return;
        }

        try {
            initialize();
            configure();
            if (ps_settings.auto_arm) {
                arm();
            }
            if (ps_settings.acquisition_mode == acquisition_mode_t::STREAMING) {
                start_poll_thread();
            }
            state.started = true;
        } catch (const std::exception &e) {
            fmt::println(std::cerr, "{}", e.what());
        }
    }

    void
    stop() noexcept {
        if (!state.started) {
            return;
        }

        state.started = false;

        if (!state.initialized) {
            return;
        }

        disarm();
        close();

        if (ps_settings.acquisition_mode == acquisition_mode_t::STREAMING) {
            stop_poll_thread();
        }
    }

    void
    start_poll_thread() {
        if (state.poller.joinable()) {
            return;
        }
        const auto poll_duration = std::chrono::seconds(1) * ps_settings.streaming_mode_poll_rate;

        if (state.poller_state == detail::poller_state_t::EXIT) {
            state.poller_state = detail::poller_state_t::IDLE;
        }
#ifdef GR_PICOSCOPE_POLLER_THREAD
        state.poller = std::thread([this, poll_duration] {
            while (state.poller_state != detail::poller_state_t::EXIT) {
                if (state.poller_state == detail::poller_state_t::IDLE) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }

                const auto poll_start = std::chrono::high_resolution_clock::now();

                auto       ec         = self().driver_poll();
                if (ec) {
                    fmt::println(std::cerr, "poll failed");
                }
                // Substract the time each iteration itself took in order to get closer to
                // the desired poll duration
                const auto elapsed_poll_duration = std::chrono::high_resolution_clock::now() - poll_start;
                if (elapsed_poll_duration < poll_duration) {
                    std::this_thread::sleep_for(poll_duration - elapsed_poll_duration);
                }
            }
        });
#endif
    }

    void
    stop_poll_thread() {
        state.poller_state = detail::poller_state_t::EXIT;
        if (state.poller.joinable()) {
            state.poller.join();
        }
    }

    std::string
    driver_version() const {
        return self().driver_driver_version();
    }

    std::string
    hardware_version() const {
        return self().driver_hardware_version();
    }

    void
    initialize() {
        if (state.initialized) {
            return;
        }
        const auto ec = self().driver_initialize();
        if (ec) {
            throw std::runtime_error(fmt::format("Initialization failed"));
        }

        state.initialized = true;
    }

    void
    close() {
        self().driver_close();
        state.closed      = true;
        state.initialized = false;
        state.configured  = false;
    }

    void
    configure() {
        if (!state.initialized) {
            throw std::runtime_error("Cannot configure without initialization");
        }

        if (state.armed) {
            throw std::runtime_error("Cannot configure armed device");
        }

        auto ec = self().driver_configure();
        if (ec) {
            throw std::runtime_error("Configuration failed");
        }

        state.configured = true;
    }

    void
    arm() {
        if (state.armed) {
            return;
        }

        // arm the driver
        auto ec = self().driver_arm();
        if (ec) {
            throw std::runtime_error("Arming failed");
        }

        state.armed = true;
        if (ps_settings.acquisition_mode == acquisition_mode_t::STREAMING) {
            state.poller_state = detail::poller_state_t::RUNNING;
        }
    }

    void
    disarm() noexcept {
        if (!state.armed) {
            return;
        }

        if (ps_settings.acquisition_mode == acquisition_mode_t::STREAMING) {
            state.poller_state = detail::poller_state_t::IDLE;
        }

        const auto ec = self().driver_disarm();

        if (ec) {
            fmt::println(std::cerr, "disarm failed");
        }

        state.armed = false;
    }

    void
    process_driver_data(std::size_t nr_samples, std::size_t offset) {
        for (auto &channel : state.channels) {
            const auto error_estimate     = channel.settings.range * static_cast<double>(PSImpl::DRIVER_VERTICAL_PRECISION);

            const auto voltage_multiplier = static_cast<float>(channel.settings.range / state.max_value);

            auto       write_values       = channel.data_writer.reserve_output_range(nr_samples);
            // TODO use SIMD
            for (std::size_t i = 0; i < nr_samples; ++i) {
                write_values[i] = voltage_multiplier * channel.driver_buffer[offset + i];
            }
            auto write_errors = channel.error_writer.reserve_output_range(nr_samples);
            std::fill(write_errors.begin(), write_errors.end(), error_estimate);

            if (!channel.signal_info_written) {
                auto write_tag                = channel.tag_writer.reserve_output_range(1);
                write_tag[0].index            = static_cast<int64_t>(state.produced_worker);
                write_tag[0].map              = channel.signal_info();
                static const auto SAMPLE_RATE = std::string(fair::graph::tag::SAMPLE_RATE.key());
                write_tag[0].map[SAMPLE_RATE] = static_cast<float>(sample_rate);
                write_tag.publish(1);
                channel.signal_info_written = true;
            }

            write_values.publish(nr_samples);
            write_errors.publish(nr_samples);
        }

        state.data_available += nr_samples;
        state.produced_worker += nr_samples;
    }

    void
    streaming_callback(int32_t nr_samples_signed, uint32_t start_index, int16_t overflow) {
        assert(nr_samples_signed >= 0);
        const auto nr_samples = static_cast<std::size_t>(nr_samples_signed);

        // According to well informed sources, the driver indicates the buffer overrun by
        // setting all the bits of the overflow argument to true.
        if (static_cast<uint16_t>(overflow) == 0xffff) {
            fmt::println(std::cerr, "Buffer overrun detected, continue...");
        }

        if (nr_samples == 0) {
            return;
        }

        process_driver_data(nr_samples, start_index);
    }

    void
    rapid_block_callback(std::error_code ec) {
        if (ec) {
            report_error(ec);
            return;
        }
        const auto samples = ps_settings.pre_samples + ps_settings.post_samples;
        for (std::size_t capture = 0; capture < ps_settings.rapid_block_nr_captures; ++capture) {
            const auto get_values_result = self().driver_rapid_block_get_values(capture, samples);
            if (get_values_result.error) {
                report_error(ec);
                return;
            }

            // TODO handle overflow for individual channels?
            process_driver_data(get_values_result.samples, 0);
        }

        if (ps_settings.trigger_once) {
            state.data_finished = true;
        }
    }

    void
    report_error(std::error_code ec) {
        auto out = state.errors.writer.reserve_output_range(1);
        out[0]   = { state.produced_worker, ec };
        out.publish(1);
    }

    [[nodiscard]] constexpr auto &
    self() noexcept {
        return *static_cast<PSImpl *>(this);
    }

    [[nodiscard]] constexpr const auto &
    self() const noexcept {
        return *static_cast<const PSImpl *>(this);
    }
};

} // namespace fair::picoscope

#endif
