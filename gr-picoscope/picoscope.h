#include <node.hpp>

#include <volk/volk.h>

#include <fmt/format.h>

#include <functional>

namespace gr::picoscope {

namespace detail {

using streaming_callback_function_t = std::function<void(int32_t, uint32_t, int16_t)>;
/*!
 * \brief The state of the poller worker function.
 */
enum class poller_state_t { IDLE = 0, RUNNING, EXIT };

inline void invoke_streaming_callback(int16_t handle,
                                      int32_t noOfSamples,
                                      uint32_t startIndex,
                                      int16_t overflow,
                                      uint32_t triggerAt,
                                      int16_t triggered,
                                      int16_t autoStop,
                                      void* parameter)
{
    std::ignore = handle;
    std::ignore = triggerAt;
    std::ignore = triggered;
    std::ignore = autoStop;
    (*static_cast<streaming_callback_function_t*>(parameter))(
        noOfSamples, startIndex, overflow);
}

} // namespace detail

enum class acquisition_mode_t { STREAMING, RAPID_BLOCK };

/**
 * An enum representing coupling mode
 */
enum class coupling_t {
    DC_1M = 0,  ///< DC, 1 MOhm
    AC_1M = 1,  ///< AC, 1 MOhm
    DC_50R = 2, ///< DC, 50 Ohm
};

/*!
 * \brief Specifies a trigger mechanism
 * \ingroup digitizers
 */
enum class trigger_direction_t { Rising, Falling, Low, High };

struct trigger_setting_t {
    static constexpr std::string_view TRIGGER_DIGITAL_SOURCE =
        "DI"; // DI is as well used as "AUX" for p6000 scopes

    bool is_enabled() const { return !source.empty(); }

    bool is_digital() const { return is_enabled() && source == TRIGGER_DIGITAL_SOURCE; }

    bool is_analog() const { return is_enabled() && source != TRIGGER_DIGITAL_SOURCE; }

    std::string source;
    float threshold = 0; // AI only
    trigger_direction_t direction = trigger_direction_t::Rising;
    int pin_number = 0; // DI only
};

struct channel_setting_t {
    double range = 2.;
    float offset = 0.;
    coupling_t coupling = coupling_t::AC_1M;
};

struct port_setting_t {
    float logic_level = 1.5;
    bool enabled = false;
};

struct GetValuesResult {
    std::error_code error;
    std::size_t samples;
    int16_t overflow;
};

namespace detail {

template <typename T, std::size_t InitialSize = 1>
struct BufferHelper {
    gr::circular_buffer<T> buffer = gr::circular_buffer<T>(InitialSize);
    decltype(buffer.new_reader()) reader = buffer.new_reader();
    decltype(buffer.new_writer()) writer = buffer.new_writer();
};

struct Channel {
    std::string id;
    channel_setting_t settings;
    std::vector<int16_t> driver_buffer;
    BufferHelper<float> data;
};

struct Error {
    std::size_t sample;
    std::error_code error;
};

struct State {
    std::vector<Channel> channels;
    std::atomic<std::size_t> data_available = 0;
    std::atomic<bool> data_finished = false;
    bool initialized = false;
    bool configured = false;
    bool closed = false;
    bool armed = false;
    bool started = false;  // TODO transitional until nodes have state handling
    int16_t handle = -1;   ///< picoscope handle
    int16_t overflow = 0;  ///< status returned from getValues
    int16_t max_value = 0; ///< maximum ADC count used for ADC conversion
    double actual_sample_rate = 0;
    std::atomic<std::size_t> lost_count = 0;
    std::thread poller;
    BufferHelper<Error> errors;
    std::atomic<poller_state_t> poller_state = poller_state_t::IDLE;
    std::atomic<bool> forced_quit = false; // TODO transitional until we found out what
                                           // goes wrong with multithreaded scheduler
    std::size_t produced_worker = 0;       // poller/callback thread
    std::size_t produced_block = 0;        // "main" thread
};

} // namespace detail

using fair::graph::PublishableSpan;

template <PublishableSpan T>
struct ChannelPair {
    T& values;
    T& errors;
};

// optional shortening
template <typename T, fair::meta::fixed_string description = "", typename... Arguments>
using A = fair::graph::Annotated<T, description, Arguments...>;

using fair::graph::Visible;

template <typename PSImpl>
struct Picoscope : public fair::graph::node<PSImpl> {
    using ChannelMap = std::map<std::string, channel_setting_t, std::less<>>;
    using PortMap = std::map<std::string, port_setting_t, std::less<>>;

    A<std::string, "serial number", Visible> serial_number;
    A<double, "sample rate", Visible> sample_rate = 10000.;
    A<acquisition_mode_t, "acquisition mode", Visible> acquisition_mode =
        acquisition_mode_t::STREAMING;
    // TODO any way to get custom enums into pmtv??
    A<std::string, "acquisition mode as string", Visible> acquisition_mode_string =
        std::string("STREAMING");
    A<std::size_t, "pre-samples", Visible> pre_samples = 1000;
    A<std::size_t, "post-samples", Visible> post_samples = 9000;
    A<std::size_t, "no. captures (rapid block mode)", Visible> rapid_block_nr_captures =
        1;
    A<bool, "trigger once (rapid block mode)", Visible> trigger_once = false;
    A<double, "poll rate (streaming mode)", Visible> streaming_mode_poll_rate = 0.001;
    A<std::size_t, "driver buffer size", Visible> driver_buffer_size = 100000;
    A<bool, "auto-arm", Visible> auto_arm = true;
    A<ChannelMap, "enabled analog channels", Visible> enabled_channels;
    A<PortMap, "enabled digital ports", Visible> enabled_ports;
    A<trigger_setting_t, "trigger settings", Visible> trigger;

    detail::State state;
    detail::streaming_callback_function_t _streaming_callback;

    explicit Picoscope()
        : _streaming_callback(
              [this](int32_t noSamples, uint32_t startIndex, int16_t overflow) {
                  streaming_callback(noSamples, startIndex, overflow);
              })
    {
#if 0
        if (ps_settings.acquisition_mode == acquisition_mode_t::RAPID_BLOCK) {
            if (ps_settings.post_samples == 0) {
                throw std::invalid_argument("Post-trigger samples cannot be zero");
            }
            if (ps_settings.rapid_block_nr_captures == 0) {
                throw std::invalid_argument("Number of captures cannot be zero");
            }
        }
        else { // streaming mode
            if (ps_settings.streaming_mode_poll_rate <= 0.0) {
                throw std::invalid_argument("Poll rate must be greater than zero");
            }
        }
        if (ps_settings.sample_rate <= 0) {
            throw std::invalid_argument("Sample rate has to be greater than zero");
        }
        if (ps_settings.driver_buffer_size == 0) {
            throw std::invalid_argument("Driver buffer size cannot be zero");
        }

        state.actual_sample_rate = ps_settings.sample_rate;
#endif
    }

    ~Picoscope() { stop(); }

    // TODO this should be part of the settings, but how to pass it through property_map?
    void set_channel_configuration(ChannelMap channels,
                                   PortMap ports = {},
                                   trigger_setting_t trigger_ = {})
    {
        const auto was_started = state.started;
        if (was_started) {
            stop();
        }

        // TODO maybe also check (at the end of configure()) that the configured channels
        // actually exist on the given device
        for (const auto& [id, _] : channels) {
            if (!PSImpl::driver_channel_id_to_index(id)) {
                throw std::invalid_argument(fmt::format("Unknown channel '{}'", id));
            }
        }

        if (trigger_.is_enabled()) {
            if (!PSImpl::driver_channel_id_to_index(trigger_.source)) {
                throw std::invalid_argument(
                    fmt::format("Unknown trigger channel '{}'", trigger_.source));
            }
        }

        enabled_channels = std::move(channels);
        enabled_ports = std::move(ports);
        trigger = std::move(trigger_);

        state.channels.resize(enabled_channels.value.size());
        std::size_t channel_idx = 0;
        for (const auto& [id, settings] : enabled_channels.value) {
            auto& ch = state.channels[channel_idx];
            ch.id = id;
            ch.settings = settings;
            ch.data.buffer = gr::circular_buffer<float>(
                driver_buffer_size); // TODO think about what a sensible
                                     // buffer size would be
            ch.data.reader = ch.data.buffer.new_reader();
            ch.data.writer = ch.data.buffer.new_writer();
            channel_idx++;
        }

        if (was_started) {
            start();
        }
    }

    void settings_changed(const fair::graph::property_map& /*old_settings*/,
                          const fair::graph::property_map& /*new_settings*/)
    {
        acquisition_mode = acquisition_mode_string == "STREAMING"
                               ? acquisition_mode_t::STREAMING
                               : acquisition_mode_t::RAPID_BLOCK;
        if (state.started) {
            stop();
            start();
        }
    }

    template <PublishableSpan AnalogSpan, std::size_t ChannelCount>
    inline fair::graph::work_return_status_t
    process_bulk_impl(const std::array<ChannelPair<AnalogSpan>, ChannelCount>&
                          channel_outputs) noexcept
    {
        for (std::size_t channel_index = 0; channel_index < ChannelCount;
             ++channel_index) {
            channel_outputs[channel_index].values.publish(0);
            channel_outputs[channel_index].errors.publish(0);
        }

        if (state.channels.empty()) {
            return fair::graph::work_return_status_t::ERROR;
        }
        if (state.forced_quit) {
            return fair::graph::work_return_status_t::DONE;
        }

        if (state.data_finished && state.data_available == 0) {
            return fair::graph::work_return_status_t::DONE;
        }

        assert(state.channels.size() <= ChannelCount);

        auto noutput_items = state.data_available.load();

        if (noutput_items == 0) {
            return fair::graph::work_return_status_t::OK;
        }

        for (std::size_t channel_index = 0; channel_index < state.channels.size();
             ++channel_index) {
            noutput_items = std::min({ noutput_items,
                                       channel_outputs[channel_index].values.size(),
                                       channel_outputs[channel_index].errors.size() });
        }

        if (noutput_items == 0) {
            return fair::graph::work_return_status_t::INSUFFICIENT_OUTPUT_ITEMS;
        }

        for (std::size_t channel_index = 0; channel_index < state.channels.size();
             ++channel_index) {
            auto& channel = state.channels[channel_index];
            auto& values = channel_outputs[channel_index].values;
            auto& errors = channel_outputs[channel_index].errors;

            const auto data = channel.data.reader.get(noutput_items);
            std::ignore = std::copy(data.begin(), data.end(), values.begin());
            const auto error_estimate =
                channel.settings.range *
                static_cast<double>(PSImpl::DRIVER_VERTICAL_PRECISION);
            std::fill(errors.begin(), errors.end(), error_estimate);
            std::ignore = channel.data.reader.consume(data.size());
            values.publish(noutput_items);
            errors.publish(noutput_items);
            ;
        }

        state.data_available -= noutput_items;
        state.produced_block += noutput_items;
        return fair::graph::work_return_status_t::OK;
    }

    void force_quit() { state.forced_quit = true; }
    void start() noexcept
    {
        if (state.started) {
            return;
        }

        try {
            initialize();
            configure();
            if (auto_arm) {
                arm();
            }
            if (acquisition_mode == acquisition_mode_t::STREAMING) {
                start_poll_thread();
            }
            state.started = true;
        } catch (const std::exception& e) {
            fmt::println(std::cerr, "{}", e.what());
        }
    }

    void stop() noexcept
    {
        if (!state.started) {
            return;
        }

        state.started = false;

        if (!state.initialized) {
            return;
        }

        disarm();
        close();

        if (acquisition_mode == acquisition_mode_t::STREAMING) {
            stop_poll_thread();
        }
    }

    void start_poll_thread()
    {
        if (state.poller.joinable()) {
            return;
        }
        const auto poll_duration =
            std::chrono::seconds(1) * streaming_mode_poll_rate.value;

        if (state.poller_state == detail::poller_state_t::EXIT) {
            state.poller_state = detail::poller_state_t::IDLE;
        }

        state.poller = std::thread([this, poll_duration] {
            while (state.poller_state != detail::poller_state_t::EXIT) {
                if (state.poller_state == detail::poller_state_t::IDLE) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }

                const auto poll_start = std::chrono::high_resolution_clock::now();

                auto ec = self().driver_poll();
                if (ec) {
                    fmt::println(std::cerr, "poll failed");
                }
                // Substract the time each iteration itself took in order to get closer to
                // the desired poll duration
                const auto elapsed_poll_duration =
                    std::chrono::high_resolution_clock::now() - poll_start;
                if (elapsed_poll_duration < poll_duration) {
                    std::this_thread::sleep_for(poll_duration - elapsed_poll_duration);
                }
            }
        });
    }

    void stop_poll_thread()
    {
        state.poller_state = detail::poller_state_t::EXIT;
        if (state.poller.joinable()) {
            state.poller.join();
        }
    }

    std::string driver_version() const { return self().driver_driver_version(); }

    std::string hardware_version() const { return self().driver_hardware_version(); }

    void initialize()
    {
        if (state.initialized) {
            return;
        }
        const auto ec = self().driver_initialize();
        if (ec) {
            throw std::runtime_error(fmt::format("Initialization failed"));
        }

        state.initialized = true;
    }

    void close()
    {
        self().driver_close();
        state.closed = true;
        state.initialized = false;
        state.configured = false;
    }

    void configure()
    {
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

    void arm()
    {
        if (state.armed) {
            return;
        }

        // arm the driver
        auto ec = self().driver_arm();
        if (ec) {
            throw std::runtime_error("Arming failed");
        }

        state.armed = true;
        if (acquisition_mode == acquisition_mode_t::STREAMING) {
            state.poller_state = detail::poller_state_t::RUNNING;
        }
    }

    void disarm() noexcept
    {
        if (!state.armed) {
            return;
        }

        if (acquisition_mode == acquisition_mode_t::STREAMING) {
            state.poller_state = detail::poller_state_t::IDLE;
        }

        const auto ec = self().driver_disarm();

        if (ec) {
            fmt::println(std::cerr, "disarm failed");
        }

        state.armed = false;
    }

    void process_driver_data(std::size_t nr_samples, std::size_t offset)
    {
        for (auto& channel : state.channels) {
            const auto voltage_multiplier =
                static_cast<float>(channel.settings.range / state.max_value);
            auto write_data = channel.data.writer.reserve_output_range(nr_samples);
            volk_16i_s32f_convert_32f(write_data.data(),
                                      channel.driver_buffer.data() + offset,
                                      1.0f / voltage_multiplier,
                                      static_cast<uint>(nr_samples));
            write_data.publish(nr_samples);
        }

        state.data_available += nr_samples;
        state.produced_worker += nr_samples;
    }

    void
    streaming_callback(int32_t nr_samples_signed, uint32_t start_index, int16_t overflow)
    {
        assert(nr_samples_signed >= 0);
        const auto nr_samples = static_cast<std::size_t>(nr_samples_signed);

        // According to well informed sources, the driver indicates the buffer overrun by
        // setting all the bits of the overflow argument to true.
        if (static_cast<uint16_t>(overflow) == 0xffff) {
            fmt::println(std::cerr, "Buffer overrun detected, continue...");
        }
        auto can_write = nr_samples;
        for (const auto& channel : state.channels) {
            can_write = std::min(can_write, channel.data.writer.available());
        }

        if (can_write < nr_samples) {
            const auto lost = nr_samples - can_write;
            state.lost_count += lost;
            fmt::println(std::cerr, "Dropped {} samples", lost);
        }

        if (can_write == 0) {
            return;
        }

        process_driver_data(can_write, start_index);
    }

    void rapid_block_callback(std::error_code ec)
    {
        if (ec) {
            report_error(ec);
            return;
        }
        const auto samples = pre_samples + post_samples;
        for (std::size_t capture = 0; capture < rapid_block_nr_captures; ++capture) {

            const auto get_values_result =
                self().driver_rapid_block_get_values(capture, samples);
            if (get_values_result.error) {
                report_error(ec);
                return;
            }

            // TODO handle overflow for individual channels?

            process_driver_data(get_values_result.samples, 0);
        }

        if (trigger_once) {
            state.data_finished = true;
        }
    }

    void report_error(std::error_code ec)
    {
        auto out = state.errors.writer.reserve_output_range(1);
        out[0] = { state.produced_worker, ec };
        out.publish(1);
    }

    [[nodiscard]] constexpr auto& self() noexcept { return *static_cast<PSImpl*>(this); }

    [[nodiscard]] constexpr const auto& self() const noexcept
    {
        return *static_cast<const PSImpl*>(this);
    }
};

} // namespace gr::picoscope
