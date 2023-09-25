#include <node.hpp>

#include <volk/volk.h>

#include <fmt/format.h>

#include <functional>

namespace gr::picoscope {

namespace detail {

/*!
 * \brief The state of the poller worker function.
 */
enum class poller_state_t { IDLE = 0, RUNNING, EXIT };

using streaming_callback_function_t = std::function<void(int32_t, uint32_t, int16_t)>;

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

struct Settings {
    std::string serial_number = {};
    std::map<std::string, channel_setting_t, std::less<>> enabled_channels = {};
    std::map<std::string, port_setting_t, std::less<>> enabled_ports = {};
    trigger_setting_t trigger = {};
    double sample_rate = 10000.;
    std::size_t driver_buffer_size = 100000;
    std::size_t pre_samples = 1000;
    std::size_t post_samples = 9000;
    acquisition_mode_t acquisition_mode = acquisition_mode_t::STREAMING;
    std::size_t rapid_block_nr_captures = 1;
    double streaming_mode_poll_rate = 0.001;
    bool auto_arm = true;
    bool trigger_once = false;
};

namespace detail {
struct Channel {
    std::string id;
    channel_setting_t settings;
    std::vector<int16_t> driver_buffer;
    gr::circular_buffer<float> data_buffer = gr::circular_buffer<float>(1);
    decltype(data_buffer.new_reader()) data_reader = data_buffer.new_reader();
    decltype(data_buffer.new_writer()) data_writer = data_buffer.new_writer();
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
    std::atomic<poller_state_t> poller_state = poller_state_t::IDLE;
    std::atomic<bool> forced_quit = false; // TODO transitional until we found out what
                                           // goes wrong with multithreaded scheduler

    std::size_t produced_worker = 0;
};
} // namespace detail

template <typename PSImpl>
struct Picoscope : public fair::graph::node<PSImpl> {
    const Settings ps_settings;
    detail::State state;
    detail::streaming_callback_function_t _streaming_callback;

    explicit Picoscope(Settings settings_ = {})
        : ps_settings{ std::move(settings_) },
          _streaming_callback(
              [this](int32_t noSamples, uint32_t startIndex, int16_t overflow) {
                  streaming_callback(noSamples, startIndex, overflow);
              })
    {
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

        // TODO maybe also check (at the end of configure()) that the configured channels
        // actually exist on the given device
        for (const auto& [id, _] : ps_settings.enabled_channels) {
            if (!PSImpl::driver_channel_id_to_index(id)) {
                throw std::invalid_argument(fmt::format("Unknown channel '{}'", id));
            }
        }

        if (ps_settings.trigger.is_enabled()) {
            if (!PSImpl::driver_channel_id_to_index(ps_settings.trigger.source)) {
                throw std::invalid_argument(fmt::format("Unknown trigger channel '{}'",
                                                        ps_settings.trigger.source));
            }
        }

        state.actual_sample_rate = ps_settings.sample_rate;

        state.channels.resize(ps_settings.enabled_channels.size());
        std::size_t channel_idx = 0;
        for (const auto& [id, settings] : ps_settings.enabled_channels) {
            auto& ch = state.channels[channel_idx];
            // TODO check id for validity (is subtype-specific)
            ch.id = id;
            ch.settings = settings;
            ch.data_buffer = gr::circular_buffer<float>(
                ps_settings.driver_buffer_size); // TODO think about what a sensible
                                                 // buffer size would be
            ch.data_reader = ch.data_buffer.new_reader();
            ch.data_writer = ch.data_buffer.new_writer();
            channel_idx++;
        }
    }

    ~Picoscope() { stop(); }

    std::make_signed_t<std::size_t> available_samples_impl() const noexcept
    {
        if (state.channels.empty()) {
            return -1;
        }

        if (state.data_finished && state.data_available == 0) {
            return -1;
        }

        if (state.forced_quit) {
            return -1;
        }

        return std::make_signed_t<std::size_t>(state.data_available);
    }

    struct ChannelOutput {
        std::span<float> values;
        std::span<float> errors;
    };

    template <std::size_t ChannelCount>
    inline fair::graph::work_return_status_t process_bulk_impl(
        const std::array<ChannelOutput, ChannelCount>& channel_outputs) noexcept
    {
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

        const auto data_available = state.data_available.load();

        for (std::size_t channel_index = 0; channel_index < state.channels.size();
             ++channel_index) {
            auto& channel = state.channels[channel_index];
            auto& values = channel_outputs[channel_index].values;
            auto& errors = channel_outputs[channel_index].errors;
            assert(values.size() == errors.size());
            assert(data_available >= values.size());

            const auto data = channel.data_reader.get(values.size());
            std::ignore = std::copy(data.begin(), data.end(), values.begin());
            const auto error_estimate =
                channel.settings.range *
                static_cast<double>(PSImpl::DRIVER_VERTICAL_PRECISION);
            std::fill(errors.begin(), errors.end(), error_estimate);
            std::ignore = channel.data_reader.consume(data.size());
        }

        state.data_available -= channel_outputs[0].values.size();

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
            if (ps_settings.auto_arm) {
                arm();
            }
            if (ps_settings.acquisition_mode == acquisition_mode_t::STREAMING) {
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

        if (ps_settings.acquisition_mode == acquisition_mode_t::STREAMING) {
            stop_poll_thread();
        }
    }

    void start_poll_thread()
    {
        if (state.poller.joinable()) {
            return;
        }
        const auto poll_duration =
            std::chrono::seconds(1) * ps_settings.streaming_mode_poll_rate;

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
        if (ps_settings.acquisition_mode == acquisition_mode_t::STREAMING) {
            state.poller_state = detail::poller_state_t::RUNNING;
        }
    }

    void disarm() noexcept
    {
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
            can_write = std::min(can_write, channel.data_writer.available());
        }

        if (can_write < nr_samples) {
            const auto lost = nr_samples - can_write;
            state.lost_count += lost;
            fmt::println(std::cerr, "Dropped {} samples", lost);
        }

        if (can_write == 0) {
            return;
        }

        for (auto& channel : state.channels) {

            const auto voltage_multiplier =
                static_cast<float>(channel.settings.range / state.max_value);
            auto write_data = channel.data_writer.reserve_output_range(can_write);
            volk_16i_s32f_convert_32f(write_data.data(),
                                      channel.driver_buffer.data() + start_index,
                                      1.0f / voltage_multiplier,
                                      static_cast<uint>(can_write));
            write_data.publish(can_write);
        }

        state.data_available += can_write;
    }

    [[nodiscard]] constexpr auto& self() noexcept { return *static_cast<PSImpl*>(this); }

    [[nodiscard]] constexpr const auto& self() const noexcept
    {
        return *static_cast<const PSImpl*>(this);
    }
};

} // namespace gr::picoscope
