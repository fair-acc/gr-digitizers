#ifndef FAIR_PICOSCOPE_PICOSCOPE4000A_HPP
#define FAIR_PICOSCOPE_PICOSCOPE4000A_HPP

#include <Picoscope.hpp>

#include <ps4000aApi.h>

namespace fair::picoscope {

namespace detail {
std::mutex g_init_mutex;

std::string
getUnitInfoTopic(int16_t handle, PICO_INFO info) {
    std::array<int8_t, 40> line;
    int16_t                requiredSize;

    auto                   status = ps4000aGetUnitInfo(handle, line.data(), line.size(), &requiredSize, info);
    if (status == PICO_OK) {
        return std::string(reinterpret_cast<char *>(line.data()), static_cast<std::size_t>(requiredSize));
    }

    return {};
}

constexpr PS4000A_COUPLING
convertToPs4000aCoupling(Coupling coupling) {
    if (coupling == Coupling::AC_1M) return PS4000A_AC;
    if (coupling == Coupling::DC_1M) return PS4000A_DC;

    throw std::runtime_error(fmt::format("Unsupported coupling mode: {}", static_cast<int>(coupling)));
}

constexpr PS4000A_RANGE
convertToPs4000aRange(double range) {
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
validateDesiredActualFrequencyPs4000a(double desiredFreq, double actualFreq) {
    // In order to prevent exceptions/exit due to rounding errors, we dont directly
    // compare actual_freq to desired_freq, but instead allow a difference up to 0.001%
    constexpr double MAX_DIFF_PERCENTAGE = 0.001;
    const double     diffPercent         = (actualFreq - desiredFreq) * 100 / desiredFreq;
    if (abs(diffPercent) > MAX_DIFF_PERCENTAGE) {
        throw std::runtime_error(fmt::format("Desired and actual frequency do not match. desired: {} actual: {}", desiredFreq, actualFreq));
    }
}

constexpr int16_t
convertVoltageToPs4000aRawLogicValue(double value) {
    constexpr double MAX_LOGICAL_VOLTAGE = 5.0;

    if (value > MAX_LOGICAL_VOLTAGE) {
        throw std::invalid_argument(fmt::format("Maximum logical level is: {}, received: {}.", MAX_LOGICAL_VOLTAGE, value));
    }
    // Note max channel value not provided with PicoScope API, we use ext max value
    return static_cast<int16_t>(value / MAX_LOGICAL_VOLTAGE * PS4000A_EXT_MAX_VALUE);
}

constexpr std::optional<PS4000A_CHANNEL>
convertToPs4000aChannel(std::string_view source) {
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
convertToPs4000aThresholdDirection(TriggerDirection direction) {
    using enum TriggerDirection;
    switch (direction) {
    case RISING: return PS4000A_RISING;
    case FALLING: return PS4000A_FALLING;
    case LOW: return PS4000A_BELOW;
    case HIGH: return PS4000A_ABOVE;
    default: throw std::runtime_error(fmt::format("Unsupported trigger direction: {}", static_cast<int>(direction)));
    }
};

/*!
 * a structure used for streaming setup
 */
struct Ps4000aUnitInterval {
    PS4000A_TIME_UNITS unit;
    uint32_t           interval;
};

Ps4000aUnitInterval
convertFrequencyToPs4000aTimeUnitsAndInterval(double desired_freq, double &actual_freq) {
    Ps4000aUnitInterval unint;

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
    validateDesiredActualFrequencyPs4000a(desired_freq, actual_freq);
    return unint;
}

/*!
 * Note this function has to be called after the call to the ps3000aSetChannel function,
 * that is just befor the arm!!!
 */
uint32_t
convertFrequencyToPs4000aTimebase(int16_t handle, double desiredFreq, double &actualFreq) {
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
        auto status = ps4000aGetTimebase2(handle, 3 + static_cast<uint32_t>(i), 1024, &timeIntervalNS_34[i], &dummy, 0);
        if (status != PICO_OK) {
#ifdef PROTO_PORT_DISABLED
            d_logger->notice("timebase cannot be obtained: {}", get_error_message(status));
            d_logger->notice("    estimated timebase will be used...");
#endif

            float timeIntervalNS_tmp;
            status = ps4000aGetTimebase2(handle, timebaseEstimate, 1024, &timeIntervalNS_tmp, &dummy, 0);
            if (status != PICO_OK) {
                throw std::runtime_error(fmt::format("Local time {}. Error: {}", timebaseEstimate, detail::getErrorMessage(status)));
            }

            actualFreq = 1000000000.0 / static_cast<double>(timeIntervalNS_tmp);
            validateDesiredActualFrequencyPs4000a(desiredFreq, actualFreq);
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

    uint32_t                       startTimebase = timebaseEstimate > (searchSpace / 2) ? timebaseEstimate - (searchSpace / 2) : 0;

    for (std::size_t i = 0; i < searchSpace; i++) {
        float obtained_time_interval_ns;
        auto  status = ps4000aGetTimebase2(handle, startTimebase + static_cast<uint32_t>(i), 1024, &obtained_time_interval_ns, &dummy, 0);
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
    validateDesiredActualFrequencyPs4000a(desiredFreq, actualFreq);
    return static_cast<uint32_t>(startTimebase + distance);
}

} // namespace detail

template<typename T>
struct Picoscope4000a : public fair::picoscope::Picoscope<T, Picoscope4000a<T>> {
    std::array<gr::PortOut<T>, 8> analog_out;

    gr::work_return_t
    work(std::size_t requestedWork = 0) {
        std::ignore = requestedWork;
        return this->workImpl();
    }

    Error
    setBuffers(size_t samples, uint32_t blockNumber) {
        for (auto &channel : this->state.channels) {
            const auto channelIndex = detail::convertToPs4000aChannel(channel.id);
            assert(channelIndex);

            channel.driver_buffer.resize(std::max(samples, channel.driver_buffer.size()));
            const auto status = ps4000aSetDataBuffer(this->state.handle, *channelIndex, channel.driver_buffer.data(), static_cast<int32_t>(samples), blockNumber, PS4000A_RATIO_MODE_NONE);

            if (status != PICO_OK) {
                fmt::println(std::cerr, "ps4000aSetDataBuffer (chan {}): {}", static_cast<std::size_t>(*channelIndex), detail::getErrorMessage(status));
                return { status };
            }
        }

        return {};
    }

    static constexpr float DRIVER_VERTICAL_PRECISION = 0.01f;

    static std::optional<std::size_t>
    driver_channelIdToIndex(std::string_view id) {
        const auto channel = detail::convertToPs4000aChannel(id);
        if (!channel) {
            return {};
        }
        return static_cast<std::size_t>(*channel);
    }

    std::string
    driver_driverVersion() const {
        const std::string prefix  = "PS4000A Linux Driver, ";
        auto              version = detail::getUnitInfoTopic(this->state.handle, PICO_DRIVER_VERSION);

        if (auto i = version.find(prefix); i != std::string::npos) version.erase(i, prefix.length());
        return version;
    }

    std::string
    driver_hardwareVersion() const {
        if (!this->state.initialized) return {};
        return detail::getUnitInfoTopic(this->state.handle, PICO_HARDWARE_VERSION);
    }

    fair::picoscope::GetValuesResult
    driver_rapidBlockGetValues(std::size_t capture, std::size_t samples) {
        if (const auto ec = setBuffers(samples, static_cast<uint32_t>(capture)); ec) {
            return { ec, 0, 0 };
        }

        auto       nrSamples = static_cast<uint32_t>(samples);
        int16_t    overflow  = 0;
        const auto status    = ps4000aGetValues(this->state.handle,
                                                0, // offset
                                                &nrSamples, 1, PS4000A_RATIO_MODE_NONE, static_cast<uint32_t>(capture), &overflow);
        if (status != PICO_OK) {
            fmt::println(std::cerr, "ps4000aGetValues: {}", detail::getErrorMessage(status));
            return { { status }, 0, 0 };
        }

        return { {}, static_cast<std::size_t>(nrSamples), overflow };
    }

    Error
    driver_initialize() {
        PICO_STATUS status;

        // Required to force sequence execution of open unit calls...
        std::lock_guard initGuard{ detail::g_init_mutex };

        // take any if serial number is not provided (useful for testing purposes)
        if (this->ps_settings.serial_number.empty()) {
            status = ps4000aOpenUnit(&this->state.handle, nullptr);
        } else {
            status = ps4000aOpenUnit(&this->state.handle, const_cast<int8_t *>(reinterpret_cast<const int8_t *>(this->ps_settings.serial_number.data())));
        }

        // ignore ext. power not connected error/warning
        if (status == PICO_POWER_SUPPLY_NOT_CONNECTED || status == PICO_USB3_0_DEVICE_NON_USB3_0_PORT) {
            status = ps4000aChangePowerSource(this->state.handle, status);
            if (status == PICO_POWER_SUPPLY_NOT_CONNECTED || status == PICO_USB3_0_DEVICE_NON_USB3_0_PORT) {
                status = ps4000aChangePowerSource(this->state.handle, status);
            }
        }

        if (status != PICO_OK) {
            fmt::println(std::cerr, "open unit failed: {} ", detail::getErrorMessage(status));
            return { status };
        }

        // maximum value is used for conversion to volts
        status = ps4000aMaximumValue(this->state.handle, &this->state.max_value);
        if (status != PICO_OK) {
            ps4000aCloseUnit(this->state.handle);
            fmt::println(std::cerr, "ps4000aMaximumValue: {}", detail::getErrorMessage(status));
            return { status };
        }

        return {};
    }

    Error
    driver_close() {
        if (this->state.handle == -1) {
            return {};
        }

        auto status        = ps4000aCloseUnit(this->state.handle);
        this->state.handle = -1;

        if (status != PICO_OK) {
            fmt::println(std::cerr, "ps4000aCloseUnit: {}", detail::getErrorMessage(status));
        }
        return { status };
    }

    Error
    driver_configure() {
        int32_t maxSamples;
        auto    status = ps4000aMemorySegments(this->state.handle, static_cast<uint32_t>(this->ps_settings.rapid_block_nr_captures), &maxSamples);
        if (status != PICO_OK) {
            fmt::println(std::cerr, "ps4000aMemorySegments: {}", detail::getErrorMessage(status));
            return { status };
        }

        if (this->ps_settings.acquisition_mode == AcquisitionMode::RAPID_BLOCK) {
            status = ps4000aSetNoOfCaptures(this->state.handle, static_cast<uint32_t>(this->ps_settings.rapid_block_nr_captures));
            if (status != PICO_OK) {
                fmt::println(std::cerr, "ps4000aSetNoOfCaptures: {}", detail::getErrorMessage(status));
                return { status };
            }
        }

        // configure analog channels
        for (std::size_t i = 0; i <= PS4000A_MAX_CHANNELS; ++i) {
            ps4000aSetChannel(this->state.handle, static_cast<PS4000A_CHANNEL>(i), false, PS4000A_AC, PICO_X10_ACTIVE_PROBE_100MV, 0.);
        }

        for (const auto &channel : this->state.channels) {
            const auto idx = detail::convertToPs4000aChannel(channel.id);
            assert(idx);
            const auto coupling = detail::convertToPs4000aCoupling(channel.settings.coupling);
            const auto range    = detail::convertToPs4000aRange(channel.settings.range);

            status              = ps4000aSetChannel(this->state.handle, *idx, true, coupling, static_cast<PICO_CONNECT_PROBE_RANGE>(range), channel.settings.offset);
            if (status != PICO_OK) {
                fmt::println(std::cerr, "ps4000aSetChannel (chan '{}'): {}", channel.id, detail::getErrorMessage(status));
                return { status };
            }
        }

        // apply trigger configuration
        if (this->ps_settings.trigger.isAnalog() && this->ps_settings.acquisition_mode == AcquisitionMode::RAPID_BLOCK) {
            const auto channel = detail::convertToPs4000aChannel(this->ps_settings.trigger.source);
            assert(channel);
            status = ps4000aSetSimpleTrigger(this->state.handle,
                                             true, // enable
                                             *channel, detail::convertVoltageToPs4000aRawLogicValue(this->ps_settings.trigger.threshold),
                                             detail::convertToPs4000aThresholdDirection(this->ps_settings.trigger.direction),
                                             0,   // delay
                                             -1); // auto trigger
            if (status != PICO_OK) {
                fmt::println(std::cerr, "ps4000aSetSimpleTrigger: {}", detail::getErrorMessage(status));
                return { status };
            }
        } else {
            // disable triggers
            for (int i = 0; i < PS4000A_MAX_CHANNELS; i++) {
                PS4000A_CONDITION cond;
                cond.source    = static_cast<PS4000A_CHANNEL>(i);
                cond.condition = PS4000A_CONDITION_DONT_CARE;
                status         = ps4000aSetTriggerChannelConditions(this->state.handle, &cond, 1, PS4000A_CLEAR);
                if (status != PICO_OK) {
                    fmt::println(std::cerr, "ps4000aSetTriggerChannelConditionsV2: {}", detail::getErrorMessage(status));
                    return { status };
                }
            }
        }

        // In order to validate desired frequency before startup
        double actual_freq;
        detail::convertFrequencyToPs4000aTimebase(this->state.handle, this->ps_settings.sample_rate, actual_freq);

        return {};
    }

    Error
    driver_arm() {
        if (this->ps_settings.acquisition_mode == AcquisitionMode::RAPID_BLOCK) {
            uint32_t    timebase   = detail::convertFrequencyToPs4000aTimebase(this->state.handle, this->ps_settings.sample_rate, this->state.actual_sample_rate);

            static auto redirector = [](int16_t, PICO_STATUS status, void *vobj) { static_cast<Picoscope4000a *>(vobj)->rapidBlockCallback({ status }); };

            auto        status     = ps4000aRunBlock(this->state.handle, static_cast<int32_t>(this->ps_settings.pre_samples), static_cast<int32_t>(this->ps_settings.post_samples),
                                                     timebase, // timebase
                                                     nullptr,  // time indispossed
                                                     0,        // segment index
                                                     static_cast<ps4000aBlockReady>(redirector), this);
            if (status != PICO_OK) {
                fmt::println(std::cerr, "ps4000aRunBlock: {}", detail::getErrorMessage(status));
                return { status };
            }
        } else {
            using fair::picoscope::detail::driver_buffer_size;
            setBuffers(driver_buffer_size, 0);

            auto unit_int = detail::convertFrequencyToPs4000aTimeUnitsAndInterval(this->ps_settings.sample_rate, this->state.actual_sample_rate);

            auto status   = ps4000aRunStreaming(this->state.handle,
                                                &unit_int.interval, // sample interval
                                                unit_int.unit,      // time unit of sample interval
                                                0,                  // pre-triggersamples (unused)
                                                static_cast<uint32_t>(driver_buffer_size), false,
                                                1, // downsampling factor // TODO reconsider if we need downsampling support
                                                PS4000A_RATIO_MODE_NONE, static_cast<uint32_t>(driver_buffer_size));

            if (status != PICO_OK) {
                fmt::println(std::cerr, "ps4000aRunStreaming: {}", detail::getErrorMessage(status));
                return { status };
            }
        }

        return {};
    }

    Error
    driver_disarm() noexcept {
        if (const auto status = ps4000aStop(this->state.handle); status != PICO_OK) {
            fmt::println(std::cerr, "ps4000aStop: {}", detail::getErrorMessage(status));
            return { status };
        }

        return {};
    }

    Error
    driver_poll() {
        static auto redirector = [](int16_t handle, int32_t noOfSamples, uint32_t startIndex, int16_t overflow, uint32_t triggerAt, int16_t triggered, int16_t autoStop, void *vobj) {
            std::ignore = handle;
            std::ignore = triggerAt;
            std::ignore = triggered;
            std::ignore = autoStop;
            static_cast<Picoscope4000a *>(vobj)->streamingCallback(noOfSamples, startIndex, overflow);
        };

        const auto status = ps4000aGetStreamingLatestValues(this->state.handle, static_cast<ps4000aStreamingReady>(redirector), this);
        if (status == PICO_BUSY || status == PICO_DRIVER_FUNCTION) {
            return {};
        }
        return { status };
    }
};

} // namespace fair::picoscope

ENABLE_REFLECTION_FOR_TEMPLATE(fair::picoscope::Picoscope4000a, analog_out, serial_number, sample_rate, pre_samples, post_samples, acquisition_mode, rapid_block_nr_captures, streaming_mode_poll_rate,
                               auto_arm, trigger_once, channel_ids, channel_names, channel_units, channel_ranges, channel_offsets, channel_couplings, trigger_source, trigger_threshold,
                               trigger_direction, trigger_pin);

#endif
