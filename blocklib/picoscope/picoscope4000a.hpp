#ifndef FAIR_PICOSCOPE_PICOSCOPE4000A_HPP
#define FAIR_PICOSCOPE_PICOSCOPE4000A_HPP

#include <picoscope.hpp>

#include <ps4000aApi.h>

namespace fair::picoscope {

namespace detail {
std::mutex g_init_mutex;

std::string
get_unit_info_topic(int16_t handle, PICO_INFO info) {
    std::array<int8_t, 40> line;
    int16_t                required_size;

    auto                   status = ps4000aGetUnitInfo(handle, line.data(), line.size(), &required_size, info);
    if (status == PICO_OK) {
        return std::string(reinterpret_cast<char *>(line.data()), static_cast<std::size_t>(required_size));
    }

    return {};
}

constexpr PS4000A_COUPLING
convert_to_ps4000a_coupling(coupling_t coupling) {
    if (coupling == coupling_t::AC_1M) return PS4000A_AC;
    if (coupling == coupling_t::DC_1M) return PS4000A_DC;

    throw std::runtime_error(fmt::format("Unsupported coupling mode: {}", static_cast<int>(coupling)));
}

constexpr PS4000A_RANGE
convert_to_ps4000a_range(double range) {
    if (range == 0.01) return PS4000A_10MV;
    if (range == 0.02) return PS4000A_20MV;
    if (range == 0.05) return PS4000A_50MV;
    if (range == 0.1) return PS4000A_100MV;
    if (range == 0.2) return PS4000A_200MV;
    if (range == 0.5) return PS4000A_500MV;
    if (range == 1.0) return PS4000A_1V;
    if (range == 2.0) return PS4000A_2V;
    if (range == 5.0) return PS4000A_5V;
    if (range == 10.0) return PS4000A_10V;
    if (range == 20.0) return PS4000A_20V;
    if (range == 50.0) return PS4000A_50V;
    if (range == 100.0) return PS4000A_100V;
    if (range == 200.0) return PS4000A_200V;

    throw std::runtime_error(fmt::format("Range value not supported: {}", range));
}

constexpr void
validate_desired_actual_frequency_ps4000(double desired_freq, double actual_freq) {
    // In order to prevent exceptions/exit due to rounding errors, we dont directly
    // compare actual_freq to desired_freq, but instead allow a difference up to 0.001%
    constexpr double max_diff_percentage = 0.001;
    const double     diff_percent        = (actual_freq - desired_freq) * 100 / desired_freq;
    if (abs(diff_percent) > max_diff_percentage) {
        throw std::runtime_error(fmt::format("Desired and actual frequency do not match. desired: {} actual: {}", desired_freq, actual_freq));
    }
}

constexpr int16_t
convert_voltage_to_ps4000a_raw_logic_value(double value) {
    constexpr double max_logical_voltage = 5.0;

    if (value > max_logical_voltage) {
        throw std::invalid_argument(fmt::format("Maximum logical level is: {}, received: {}.", max_logical_voltage, value));
    }
    // Note max channel value not provided with PicoScope API, we use ext max value
    return static_cast<int16_t>(value / max_logical_voltage * PS4000A_EXT_MAX_VALUE);
}

constexpr std::optional<PS4000A_CHANNEL>
convert_to_ps4000a_channel(std::string_view source) {
    if (source == "A") return PS4000A_CHANNEL_A;
    if (source == "B") return PS4000A_CHANNEL_B;
    if (source == "C") return PS4000A_CHANNEL_C;
    if (source == "D") return PS4000A_CHANNEL_D;
    if (source == "E") return PS4000A_CHANNEL_E;
    if (source == "F") return PS4000A_CHANNEL_F;
    if (source == "G") return PS4000A_CHANNEL_G;
    if (source == "H") return PS4000A_CHANNEL_H;
    if (source == "EXTERNAL") return PS4000A_EXTERNAL;
    return {};
}

constexpr PS4000A_THRESHOLD_DIRECTION
convert_to_ps4000a_threshold_direction(trigger_direction_t direction) {
    switch (direction) {
    case trigger_direction_t::RISING: return PS4000A_RISING;
    case trigger_direction_t::FALLING: return PS4000A_FALLING;
    case trigger_direction_t::LOW: return PS4000A_BELOW;
    case trigger_direction_t::HIGH: return PS4000A_ABOVE;
    default: throw std::runtime_error(fmt::format("Unsupported trigger direction: {}", static_cast<int>(direction)));
    }
};

/*!
 * a structure used for streaming setup
 */
struct ps4000a_unit_interval_t {
    PS4000A_TIME_UNITS unit;
    uint32_t           interval;
};

ps4000a_unit_interval_t
convert_frequency_to_ps4000a_time_units_and_interval(double desired_freq, double &actual_freq) {
    ps4000a_unit_interval_t unint;

    if (const auto interval = 1.0 / desired_freq; interval < 0.000001) {
        unint.unit     = PS4000A_PS;
        unint.interval = static_cast<uint32_t>(1000000000000.0 / desired_freq);
        actual_freq    = 1000000000000.0 / unint.interval;
    } else if (interval < 0.001) {
        unint.unit     = PS4000A_NS;
        unint.interval = static_cast<uint32_t>(1000000000.0 / desired_freq);
        actual_freq    = 1000000000.0 / unint.interval;
    } else if (interval < 0.1) {
        unint.unit     = PS4000A_US;
        unint.interval = static_cast<uint32_t>(1000000.0 / desired_freq);
        actual_freq    = 1000000.0 / unint.interval;
    } else {
        unint.unit     = PS4000A_MS;
        unint.interval = static_cast<uint32_t>(1000.0 / desired_freq);
        actual_freq    = 1000.0 / unint.interval;
    }
    validate_desired_actual_frequency_ps4000(desired_freq, actual_freq);
    return unint;
}

/*!
 * Note this function has to be called after the call to the ps3000aSetChannel function,
 * that is just befor the arm!!!
 */
uint32_t
convert_frequency_to_ps4000a_timebase(int16_t handle, double desired_freq, double &actual_freq) {
    // It is assumed that the timebase is calculated like this:
    // (timebase–2) / 125,000,000
    // e.g. timeebase == 3 --> 8ns sample interval
    //
    // Note, for some devices, the above formula might be wrong! To overcome this
    // limitation we use the ps3000aGetTimebase2 function to find the closest possible
    // timebase. The below timebase estimate is therefore used as a fallback only.
    auto     time_interval_ns  = 1000000000.0 / desired_freq;
    uint32_t timebase_estimate = (static_cast<uint32_t>(time_interval_ns) / 8) + 2;

    // In order to cover for all possible 30000 series devices, we use ps3000aGetTimebase2
    // function to get step size in ns between timebase 3 and 4. Based on that the actual
    // timebase is calculated.
    int32_t              dummy;
    std::array<float, 2> time_interval_ns_34;

    for (std::size_t i = 0; i < 2; i++) {
        auto status = ps4000aGetTimebase2(handle, 3 + static_cast<uint32_t>(i), 1024, &time_interval_ns_34[i], &dummy, 0);
        if (status != PICO_OK) {
#ifdef PROTO_PORT_DISABLED
            d_logger->notice("timebase cannot be obtained: {}", get_error_message(status));
            d_logger->notice("    estimated timebase will be used...");
#endif

            float time_interval_ns_tmp;
            status = ps4000aGetTimebase2(handle, timebase_estimate, 1024, &time_interval_ns_tmp, &dummy, 0);
            if (status != PICO_OK) {
                throw std::runtime_error(fmt::format("Local time {}. Error: {}", timebase_estimate, get_error_message(status)));
            }

            actual_freq = 1000000000.0 / static_cast<double>(time_interval_ns_tmp);
            validate_desired_actual_frequency_ps4000(desired_freq, actual_freq);
            return timebase_estimate;
        }
    }

    // Calculate steps between timebase 3 and 4 and correct start_timebase estimate based
    // on that
    auto step         = static_cast<double>(time_interval_ns_34[1] - time_interval_ns_34[0]);
    timebase_estimate = static_cast<uint32_t>((time_interval_ns - static_cast<double>(time_interval_ns_34[0])) / step) + 3;

    // The below code iterates trought the neighbouring timebases in order to find the
    // best match. In principle we could check only timebases on the left and right but
    // since first three timebases are in most cases special we make search space a bit
    // bigger.
    const int                       search_space = 8;
    std::array<float, search_space> timebases;
    std::array<float, search_space> error_estimates;

    uint32_t                        start_timebase = timebase_estimate > (search_space / 2) ? timebase_estimate - (search_space / 2) : 0;

    for (std::size_t i = 0; i < search_space; i++) {
        float obtained_time_interval_ns;
        auto  status = ps4000aGetTimebase2(handle, start_timebase + static_cast<uint32_t>(i), 1024, &obtained_time_interval_ns, &dummy, 0);
        if (status != PICO_OK) {
            // this timebase can't be used, lets set error estimate to something big
            timebases[i]       = -1;
            error_estimates[i] = 10000000000.0;
        } else {
            timebases[i]       = obtained_time_interval_ns;
            error_estimates[i] = static_cast<float>(std::abs(time_interval_ns - static_cast<double>(obtained_time_interval_ns)));
        }
    }

    auto it       = std::min_element(&error_estimates[0], &error_estimates[0] + error_estimates.size());
    auto distance = std::distance(&error_estimates[0], it);

    assert(distance < search_space);

    // update actual update rate and return timebase number
    actual_freq = 1000000000.0 / static_cast<double>(timebases[static_cast<std::size_t>(distance)]);
    validate_desired_actual_frequency_ps4000(desired_freq, actual_freq);
    return static_cast<uint32_t>(start_timebase + distance);
}

} // namespace detail

struct Picoscope4000a : public fair::picoscope::Picoscope<Picoscope4000a> {
    using AnalogPort = gr::PortOut<float>;

    std::array<gr::PortOut<float>, 8> values;
    std::array<gr::PortOut<float>, 8> errors;

    gr::work_return_t
    work(std::size_t requested_work = 0) {
        std::ignore = requested_work;
        return this->work_impl();
    }

    Error
    set_buffers(size_t samples, uint32_t block_number) {
        for (auto &channel : state.channels) {
            const auto channel_index = detail::convert_to_ps4000a_channel(channel.id);
            assert(channel_index);

            channel.driver_buffer.resize(std::max(samples, channel.driver_buffer.size()));
            const auto status = ps4000aSetDataBuffer(state.handle, *channel_index, channel.driver_buffer.data(), static_cast<int32_t>(samples), block_number, PS4000A_RATIO_MODE_NONE);

            if (status != PICO_OK) {
                fmt::println(std::cerr, "ps4000aSetDataBuffer (chan {}): {}", static_cast<std::size_t>(*channel_index), get_error_message(status));
                return { status };
            }
        }

        return {};
    }

    static constexpr float DRIVER_VERTICAL_PRECISION = 0.01f;

    static std::optional<std::size_t>
    driver_channel_id_to_index(std::string_view id) {
        const auto channel = detail::convert_to_ps4000a_channel(id);
        if (!channel) {
            return {};
        }
        return static_cast<std::size_t>(*channel);
    }

    std::string
    driver_driver_version() const {
        const std::string prefix  = "PS4000A Linux Driver, ";
        auto              version = detail::get_unit_info_topic(state.handle, PICO_DRIVER_VERSION);

        if (auto i = version.find(prefix); i != std::string::npos) version.erase(i, prefix.length());
        return version;
    }

    std::string
    driver_hardware_version() const {
        if (!state.initialized) return {};
        return detail::get_unit_info_topic(state.handle, PICO_HARDWARE_VERSION);
    }

    fair::picoscope::GetValuesResult
    driver_rapid_block_get_values(std::size_t capture, std::size_t samples) {
        if (const auto ec = set_buffers(samples, static_cast<uint32_t>(capture)); ec) {
            return { ec, 0, 0 };
        }

        auto       nr_samples = static_cast<uint32_t>(samples);
        int16_t    overflow   = 0;
        const auto status     = ps4000aGetValues(state.handle,
                                                 0, // offset
                                                 &nr_samples, 1, PS4000A_RATIO_MODE_NONE, static_cast<uint32_t>(capture), &overflow);
        if (status != PICO_OK) {
            fmt::println(std::cerr, "ps4000aGetValues: {}", get_error_message(status));
            return { { status }, 0, 0 };
        }

        return { {}, static_cast<std::size_t>(nr_samples), overflow };
    }

    Error
    driver_initialize() {
        PICO_STATUS status;

        // Required to force sequence execution of open unit calls...
        std::lock_guard init_guard{ detail::g_init_mutex };

        // take any if serial number is not provided (useful for testing purposes)
        if (ps_settings.serial_number.empty()) {
            status = ps4000aOpenUnit(&state.handle, nullptr);
        } else {
            status = ps4000aOpenUnit(&state.handle, const_cast<int8_t *>(reinterpret_cast<const int8_t *>(ps_settings.serial_number.data())));
        }

        // ignore ext. power not connected error/warning
        if (status == PICO_POWER_SUPPLY_NOT_CONNECTED || status == PICO_USB3_0_DEVICE_NON_USB3_0_PORT) {
            status = ps4000aChangePowerSource(state.handle, status);
            if (status == PICO_POWER_SUPPLY_NOT_CONNECTED || status == PICO_USB3_0_DEVICE_NON_USB3_0_PORT) {
                status = ps4000aChangePowerSource(state.handle, status);
            }
        }

        if (status != PICO_OK) {
            fmt::println(std::cerr, "open unit failed: {} ", get_error_message(status));
            return { status };
        }

        // maximum value is used for conversion to volts
        status = ps4000aMaximumValue(state.handle, &state.max_value);
        if (status != PICO_OK) {
            ps4000aCloseUnit(state.handle);
            fmt::println(std::cerr, "ps4000aMaximumValue: {}", get_error_message(status));
            return { status };
        }

        return {};
    }

    Error
    driver_close() {
        if (state.handle == -1) {
            return {};
        }

        auto status  = ps4000aCloseUnit(state.handle);
        state.handle = -1;

        if (status != PICO_OK) {
            fmt::println(std::cerr, "ps4000aCloseUnit: {}", get_error_message(status));
        }
        return { status };
    }

    Error
    driver_configure() {
        int32_t max_samples;
        auto    status = ps4000aMemorySegments(state.handle, static_cast<uint32_t>(ps_settings.rapid_block_nr_captures), &max_samples);
        if (status != PICO_OK) {
            fmt::println(std::cerr, "ps4000aMemorySegments: {}", get_error_message(status));
            return { status };
        }

        if (ps_settings.acquisition_mode == acquisition_mode_t::RAPID_BLOCK) {
            status = ps4000aSetNoOfCaptures(state.handle, static_cast<uint32_t>(ps_settings.rapid_block_nr_captures));
            if (status != PICO_OK) {
                fmt::println(std::cerr, "ps4000aSetNoOfCaptures: {}", get_error_message(status));
                return { status };
            }
        }

        // configure analog channels
        for (std::size_t i = 0; i <= PS4000A_MAX_CHANNELS; ++i) {
            ps4000aSetChannel(state.handle, static_cast<PS4000A_CHANNEL>(i), false, PS4000A_AC, PICO_X10_ACTIVE_PROBE_100MV, 0.);
        }

        for (const auto &channel : state.channels) {
            const auto idx = detail::convert_to_ps4000a_channel(channel.id);
            assert(idx);
            const auto coupling = detail::convert_to_ps4000a_coupling(channel.settings.coupling);
            const auto range    = detail::convert_to_ps4000a_range(channel.settings.range);

            status              = ps4000aSetChannel(state.handle, *idx, true, coupling, static_cast<PICO_CONNECT_PROBE_RANGE>(range), channel.settings.offset);
            if (status != PICO_OK) {
                fmt::println(std::cerr, "ps4000aSetChannel (chan '{}'): {}", channel.id, get_error_message(status));
                return { status };
            }
        }

        // apply trigger configuration
        if (ps_settings.trigger.is_analog() && ps_settings.acquisition_mode == acquisition_mode_t::RAPID_BLOCK) {
            const auto channel = detail::convert_to_ps4000a_channel(ps_settings.trigger.source);
            assert(channel);
            status = ps4000aSetSimpleTrigger(state.handle,
                                             true, // enable
                                             *channel, detail::convert_voltage_to_ps4000a_raw_logic_value(ps_settings.trigger.threshold),
                                             detail::convert_to_ps4000a_threshold_direction(ps_settings.trigger.direction),
                                             0,   // delay
                                             -1); // auto trigger
            if (status != PICO_OK) {
                fmt::println(std::cerr, "ps4000aSetSimpleTrigger: {}", get_error_message(status));
                return { status };
            }
        } else {
            // disable triggers
            for (int i = 0; i < PS4000A_MAX_CHANNELS; i++) {
                PS4000A_CONDITION cond;
                cond.source    = static_cast<PS4000A_CHANNEL>(i);
                cond.condition = PS4000A_CONDITION_DONT_CARE;
                status         = ps4000aSetTriggerChannelConditions(state.handle, &cond, 1, PS4000A_CLEAR);
                if (status != PICO_OK) {
                    fmt::println(std::cerr, "ps4000aSetTriggerChannelConditionsV2: {}", get_error_message(status));
                    return { status };
                }
            }
        }

        // In order to validate desired frequency before startup
        double actual_freq;
        detail::convert_frequency_to_ps4000a_timebase(state.handle, ps_settings.sample_rate, actual_freq);

        return {};
    }

    Error
    driver_arm() {
        if (ps_settings.acquisition_mode == acquisition_mode_t::RAPID_BLOCK) {
            uint32_t    timebase   = detail::convert_frequency_to_ps4000a_timebase(state.handle, ps_settings.sample_rate, state.actual_sample_rate);

            static auto redirector = [](int16_t, PICO_STATUS status, void *vobj) { static_cast<Picoscope4000a *>(vobj)->rapid_block_callback({ status }); };

            auto        status     = ps4000aRunBlock(state.handle, static_cast<int32_t>(ps_settings.pre_samples), static_cast<int32_t>(ps_settings.post_samples),
                                                     timebase, // timebase
                                                     nullptr,  // time indispossed
                                                     0,        // segment index
                                                     static_cast<ps4000aBlockReady>(redirector), this);
            if (status != PICO_OK) {
                fmt::println(std::cerr, "ps4000aRunBlock: {}", get_error_message(status));
                return { status };
            }
        } else {
            using fair::picoscope::detail::driver_buffer_size;
            set_buffers(driver_buffer_size, 0);

            auto unit_int = detail::convert_frequency_to_ps4000a_time_units_and_interval(ps_settings.sample_rate, state.actual_sample_rate);

            auto status   = ps4000aRunStreaming(state.handle,
                                                &unit_int.interval, // sample interval
                                                unit_int.unit,      // time unit of sample interval
                                                0,                  // pre-triggersamples (unused)
                                                static_cast<uint32_t>(driver_buffer_size), false,
                                                1, // downsampling factor // TODO reconsider if we need downsampling support
                                                PS4000A_RATIO_MODE_NONE, static_cast<uint32_t>(driver_buffer_size));

            if (status != PICO_OK) {
                fmt::println(std::cerr, "ps4000aRunStreaming: {}", get_error_message(status));
                return { status };
            }
        }

        return {};
    }

    Error
    driver_disarm() noexcept {
        if (const auto status = ps4000aStop(state.handle); status != PICO_OK) {
            fmt::println(std::cerr, "ps4000aStop: {}", get_error_message(status));
            return { status };
        }

        return {};
    }

    Error
    driver_poll() {
        const auto status = ps4000aGetStreamingLatestValues(state.handle, static_cast<ps4000aStreamingReady>(fair::picoscope::detail::invoke_streaming_callback), &_streaming_callback);
        if (status == PICO_BUSY || status == PICO_DRIVER_FUNCTION) {
            return {};
        }
        return { status };
    }
};

} // namespace fair::picoscope

ENABLE_REFLECTION(fair::picoscope::Picoscope4000a, values, errors, serial_number, sample_rate, pre_samples, post_samples, acquisition_mode, rapid_block_nr_captures, streaming_mode_poll_rate, auto_arm,
                  trigger_once, channel_ids, channel_names, channel_units, channel_ranges, channel_offsets, channel_couplings, trigger_source, trigger_threshold, trigger_direction, trigger_pin);

#endif
