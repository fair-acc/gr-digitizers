#include "picoscope5000a_cpu.h"
#include "picoscope5000a_cpu_gen.h"

#include "utils.h"
#include <digitizers/status.h>
#include <picoscopeimpl/status_string.h>

#include <cstring>

using namespace gr::picoscope;
using gr::digitizers::acquisition_mode_t;
using gr::digitizers::channel_status_t;
using gr::digitizers::coupling_t;
using gr::digitizers::downsampling_mode_t;
using gr::digitizers::range_t;
using gr::digitizers::trigger_direction_t;

struct PicoStatus5000aErrc : std::error_category {
    const char* name() const noexcept override;
    std::string message(int ev) const override;
};

const char* PicoStatus5000aErrc::name() const noexcept { return "Ps5000a"; }

std::string PicoStatus5000aErrc::message(int ev) const
{
    PICO_STATUS status = static_cast<PICO_STATUS>(ev);
    return detail::get_error_message(status);
}

const PicoStatus5000aErrc thePsErrCategory{};

/*!
 * This method is needed because PICO_STATUS is not a distinct type (e.g. an enum)
 * therfore we cannot really hook this into the std error code properly.
 */
std::error_code make_pico_5000a_error_code(PICO_STATUS e)
{
    return { static_cast<int>(e), thePsErrCategory };
}

namespace {
boost::mutex g_init_mutex;
}

namespace gr::picoscope5000a {

/*!
 * a structure used for streaming setup
 */
struct ps5000a_unit_interval_t {
    PS5000A_TIME_UNITS unit;
    uint32_t interval;
};

/**********************************************************************
 * Converters - helper functions
 *********************************************************************/

static PS5000A_COUPLING convert_to_ps5000a_coupling(coupling_t coupling)
{
    if (coupling == coupling_t::AC_1M)
        return PS5000A_AC;
    if (coupling == coupling_t::DC_1M)
        return PS5000A_DC;

    throw std::runtime_error(fmt::format("Exception in {}:{}: unsupported coupling mode: {}", __FILE__, __LINE__, static_cast<int>(coupling)));
}

static PS5000A_RANGE convert_to_ps5000a_range(double range)
{
    if (range == 0.01)
        return PS5000A_10MV;
    else if (range == 0.02)
        return PS5000A_20MV;
    else if (range == 0.05)
        return PS5000A_50MV;
    else if (range == 0.1)
        return PS5000A_100MV;
    else if (range == 0.2)
        return PS5000A_200MV;
    else if (range == 0.5)
        return PS5000A_500MV;
    else if (range == 1.0)
        return PS5000A_1V;
    else if (range == 2.0)
        return PS5000A_2V;
    else if (range == 5.0)
        return PS5000A_5V;
    else if (range == 10.0)
        return PS5000A_10V;
    else if (range == 20.0)
        return PS5000A_20V;
    else if (range == 50.0)
        return PS5000A_50V;
    else {
        throw std::runtime_error(
            fmt::format("Exception in {}:{}: Range value not supported: {}",
                        __FILE__,
                        __LINE__,
                        range));
    }
}

void validate_desired_actual_frequency_ps4000(double desired_freq, double actual_freq)
{
    // In order to prevent exceptions/exit due to rounding errors, we dont directly
    // compare actual_freq to desired_freq, but instead allow a difference up to 0.001%
    double max_diff_percentage = 0.001;
    double diff_percent = (actual_freq - desired_freq) * 100 / desired_freq;
    if (abs(diff_percent) > max_diff_percentage) {
        const auto message = fmt::format("Critical Error in {}:{}: Desired and actual frequency do not match. desired: {} actual: {}", __FILE__, __LINE__, desired_freq, actual_freq);
#ifdef PORT_DISABLED // TODO can't access d_logger from free function
        GR_LOG_ERROR(d_logger, message);
#endif
        throw std::runtime_error(message);
    }
}

/*!
 * Note this function has to be called after the call to the ps3000aSetChannel function,
 * that is just befor the arm!!!
 */
uint32_t picoscope_5000a_impl::convert_frequency_to_ps5000a_timebase(double desired_freq,
                                                                     double& actual_freq)
{
    // It is assumed that the timebase is calculated like this:
    // (timebase–2) / 125,000,000
    // e.g. timeebase == 3 --> 8ns sample interval
    //
    // Note, for some devices, the above formula might be wrong! To overcome this
    // limitation we use the ps3000aGetTimebase2 function to find the closest possible
    // timebase. The below timebase estimate is therefore used as a fallback only.
    auto time_interval_ns = 1000000000.0 / desired_freq;
    uint32_t timebase_estimate = (static_cast<uint32_t>(time_interval_ns) / 8) + 2;

    // In order to cover for all possible 30000 series devices, we use ps3000aGetTimebase2
    // function to get step size in ns between timebase 3 and 4. Based on that the actual
    // timebase is calculated.
    int32_t dummy;
    std::array<float, 2> time_interval_ns_34;

    for (auto i = 0; i < 2; i++) {
        auto status = ps5000aGetTimebase2(
            d_handle, 3 + i, 1024, &time_interval_ns_34[i], &dummy, 0);
        if (status != PICO_OK) {
            d_logger->notice(
                "timebase cannot be obtained: {}    estimated timebase will be used...",
                detail::get_error_message(status));

            float time_interval_ns;
            status = ps5000aGetTimebase2(
                d_handle, timebase_estimate, 1024, &time_interval_ns, &dummy, 0);
            if (status != PICO_OK) {
                throw std::runtime_error(
                    fmt::format("Exception in {}:{}: local time {} Error: {}",
                                __FILE__,
                                __LINE__,
                                timebase_estimate,
                                detail::get_error_message(status)));
            }

            actual_freq = 1000000000.0 / time_interval_ns;
            validate_desired_actual_frequency_ps4000(desired_freq, actual_freq);
            return timebase_estimate;
        }
    }

    // Calculate steps between timebase 3 and 4 and correct start_timebase estimate based
    // on that
    auto step = time_interval_ns_34[1] - time_interval_ns_34[0];
    timebase_estimate =
        static_cast<uint32_t>((time_interval_ns - time_interval_ns_34[0]) / step) + 3;

    // The below code iterates trought the neighbouring timebases in order to find the
    // best match. In principle we could check only timebases on the left and right but
    // since first three timebases are in most cases special we make search space a bit
    // bigger.
    const int search_space = 8;
    std::array<float, search_space> timebases;
    std::array<float, search_space> error_estimates;

    uint32_t start_timebase = timebase_estimate > (search_space / 2)
                                  ? timebase_estimate - (search_space / 2)
                                  : 0;

    for (auto i = 0; i < search_space; i++) {
        float obtained_time_interval_ns;
        auto status = ps5000aGetTimebase2(
            d_handle, start_timebase + i, 1024, &obtained_time_interval_ns, &dummy, 0);
        if (status != PICO_OK) {
            // this timebase can't be used, lets set error estimate to something big
            timebases[i] = -1;
            error_estimates[i] = 10000000000.0;
        }
        else {
            timebases[i] = obtained_time_interval_ns;
            error_estimates[i] = fabs(time_interval_ns - obtained_time_interval_ns);
        }
    }

    auto it = std::min_element(&error_estimates[0],
                               &error_estimates[0] + error_estimates.size());
    auto distance = std::distance(&error_estimates[0], it);

    assert(distance < search_space);

    // update actual update rate and return timebase number
    actual_freq = 1000000000.0 / double(timebases[distance]);
    validate_desired_actual_frequency_ps4000(desired_freq, actual_freq);
    return start_timebase + distance;
}

ps5000a_unit_interval_t
convert_frequency_to_ps5000a_time_units_and_interval(double desired_freq,
                                                     double& actual_freq)
{
    ps5000a_unit_interval_t unint;
    auto interval = 1.0 / desired_freq;

    if (interval < 0.000001) {
        unint.unit = PS5000A_PS;
        unint.interval = static_cast<uint32_t>(1000000000000.0 / desired_freq);
        actual_freq = 1000000000000.0 / static_cast<double>(unint.interval);
    }
    else if (interval < 0.001) {
        unint.unit = PS5000A_NS;
        unint.interval = static_cast<uint32_t>(1000000000.0 / desired_freq);
        actual_freq = 1000000000.0 / static_cast<double>(unint.interval);
    }
    else if (interval < 0.1) {
        unint.unit = PS5000A_US;
        unint.interval = static_cast<uint32_t>(1000000.0 / desired_freq);
        actual_freq = 1000000.0 / static_cast<double>(unint.interval);
    }
    else {
        unint.unit = PS5000A_MS;
        unint.interval = static_cast<uint32_t>(1000.0 / desired_freq);
        actual_freq = 1000.0 / static_cast<double>(unint.interval);
    }
    validate_desired_actual_frequency_ps4000(desired_freq, actual_freq);
    return unint;
}

static PS5000A_RATIO_MODE convert_to_ps5000a_ratio_mode(downsampling_mode_t mode)
{
    switch (mode) {
    case downsampling_mode_t::MIN_MAX_AGG:
        return PS5000A_RATIO_MODE_AGGREGATE;
    case downsampling_mode_t::DECIMATE:
        return PS5000A_RATIO_MODE_DECIMATE;
    case downsampling_mode_t::AVERAGE:
        return PS5000A_RATIO_MODE_AVERAGE;
    case downsampling_mode_t::NONE:
    default:
        return PS5000A_RATIO_MODE_NONE;
    }
}

PS5000A_THRESHOLD_DIRECTION
convert_to_ps5000a_threshold_direction(trigger_direction_t direction)
{
    switch (direction) {
    case trigger_direction_t::RISING:
        return PS5000A_RISING;
    case trigger_direction_t::FALLING:
        return PS5000A_FALLING;
    case trigger_direction_t::LOW:
        return PS5000A_BELOW;
    case trigger_direction_t::HIGH:
        return PS5000A_ABOVE;
    default:
        throw std::runtime_error(fmt::format("Exception in {}:{}: unsupported trigger direction: {}",
                                 __FILE__,
                                 __LINE__,
                                 static_cast<int>(direction)));
    }
};

int16_t convert_voltage_to_ps5000a_raw_logic_value(double value)
{
    double max_logical_voltage = 5.0;

    if (value > max_logical_voltage) {
        throw std::invalid_argument(fmt::format("Exception in {}:{}: max logical level is: {}", __FILE__, __LINE__, max_logical_voltage));
    }
    // Note max channel value not provided with PicoScope API, we use ext max value
    return (int16_t)((value / max_logical_voltage) * (double)PS5000A_EXT_MAX_VALUE);
}

PS5000A_CHANNEL
convert_to_ps5000a_channel(const std::string& source)
{
    if (source == "A") {
        return PS5000A_CHANNEL_A;
    }
    else if (source == "B") {
        return PS5000A_CHANNEL_B;
    }
    else if (source == "C") {
        return PS5000A_CHANNEL_C;
    }
    else if (source == "D") {
        return PS5000A_CHANNEL_D;
    }
    else if (source == "EXTERNAL") {
        return PS5000A_EXTERNAL;
    }
    else {
        // return invalid value
        return PS5000A_MAX_TRIGGER_SOURCES;
    }
}

/**********************************************************************
 * Structors
 *********************************************************************/

picoscope_5000a_impl::picoscope_5000a_impl(const digitizers::digitizer_args& args,
                                           digitizer_resolution_t resolution,
                                           std::string serial_number,
                                           logger_ptr logger)
    : digitizers::picoscope_impl(args, serial_number, 255, 0.01, logger),
      d_handle(-1),
      d_overflow(0),
      d_resolution(resolution)
{
    d_ranges.push_back(range_t(0.01));
    d_ranges.push_back(range_t(0.02));
    d_ranges.push_back(range_t(0.05));
    d_ranges.push_back(range_t(0.1));
    d_ranges.push_back(range_t(0.2));
    d_ranges.push_back(range_t(0.5));
    d_ranges.push_back(range_t(1));
    d_ranges.push_back(range_t(2));
    d_ranges.push_back(range_t(5));
    d_ranges.push_back(range_t(10));
    d_ranges.push_back(range_t(20));
    d_ranges.push_back(range_t(50));
    d_ranges.push_back(range_t(100));
    d_ranges.push_back(range_t(200));
}

picoscope_5000a_impl::~picoscope_5000a_impl() { driver_close(); }

/**********************************************************************
 * Driver implementation
 *********************************************************************/

std::string picoscope_5000a_impl::get_unit_info_topic(PICO_INFO info) const
{
    char line[40];
    int16_t required_size;

    auto status = ps5000aGetUnitInfo(
        d_handle, reinterpret_cast<int8_t*>(line), sizeof(line), &required_size, info);
    if (status == PICO_OK) {
        return std::string(line, required_size);
    }
    else {
        return "NA";
    }
}

std::string picoscope_5000a_impl::get_driver_version() const
{
    const std::string prefix = "PS5000A Linux Driver, ";
    auto version = get_unit_info_topic(PICO_DRIVER_VERSION);

    auto i = version.find(prefix);
    if (i != std::string::npos)
        version.erase(i, prefix.length());

    return version;
}

std::string picoscope_5000a_impl::get_hardware_version() const
{
    if (!d_initialized)
        return "NA";
    return get_unit_info_topic(PICO_HARDWARE_VERSION);
}

static constexpr PS5000A_DEVICE_RESOLUTION map_resolution(digitizer_resolution_t r)
{
    switch (r) {
    case digitizer_resolution_t::R_8BIT:
        return PS5000A_DR_8BIT;
    case digitizer_resolution_t::R_12BIT:
        return PS5000A_DR_12BIT;
    case digitizer_resolution_t::R_14BIT:
        return PS5000A_DR_14BIT;
    case digitizer_resolution_t::R_15BIT:
        return PS5000A_DR_15BIT;
    case digitizer_resolution_t::R_16BIT:
        return PS5000A_DR_16BIT;
    }

    return PS5000A_DR_8BIT;
}

std::error_code picoscope_5000a_impl::driver_initialize()
{
    PICO_STATUS status;

    // Required to force sequence execution of open unit calls...
    boost::mutex::scoped_lock init_guard(g_init_mutex);

    const auto resolution = map_resolution(d_resolution);

    // take any if serial number is not provided (usefull for testing purposes)
    if (d_serial_number.empty()) {
        status = ps5000aOpenUnit(&(d_handle), NULL, resolution);
    }
    else {
        status = ps5000aOpenUnit(&(d_handle), (int8_t*)d_serial_number.c_str(), resolution);
    }

    // ignore ext. power not connected error/warning
    if (status == PICO_POWER_SUPPLY_NOT_CONNECTED ||
        status == PICO_USB3_0_DEVICE_NON_USB3_0_PORT) {
        status = ps5000aChangePowerSource(d_handle, status);
        if (status == PICO_POWER_SUPPLY_NOT_CONNECTED ||
            status == PICO_USB3_0_DEVICE_NON_USB3_0_PORT) {
            status = ps5000aChangePowerSource(d_handle, status);
        }
    }

    if (status != PICO_OK) {
        d_logger->error("open unit failed: {} ", detail::get_error_message(status));
        return make_pico_5000a_error_code(status);
    }

    // maximum value is used for conversion to volts
    status = ps5000aMaximumValue(d_handle, &d_max_value);
    if (status != PICO_OK) {
        ps5000aCloseUnit(d_handle);
        d_logger->error("ps5000aMaximumValue: {}", detail::get_error_message(status));
        return make_pico_5000a_error_code(status);
    }

    return std::error_code{};
}

std::error_code picoscope_5000a_impl::driver_configure()
{
    assert(d_ai_channels <= PS5000A_MAX_CHANNELS);

    int32_t max_samples;
    PICO_STATUS status = ps5000aMemorySegments(d_handle, d_nr_captures, &max_samples);
    if (status != PICO_OK) {
        d_logger->error("ps5000aMemorySegments: {}", detail::get_error_message(status));
        return make_pico_5000a_error_code(status);
    }

    if (d_acquisition_mode == acquisition_mode_t::RAPID_BLOCK) {
        status = ps5000aSetNoOfCaptures(d_handle, d_nr_captures);
        if (status != PICO_OK) {
            d_logger->error("ps5000aSetNoOfCaptures: {}",
                            detail::get_error_message(status));
            return make_pico_5000a_error_code(status);
        }
    }

    // configure analog channels
    for (auto i = 0; i < d_ai_channels; i++) {
        auto enabled = d_channel_settings[i].enabled;
        auto coupling = convert_to_ps5000a_coupling(d_channel_settings[i].coupling);
        auto range = convert_to_ps5000a_range(d_channel_settings[i].range);
        auto offset = d_channel_settings[i].offset;

        status = ps5000aSetChannel(d_handle,
                                   static_cast<PS5000A_CHANNEL>(i),
                                   enabled,
                                   coupling,
                                   range,
                                   offset);
        if (status != PICO_OK) {
            d_logger->error(
                "ps5000aSetChannel (chan {}): {}", i, detail::get_error_message(status));
            return make_pico_5000a_error_code(status);
        }
    }

    // apply trigger configuration
    if (d_trigger_settings.is_analog() &&
        d_acquisition_mode == acquisition_mode_t::RAPID_BLOCK) {
        status = ps5000aSetSimpleTrigger(
            d_handle,
            true, // enable
            convert_to_ps5000a_channel(d_trigger_settings.source),
            convert_voltage_to_ps5000a_raw_logic_value(d_trigger_settings.threshold),
            convert_to_ps5000a_threshold_direction(d_trigger_settings.direction),
            0,   // delay
            -1); // auto trigger
        if (status != PICO_OK) {
            d_logger->error("ps5000aSetSimpleTrigger: {}",
                            detail::get_error_message(status));
            return make_pico_5000a_error_code(status);
        }
    }
    else {
        // disable triggers...
        for (int i = 0; i < PS5000A_MAX_CHANNELS; i++) {
            PS5000A_CONDITION cond;
            cond.source = static_cast<PS5000A_CHANNEL>(i);
            cond.condition = PS5000A_CONDITION_DONT_CARE;
            status =
                ps5000aSetTriggerChannelConditionsV2(d_handle, &cond, 1, PS5000A_CLEAR);
            if (status != PICO_OK) {
                d_logger->error("ps5000aSetTriggerChannelConditionsV2: {}",
                                detail::get_error_message(status));
                return make_pico_5000a_error_code(status);
            }
        }
    }

    // In order to validate desired frequency before startup
    double actual_freq;
    convert_frequency_to_ps5000a_timebase(d_samp_rate, actual_freq);

    return std::error_code{};
}

void rapid_block_callback_redirector_5000a(int16_t handle, PICO_STATUS status, void* vobj)
{
    static_cast<picoscope_5000a_impl*>(vobj)->rapid_block_callback(handle, status);
}

void picoscope_5000a_impl::rapid_block_callback(int16_t handle, PICO_STATUS status)
{
    auto errc = make_pico_5000a_error_code(status);
    notify_data_ready(errc);
}

std::error_code picoscope_5000a_impl::driver_arm()
{
    if (d_acquisition_mode == acquisition_mode_t::RAPID_BLOCK) {
        uint32_t timebase =
            convert_frequency_to_ps5000a_timebase(d_samp_rate, d_actual_samp_rate);

        auto status =
            ps5000aRunBlock(d_handle,
                            d_pre_samples,  // pre-triggersamples
                            d_post_samples, // post-trigger samples
                            timebase,       // timebase
                            NULL,           // time indispossed
                            0,              // segment index
                            (ps5000aBlockReady)rapid_block_callback_redirector_5000a,
                            this);
        if (status != PICO_OK) {
            d_logger->error("ps5000aRunBlock: {}", detail::get_error_message(status));
            return make_pico_5000a_error_code(status);
        }
    }
    else {
        set_buffers(d_driver_buffer_size, 0);

        ps5000a_unit_interval_t unit_int =
            convert_frequency_to_ps5000a_time_units_and_interval(d_samp_rate,
                                                                 d_actual_samp_rate);

        auto status =
            ps5000aRunStreaming(d_handle,
                                &(unit_int.interval), // sample interval
                                unit_int.unit,        // time unit of sample interval
                                0,                    // pre-triggersamples (unused)
                                d_driver_buffer_size,
                                false,
                                d_downsampling_factor,
                                convert_to_ps5000a_ratio_mode(d_downsampling_mode),
                                d_driver_buffer_size);

        if (status != PICO_OK) {
            d_logger->error("ps5000aRunStreaming: {}", detail::get_error_message(status));
            return make_pico_5000a_error_code(status);
        }
    }

    return std::error_code{};
}

std::error_code picoscope_5000a_impl::driver_disarm()
{
    auto status = ps5000aStop(d_handle);
    if (status != PICO_OK) {
        d_logger->error("ps5000aStop: {}", detail::get_error_message(status));
    }

    return make_pico_5000a_error_code(status);
}

std::error_code picoscope_5000a_impl::driver_close()
{
    if (d_handle == -1) {
        return std::error_code{};
    }

    auto status = ps5000aCloseUnit(d_handle);
    if (status != PICO_OK) {
        d_logger->error("ps5000aCloseUnit: {}", detail::get_error_message(status));
    }

    d_handle = -1;
    return make_pico_5000a_error_code(status);
}

std::error_code picoscope_5000a_impl::set_buffers(size_t samples, uint32_t block_number)
{
    PICO_STATUS status;

    for (auto aichan = 0; aichan < d_ai_channels; aichan++) {
        if (!d_channel_settings[aichan].enabled)
            continue;

        if (d_downsampling_mode == downsampling_mode_t::MIN_MAX_AGG) {
            d_buffers[aichan].reserve(samples);
            d_buffers_min[aichan].reserve(samples);

            status =
                ps5000aSetDataBuffers(d_handle,
                                      static_cast<PS5000A_CHANNEL>(aichan),
                                      &d_buffers[aichan][0],
                                      &d_buffers_min[aichan][0],
                                      samples,
                                      block_number,
                                      convert_to_ps5000a_ratio_mode(d_downsampling_mode));
        }
        else {
            d_buffers[aichan].reserve(samples);

            status =
                ps5000aSetDataBuffer(d_handle,
                                     static_cast<PS5000A_CHANNEL>(aichan),
                                     &d_buffers[aichan][0],
                                     samples,
                                     block_number,
                                     convert_to_ps5000a_ratio_mode(d_downsampling_mode));
        }

        if (status != PICO_OK) {
            d_logger->error("ps5000aSetDataBuffer (chan {}): {}",
                            aichan,
                            detail::get_error_message(status));
            return make_pico_5000a_error_code(status);
        }
    }

    return std::error_code{};
}

std::error_code picoscope_5000a_impl::driver_prefetch_block(size_t samples,
                                                            size_t block_number)
{
    auto erc = set_buffers(samples, block_number);
    if (erc) {
        return erc;
    }

    uint32_t nr_samples = samples;
    auto status = ps5000aGetValues(d_handle,
                                   0, // offset
                                   &nr_samples,
                                   d_downsampling_factor,
                                   convert_to_ps5000a_ratio_mode(d_downsampling_mode),
                                   block_number,
                                   &d_overflow);
    if (status != PICO_OK) {
        d_logger->error("ps5000aGetValues: {}", detail::get_error_message(status));
    }

    return make_pico_5000a_error_code(status);
}

std::error_code
picoscope_5000a_impl::driver_get_rapid_block_data(size_t offset,
                                                  size_t length,
                                                  size_t waveform,
                                                  work_io& wio,
                                                  std::vector<uint32_t>& status)
{
    int vec_index = 0;

    for (auto chan_idx = 0; chan_idx < d_ai_channels; chan_idx++, vec_index += 2) {
        if (!d_channel_settings[chan_idx].enabled) {
            continue;
        }

        if (d_overflow & (1 << chan_idx)) {
            status[chan_idx] = channel_status_t::CHANNEL_STATUS_OVERFLOW;
        }
        else {
            status[chan_idx] = 0;
        }

        float voltage_multiplier =
            d_channel_settings[chan_idx].range / (float)d_max_value;

        float* out = wio.outputs()[vec_index].items<float>();
        float* err_out = wio.outputs()[vec_index + 1].items<float>();
        int16_t* in = &d_buffers[chan_idx][0] + offset;

        if (d_downsampling_mode == downsampling_mode_t::NONE ||
            d_downsampling_mode == downsampling_mode_t::DECIMATE) {
            for (size_t i = 0; i < length; i++) {
                out[i] = (voltage_multiplier * (float)in[i]);
            }
            // According to specs
            auto error_estimate = d_channel_settings[chan_idx].range * 0.01;
            for (size_t i = 0; i < length; i++) {
                err_out[i] = error_estimate;
            }
        }
        else if (d_downsampling_mode == downsampling_mode_t::MIN_MAX_AGG) {
            // this mode is different because samples are in two distinct buffers
            int16_t* in_min = &d_buffers_min[chan_idx][0] + offset;

            for (size_t i = 0; i < length; i++) {
                auto max = (voltage_multiplier * (float)in[i]);
                auto min = (voltage_multiplier * (float)in_min[i]);

                out[i] = (max + min) / 2.0;
                err_out[i] = (max - min) / 4.0;
            }
        }
        else if (d_downsampling_mode == downsampling_mode_t::AVERAGE) {
            for (size_t i = 0; i < length; i++) {
                out[i] = (voltage_multiplier * (float)in[i]);
            }
            // According to specs
            float error_estimate_single = d_channel_settings[chan_idx].range * 0.01;
            float error_estimate =
                error_estimate_single / std::sqrt((float)d_downsampling_factor);
            for (size_t i = 0; i < length; i++) {
                err_out[i] = error_estimate;
            }
        }
        else {
            assert(false);
        }
    }

    return std::error_code{};
}

std::error_code picoscope_5000a_impl::driver_poll()
{
    auto status = ps5000aGetStreamingLatestValues(
        d_handle,
        (ps5000aStreamingReady)digitizers::invoke_streaming_callback,
        &d_streaming_callback);
    if (status == PICO_BUSY || status == PICO_DRIVER_FUNCTION) {
        return std::error_code{};
    }
    return make_pico_5000a_error_code(status);
}

picoscope5000a_cpu::picoscope5000a_cpu(block_args args)
    : INHERITED_CONSTRUCTORS,
      d_impl(
          { .sample_rate = args.sample_rate,
            .buffer_size = args.buffer_size,
            .nr_buffers = args.nr_buffers,
            .driver_buffer_size = args.driver_buffer_size,
            .pre_samples = args.pre_samples,
            .post_samples = args.post_samples,
            // TODO(PORT) these enums are to be assumed identical, find out how we can
            // share enums between blocks without linking errors
            .acquisition_mode = static_cast<acquisition_mode_t>(args.acquisition_mode),
            .rapid_block_nr_captures = args.rapid_block_nr_captures,
            .streaming_mode_poll_rate = args.streaming_mode_poll_rate,
            .downsampling_mode = static_cast<downsampling_mode_t>(args.downsampling_mode),
            .downsampling_factor = args.downsampling_factor,
            .auto_arm = args.auto_arm,
            .trigger_once = args.trigger_once,
            .ai_channels = PS5000A_MAX_CHANNELS,
            .ports = 0 },
          args.resolution,
          args.serial_number,
          d_logger)
{
    set_output_multiple(args.buffer_size);
}

bool picoscope5000a_cpu::start() { return d_impl.start(); }

bool picoscope5000a_cpu::stop() { return d_impl.stop(); }

work_return_t picoscope5000a_cpu::work(work_io& wio) { return d_impl.work(wio); }

void picoscope5000a_cpu::initialize() { d_impl.initialize(); }

void picoscope5000a_cpu::close() { d_impl.close(); }

} // namespace gr::picoscope5000a
