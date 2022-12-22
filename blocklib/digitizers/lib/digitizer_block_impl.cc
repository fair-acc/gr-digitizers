#include "digitizer_block_impl.h"
#include "tags.h"
#include "utils.h"

#include <boost/lexical_cast.hpp>

#include <chrono>
#include <thread>

namespace gr::digitizers {

/**********************************************************************
 * Error codes
 *********************************************************************/

struct digitizer_block_err_category : std::error_category {
    const char* name() const noexcept override;
    std::string message(int ev) const override;
};

const char* digitizer_block_err_category::name() const noexcept
{
    return "digitizer_block";
}

std::string digitizer_block_err_category::message(int ev) const
{
    switch (static_cast<digitizer_block_errc>(ev)) {
    case digitizer_block_errc::Interrupted:
        return "Wit interrupted";

    default:
        return "(unrecognized error)";
    }
}

const digitizer_block_err_category __digitizer_block_category{};

std::error_code make_error_code(digitizer_block_errc e)
{
    return { static_cast<int>(e), __digitizer_block_category };
}

/**********************************************************************
 * Structors
 *********************************************************************/

// Relevant for debugging. If gnuradio calls this block not often enough, we will get
// "WARN: XX digitizer data buffers lost" The rate in which this block is called is given
// by the number of free slots on its output buffer. So we choose a big value via
// set_min_output_buffer
uint64_t last_call_utc = 0;

static const int AVERAGE_HISTORY_LENGTH = 100000;

digitizer_block_impl::~digitizer_block_impl() {}

digitizer_block_impl::digitizer_block_impl(const digitizer_args& args,
                                           gr::logger_ptr logger)
    : d_logger{ std::move(logger) },
      d_samp_rate(args.sample_rate),
      d_actual_samp_rate(d_samp_rate),
      d_time_per_sample_ns(1000000000. / d_samp_rate),
      d_pre_samples(args.pre_samples),
      d_post_samples(args.post_samples),
      d_nr_captures(args.rapid_block_nr_captures),
      d_buffer_size(args.buffer_size),
      d_nr_buffers(args.nr_buffers),
      d_driver_buffer_size(args.driver_buffer_size),
      d_acquisition_mode(args.acquisition_mode),
      d_poll_rate(args.streaming_mode_poll_rate),
      d_downsampling_mode(args.downsampling_mode),
      d_downsampling_factor(args.downsampling_factor),
      d_ai_channels(args.ai_channels),
      d_ports(args.ports),
      d_channel_settings(),
      d_port_settings(),
      d_trigger_settings(),
      d_status(args.ai_channels),
      d_app_buffer(),
      d_was_last_callback_timestamp_taken(false),
      d_estimated_sample_rate(AVERAGE_HISTORY_LENGTH),
      d_initialized(false),
      d_closed(false),
      d_armed(false),
      d_auto_arm(args.auto_arm),
      d_trigger_once(args.trigger_once),
      d_was_triggered_once(false),
      d_timebase_published(false),
      d_data_rdy(false),
      d_trigger_state(0),
      d_read_idx(0),
      d_buffer_samples(0),
      d_errors(128),
      d_poller_state(poller_state_t::IDLE)
{
    d_ai_buffers = std::vector<std::vector<float>>(d_ai_channels);
    d_ai_error_buffers = std::vector<std::vector<float>>(d_ai_channels);

    if (d_acquisition_mode == acquisition_mode_t::RAPID_BLOCK) {
        if (args.post_samples < 1) {
            throw std::invalid_argument(fmt::format(
                "Exception in {}:{}: post-trigger samples can't be less than one",
                __FILE__,
                __LINE__));
        }
        if (args.rapid_block_nr_captures < 1) {
            throw std::invalid_argument(
                fmt::format("Exception in {}:{}: nr waveforms should be at least one: {}",
                            __FILE__,
                            __LINE__,
                            args.rapid_block_nr_captures));
        }
        d_buffer_size = args.pre_samples + args.post_samples;
    }
    else { // streaming mode
        if (args.streaming_mode_poll_rate < 0.0) {
            throw std::invalid_argument(
                fmt::format("Exception in {}:{}: poll rate can't be negative: {}",
                            __FILE__,
                            __LINE__,
                            args.streaming_mode_poll_rate));
        }
    }

    if (args.sample_rate <= 0) {
        throw std::invalid_argument(
            fmt::format("Exception in {}:{}: sample rate has to be greater than zero",
                        __FILE__,
                        __LINE__));
    }

    if (args.nr_buffers == 0) {
        throw std::invalid_argument(fmt::format(
            "Exception in {}:{}: number of buffers cannot be zero", __FILE__, __LINE__));
    }

    if (args.driver_buffer_size == 0) {
        throw std::invalid_argument(fmt::format(
            "Exception in {}:{}: driver buffer size cannot be zero", __FILE__, __LINE__));
    }

    if (args.downsampling_mode != downsampling_mode_t::NONE &&
        args.downsampling_factor < 2) {
        throw std::invalid_argument(fmt::format(
            "Exception in {}:{}: downsampling factor should be at least 2: {}",
            __FILE__,
            __LINE__,
            args.downsampling_factor));
    }

    if (args.ports) {
        d_port_buffers = std::vector<std::vector<uint8_t>>(args.ports);
    }

    assert(d_ai_channels < MAX_SUPPORTED_AI_CHANNELS);
    assert(d_ports < MAX_SUPPORTED_PORTS);
}

/**********************************************************************
 * Helpers
 **********************************************************************/

uint32_t digitizer_block_impl::get_pre_trigger_samples_with_downsampling() const
{
    if (d_downsampling_mode != downsampling_mode_t::NONE)
        return d_pre_samples / d_downsampling_factor;
    return d_pre_samples;
}

uint32_t digitizer_block_impl::get_post_trigger_samples_with_downsampling() const
{
    if (d_downsampling_mode != downsampling_mode_t::NONE)
        return d_post_samples / d_downsampling_factor;
    return d_post_samples;
}

uint32_t digitizer_block_impl::get_block_size() const
{
    return d_post_samples + d_pre_samples;
}

uint32_t digitizer_block_impl::get_block_size_with_downsampling() const
{
    return get_pre_trigger_samples_with_downsampling() +
           get_post_trigger_samples_with_downsampling();
}

double digitizer_block_impl::get_timebase_with_downsampling() const
{
    if (d_downsampling_mode == downsampling_mode_t::NONE) {
        return 1.0 / d_actual_samp_rate;
    }
    else {
        return d_downsampling_factor / d_actual_samp_rate;
    }
}

void digitizer_block_impl::add_error_code(std::error_code ec) { d_errors.push(ec); }

std::vector<std::size_t>
digitizer_block_impl::find_analog_triggers(std::span<const float> samples)
{
    std::vector<std::size_t> trigger_offsets; // relative offset of detected triggers

    if (!d_trigger_settings.is_enabled() || samples.empty()) {
        return trigger_offsets;
    }

    assert(d_trigger_settings.is_analog());

    auto aichan = convert_to_aichan_idx(d_trigger_settings.source);

    if (d_trigger_settings.direction == trigger_direction_t::RISING ||
        d_trigger_settings.direction == trigger_direction_t::HIGH) {
        float band = d_channel_settings[aichan].range / 100.0;
        float lo = static_cast<float>(d_trigger_settings.threshold - band);

        for (std::size_t i = 0; i < samples.size(); i++) {
            if (!d_trigger_state && samples[i] >= d_trigger_settings.threshold) {
                d_trigger_state = 1;
                trigger_offsets.push_back(i);
            }
            else if (d_trigger_state && samples[i] <= lo) {
                d_trigger_state = 0;
            }
        }
    }
    else if (d_trigger_settings.direction == trigger_direction_t::FALLING ||
             d_trigger_settings.direction == trigger_direction_t::LOW) {
        float band = d_channel_settings[aichan].range / 100.0;
        float hi = static_cast<float>(d_trigger_settings.threshold + band);

        for (std::size_t i = 0; i < samples.size(); i++) {
            if (d_trigger_state && samples[i] <= d_trigger_settings.threshold) {
                d_trigger_state = 0;
                trigger_offsets.push_back(i);
            }
            else if (!d_trigger_state && samples[i] >= hi) {
                d_trigger_state = 1;
            }
        }
    }

    return trigger_offsets;
}

std::vector<std::size_t>
digitizer_block_impl::find_digital_triggers(std::span<const uint8_t> samples,
                                            uint8_t mask)
{
    std::vector<std::size_t> trigger_offsets;

    if (d_trigger_settings.direction == trigger_direction_t::RISING ||
        d_trigger_settings.direction == trigger_direction_t::HIGH) {
        for (std::size_t i = 0; i < samples.size(); i++) {
            if (!d_trigger_state && (samples[i] & mask)) {
                d_trigger_state = 1;
                trigger_offsets.push_back(i);
            }
            else if (d_trigger_state && !(samples[i] & mask)) {
                d_trigger_state = 0;
            }
        }
    }
    else if (d_trigger_settings.direction == trigger_direction_t::FALLING ||
             d_trigger_settings.direction == trigger_direction_t::LOW) {
        for (std::size_t i = 0; i < samples.size(); i++) {
            if (d_trigger_state && !(samples[i] & mask)) {
                d_trigger_state = 0;
                trigger_offsets.push_back(i);
            }
            else if (!d_trigger_state && (samples[i] & mask)) {
                d_trigger_state = 1;
            }
        }
    }

    return trigger_offsets;
}

/**********************************************************************
 * Public API
 **********************************************************************/

acquisition_mode_t digitizer_block_impl::get_acquisition_mode() const
{
    return d_acquisition_mode;
}

double digitizer_block_impl::get_samp_rate() const { return d_actual_samp_rate; }

int digitizer_block_impl::convert_to_aichan_idx(const std::string& id) const
{
    if (id.length() != 1) {
        std::ostringstream message;
        message << "Exception in " << __FILE__ << ":" << __LINE__
                << ": aichan id should be a single character: " << id;
        throw std::invalid_argument(message.str());
    }

    int idx = std::toupper(id[0]) - 'A';
    if (idx < 0 || idx > MAX_SUPPORTED_AI_CHANNELS) {
        std::ostringstream message;
        message << "Exception in " << __FILE__ << ":" << __LINE__
                << ": invalid aichan id: " << id;
        throw std::invalid_argument(message.str());
    }

    return idx;
}

void digitizer_block_impl::set_aichan(const std::string& id,
                                      bool enabled,
                                      double range,
                                      coupling_t coupling,
                                      double range_offset)
{
    auto idx = convert_to_aichan_idx(id);
    d_channel_settings[idx].range = range;
    d_channel_settings[idx].offset = range_offset;
    d_channel_settings[idx].enabled = enabled;
    d_channel_settings[idx].coupling = coupling;
}

int digitizer_block_impl::get_enabled_aichan_count() const
{
    auto count = 0;
    for (const auto& c : d_channel_settings) {
        count += c.enabled;
    }
    return count;
}

void digitizer_block_impl::set_aichan_range(const std::string& id,
                                            double range,
                                            double range_offset)
{
    auto idx = convert_to_aichan_idx(id);
    d_channel_settings[idx].range = range;
    d_channel_settings[idx].offset = range_offset;
}

void digitizer_block_impl::set_aichan_trigger(const std::string& id,
                                              trigger_direction_t direction,
                                              double threshold)
{
    // Some scopes have an dedicated AUX Trigger-Input. Skip id verification for them
    if (id != "AUX")
        convert_to_aichan_idx(id); // Just to verify id

    d_trigger_settings.source = id;
    d_trigger_settings.threshold = threshold;
    d_trigger_settings.direction = direction;
    d_trigger_settings.pin_number = 0; // not used
}

int digitizer_block_impl::convert_to_port_idx(const std::string& id) const
{
    if (id.length() != 5) {
        std::ostringstream message;
        message << "Exception in " << __FILE__ << ":" << __LINE__
                << ": invalid port id: " << id
                << ", should be of the following format 'port<d>'";
        throw std::invalid_argument(message.str());
    }

    int idx = boost::lexical_cast<int>(id[4]);
    if (idx < 0 || idx > MAX_SUPPORTED_PORTS) {
        std::ostringstream message;
        message << "Exception in " << __FILE__ << ":" << __LINE__
                << ": invalid port number: " << id;
        throw std::invalid_argument(message.str());
    }

    return idx;
}

void digitizer_block_impl::set_diport(const std::string& id,
                                      bool enabled,
                                      double thresh_voltage)
{
    auto port_number = convert_to_port_idx(id);

    d_port_settings[port_number].logic_level = thresh_voltage;
    d_port_settings[port_number].enabled = enabled;
}

int digitizer_block_impl::get_enabled_diport_count() const
{
    auto count = 0;
    for (const auto& p : d_port_settings) {
        count += p.enabled;
    }
    return count;
}

void digitizer_block_impl::set_di_trigger(uint32_t pin, trigger_direction_t direction)
{
    d_trigger_settings.source = TRIGGER_DIGITAL_SOURCE;
    d_trigger_settings.threshold = 0.0; // not used
    d_trigger_settings.direction = direction;
    d_trigger_settings.pin_number = pin;
}

void digitizer_block_impl::disable_triggers()
{
    d_trigger_settings.source = TRIGGER_NONE_SOURCE;
}

void digitizer_block_impl::initialize()
{
    if (d_initialized) {
        return;
    }

    auto ec = driver_initialize();
    if (ec) {
        add_error_code(ec);
        std::ostringstream message;
        message << "Exception in " << __FILE__ << ":" << __LINE__
                << ": initialize failed. ErrorCode: " << ec;
        throw std::runtime_error(message.str());
    }

    d_initialized = true;
}

void digitizer_block_impl::configure()
{
    if (!d_initialized) {
        std::ostringstream message;
        message << "Exception in " << __FILE__ << ":" << __LINE__
                << ": initialize first!";
        throw std::runtime_error(message.str());
    }

    if (d_armed) {
        std::ostringstream message;
        message << "Exception in " << __FILE__ << ":" << __LINE__ << ": disarm first!";
        throw std::runtime_error(message.str());
    }

    auto ec = driver_configure();
    if (ec) {
        add_error_code(ec);
        std::ostringstream message;
        message << "Exception in " << __FILE__ << ":" << __LINE__
                << ": configure failed. ErrorCode: " << ec;
        throw std::runtime_error(message.str());
    }
    // initialize application buffer
    d_app_buffer.initialize(get_enabled_aichan_count(),
                            get_enabled_diport_count(),
                            d_buffer_size,
                            d_nr_buffers);
}

void digitizer_block_impl::arm()
{
    if (d_armed) {
        return;
    }

    // set estimated sample rate to expected
    float expected = static_cast<float>(get_samp_rate());
    for (auto i = 0; i < AVERAGE_HISTORY_LENGTH; i++) {
        d_estimated_sample_rate.add(expected);
    }

    // arm the driver
    auto ec = driver_arm();
    if (ec) {
        add_error_code(ec);
        std::ostringstream message;
        message << "Exception in " << __FILE__ << ":" << __LINE__
                << ": arm failed. ErrorCode: " << ec;
        throw std::runtime_error(message.str());
    }

    d_armed = true;
    d_timebase_published = false;
    d_was_last_callback_timestamp_taken = false;

    // clear error condition in the application buffer
    d_app_buffer.notify_data_ready(std::error_code{});

    // notify poll thread to start with the poll request
    if (d_acquisition_mode == acquisition_mode_t::STREAMING) {
        transit_poll_thread_to_running();
    }

    // allocate buffer pointer vectors.
    int num_enabled_ai_channels = 0;
    int num_enabled_di_ports = 0;
    for (auto i = 0; i < d_ai_channels; i++) {
        if (d_channel_settings[i].enabled) {
            num_enabled_ai_channels++;
        }
    }
    for (auto i = 0; i < d_ports; i++) {
        if (d_port_settings[i].enabled) {
            num_enabled_di_ports++;
        }
    }
}

bool digitizer_block_impl::is_armed() { return d_armed; }

void digitizer_block_impl::disarm()
{
    if (!d_armed) {
        return;
    }

    if (d_acquisition_mode == acquisition_mode_t::STREAMING) {
        transit_poll_thread_to_idle();
    }

    auto ec = driver_disarm();
    if (ec) {
        add_error_code(ec);
        d_logger->warn("disarm failed: {}", ec);
    }

    d_armed = false;
}

void digitizer_block_impl::close()
{
    auto ec = driver_close();
    if (ec) {
        add_error_code(ec);
        d_logger->warn("close failed: {}", ec);
    }
    d_closed = true;
    d_initialized = false;
}

std::vector<error_info_t> digitizer_block_impl::get_errors() { return d_errors.get(); }

std::string digitizer_block_impl::getConfigureExceptionMessage()
{
    return d_configure_exception_message;
}

bool digitizer_block_impl::start()
{
    try {
        initialize();
        configure();

        // Needed in case start/run is called multiple times without destructing the
        // flowgraph
        d_was_triggered_once = false;
        d_data_rdy_errc = std::error_code{};
        d_data_rdy = false;

        if (d_acquisition_mode == acquisition_mode_t::STREAMING) {
            start_poll_thread();
        }

        if (d_auto_arm && d_acquisition_mode == acquisition_mode_t::STREAMING) {
            arm();
        }
    } catch (const std::exception& ex) {
        d_configure_exception_message = ex.what();
        d_logger->error("{}", d_configure_exception_message);

        // No matter if true or false is returned here, gnuradio will continue to run the
        // block, so we stop in manually Re-throwing the exception would result in the
        // binary getting stuck
        this->stop();
        return false;

    } catch (...) {
        d_configure_exception_message =
            "Unknown Exception received in digitizer_block_impl::start";
        d_logger->error("{}", d_configure_exception_message);

        this->stop();
        return false;
    }

    return true;
}

bool digitizer_block_impl::stop()
{
    if (!d_initialized) {
        return true;
    }

    if (d_armed) {
        // Interrupt waiting function (workaround). From the scheduler point of view this
        // is not needed because it makes sure that the worker thread gets interrupted
        // before the stop method is called. But we have this in place in order to allow
        // for manual intervention.
        notify_data_ready(digitizer_block_errc::Stopped);

        disarm();
    }

    if (d_acquisition_mode == acquisition_mode_t::STREAMING) {
        stop_poll_thread();
    }

    d_configure_exception_message.clear();

    return true;
}

/**********************************************************************
 * Driver interface
 **********************************************************************/

void digitizer_block_impl::notify_data_ready(std::error_code ec)
{
    if (ec) {
        add_error_code(ec);
    }

    {
        boost::mutex::scoped_lock lock(d_mutex);
        d_data_rdy = true;
        d_data_rdy_errc = ec;
    }

    d_data_rdy_cv.notify_one();
}

std::error_code digitizer_block_impl::wait_data_ready()
{
    boost::mutex::scoped_lock lock(d_mutex);

    d_data_rdy_cv.wait(lock, [this] { return d_data_rdy; });
    return d_data_rdy_errc;
}

void digitizer_block_impl::clear_data_ready()
{
    boost::mutex::scoped_lock lock(d_mutex);

    d_data_rdy = false;
    d_data_rdy_errc = std::error_code{};
}

/**********************************************************************
 * GR worker functions
 **********************************************************************/

work_return_t digitizer_block_impl::work_rapid_block(work_io& wio)
{
    int noutput_items = static_cast<int>(wio.outputs()[0].n_items);
    if (d_bstate.state == rapid_block_state_t::WAITING) {
        if (d_trigger_once && d_was_triggered_once) {
            return work_return_t::DONE; // TODO(PORT) was -1
        }

        if (d_auto_arm) {
            disarm();
            while (true) {
                try {
                    arm();
                    break;
                } catch (...) {
                    return work_return_t::ERROR; // TODO(PORT) was -1
                }
            }
        }

        // Wait conditional variable, when waken clear it
        auto ec = wait_data_ready();
        clear_data_ready();

        // Stop requested
        if (ec == digitizer_block_errc::Stopped) {
            d_logger->info("stop requested");
            return work_return_t::DONE; // TODO(PORT) was -1
        }
        else if (ec) {
            d_logger->error("error occurred while waiting for data: {}", ec);
            return work_return_t::ERROR; // TODO(PORT) was 0
        }

        // we assume all the blocks are ready
        d_bstate.initialize(d_nr_captures);
    }

    if (d_bstate.state == rapid_block_state_t::READING_PART1) {
        // If d_trigger_once is true we will signal all done in the next iteration
        // with the block state set to WAITING
        d_was_triggered_once = true;

        auto samples_to_fetch = get_block_size();
        auto downsampled_samples = get_block_size_with_downsampling();

        // Instruct the driver to prefetch samples. Drivers might choose to ignore this
        // call
        auto ec = driver_prefetch_block(samples_to_fetch, d_bstate.waveform_idx);
        if (ec) {
            add_error_code(ec);
            return work_return_t::ERROR; // TODO(PORT) was -1
        }

        // Initiate state machine for the current waveform. Note state machine track
        // and adjust the waveform index.
        d_bstate.set_waveform_params(0, downsampled_samples);

        // We are good to read first batch of samples
        noutput_items = std::min(noutput_items, d_bstate.samples_left);

        ec = driver_get_rapid_block_data(
            d_bstate.offset, noutput_items, d_bstate.waveform_idx, wio, d_status);
        if (ec) {
            add_error_code(ec);
            return work_return_t::ERROR; // TODO(PORT) was -1
        }

        if (!d_timing_messages.empty()) {
            const auto timing = d_timing_messages.front();

            // Attach trigger info to value outputs and to all ports
            auto vec_idx = 0;
            const uint32_t pre_trigger_samples_with_downsampling =
                get_pre_trigger_samples_with_downsampling();
            const double time_per_sample_with_downsampling_ns =
                d_time_per_sample_ns * d_downsampling_factor;

            // TODO do we need this timestamp adjustment for pre_samples?
            const auto timestamp =
                timing.timestamp + std::chrono::nanoseconds(static_cast<int64_t>(
                                       (pre_trigger_samples_with_downsampling *
                                        time_per_sample_with_downsampling_ns)));
            const auto tag_offset =
                wio.outputs()[0].nitems_written() + pre_trigger_samples_with_downsampling;

            auto trigger_tag =
                make_trigger_tag(tag_offset, timing.name, timestamp, timing.offset);

            for (auto i = 0; i < d_ai_channels && vec_idx < (int)wio.outputs().size();
                 i++, vec_idx += 2) {
                if (d_channel_settings[i].enabled) {
                    wio.outputs()[vec_idx].add_tag(trigger_tag);
                }
            }

            // Add tags to digital port
            for (auto i = 0; i < d_ports && vec_idx < (int)wio.outputs().size();
                 i++, vec_idx++) {
                if (d_port_settings[i].enabled)
                    wio.outputs()[vec_idx].add_tag(trigger_tag);
            }
        }

        // update state
        d_bstate.update_state(noutput_items);
        wio.produce_each(noutput_items);
        return work_return_t::OK;
    }
    else if (d_bstate.state == rapid_block_state_t::READING_THE_REST) {
        noutput_items = std::min(noutput_items, d_bstate.samples_left);

        auto ec = driver_get_rapid_block_data(
            d_bstate.offset, noutput_items, d_bstate.waveform_idx, wio, d_status);
        if (ec) {
            add_error_code(ec);
            return work_return_t::ERROR; // TODO(PORT) was -1
        }

        // update state
        d_bstate.update_state(noutput_items);
        wio.produce_each(noutput_items);
        return work_return_t::OK;
    }

    return work_return_t::ERROR; // TODO(PORT) was -1
}

void digitizer_block_impl::poll_work_function()
{
    boost::unique_lock<boost::mutex> lock(d_poller_mutex, boost::defer_lock);
    std::chrono::duration<float> poll_duration(d_poll_rate);
    std::chrono::duration<float> sleep_time;

#ifdef PORT_DISABLED
    gr::thread::set_thread_name(pthread_self(), "poller");
#endif

    // relax cpu with less lock calls.
    unsigned int check_every_n_times = 10;
    unsigned int poller_state_check_counter = check_every_n_times;
    poller_state_t state = poller_state_t::IDLE;

    while (true) {
        poller_state_check_counter++;
        if (poller_state_check_counter >= check_every_n_times) {
            lock.lock();
            state = d_poller_state;
            lock.unlock();
            poller_state_check_counter = 0;
        }

        if (state == poller_state_t::RUNNING) {
            // Start watchdog a new
            auto poll_start = std::chrono::high_resolution_clock::now();
            auto ec = driver_poll();
            if (ec) {
                // Only print out an error message
                d_logger->error("poll failed with: {}", ec);
                // Notify work method about the error... Work method will re-arm the
                // driver if required.
                d_app_buffer.notify_data_ready(ec);

                // Prevent error-flood on close
                if (d_closed)
                    return;
            }

            // Watchdog is "turned on" only some time after the acquisition start for two
            // reasons:
            // - to avoid false positives
            // - to avoid fast rearm attempts
            float estimated_samp_rate = 0.0;
            {
                // Note, mutex is not needed in case of PicoScope implementations but in
                // order to make the base class relatively generic we use mutex (streaming
                // callback is called from this
                //  thread).
                boost::mutex::scoped_lock watchdog_guard(d_watchdog_mutex);
                estimated_samp_rate = d_estimated_sample_rate.get_avg_value();
            }

            if (estimated_samp_rate <
                (get_samp_rate() * WATCHDOG_SAMPLE_RATE_THRESHOLD)) {
                // This will wake up the worker thread (see do_work method), and that
                // thread will then rearm the device...
                d_logger->error("Watchdog: estimated sample rate {}Hz, expected: {}Hz",
                                estimated_samp_rate,
                                get_samp_rate());
                d_app_buffer.notify_data_ready(digitizer_block_errc::Watchdog);
            }

            // Substract the time each iteration itself took in order to get closer to the
            // desired poll duration
            std::chrono::duration<float> elapsed_poll_duration =
                std::chrono::high_resolution_clock::now() - poll_start;
            if (elapsed_poll_duration > poll_duration) {
                sleep_time = std::chrono::duration<float>::zero();
            }
            else {
                sleep_time = poll_duration - elapsed_poll_duration;
            }
            std::this_thread::sleep_for(sleep_time);
        }
        else {
            if (state == poller_state_t::PEND_IDLE) {
                lock.lock();
                d_poller_state = state = poller_state_t::IDLE;
                lock.unlock();

                d_poller_cv.notify_all();
            }
            else if (state == poller_state_t::PEND_EXIT) {
                lock.lock();
                d_poller_state = state = poller_state_t::EXIT;
                lock.unlock();

                d_poller_cv.notify_all();
                return;
            }

            // Relax CPU
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
}

void digitizer_block_impl::start_poll_thread()
{
    if (!d_poller.joinable()) {
        boost::mutex::scoped_lock guard(d_poller_mutex);
        d_poller_state = poller_state_t::IDLE;
        d_poller = boost::thread(&digitizer_block_impl::poll_work_function, this);
    }
}

void digitizer_block_impl::stop_poll_thread()
{
    if (!d_poller.joinable()) {
        return;
    }

    boost::unique_lock<boost::mutex> lock(d_poller_mutex);
    d_poller_state = poller_state_t::PEND_EXIT;
    d_poller_cv.wait_for(lock, boost::chrono::seconds(5), [this] {
        return d_poller_state == poller_state_t::EXIT;
    });
    lock.unlock();

    d_poller.join();
}

void digitizer_block_impl::transit_poll_thread_to_idle()
{
    boost::unique_lock<boost::mutex> lock(d_poller_mutex);

    if (d_poller_state == poller_state_t::EXIT) {
        return; // nothing to do
    }

    d_poller_state = poller_state_t::PEND_IDLE;
    d_poller_cv.wait(lock, [this] { return d_poller_state == poller_state_t::IDLE; });
}

void digitizer_block_impl::transit_poll_thread_to_running()
{
    boost::mutex::scoped_lock guard(d_poller_mutex);
    d_poller_state = poller_state_t::RUNNING;
}

void digitizer_block_impl::dissect_data_chunk(
    std::span<const uint8_t> chunk_data,
    std::vector<std::span<const float>>& ai_buffers,
    std::vector<std::span<const float>>& ai_error_buffers,
    std::vector<std::span<const uint8_t>>& port_buffers)
{
    const float* read_ptr = reinterpret_cast<const float*>(chunk_data.data());

    // Create views on individual channels
    for (size_t chan_idx = 0; chan_idx < ai_buffers.size(); chan_idx++) {
        ai_buffers[chan_idx] = std::span(read_ptr, d_buffer_size);
        read_ptr += d_buffer_size;
        ai_error_buffers[chan_idx] = std::span(read_ptr, d_buffer_size);
        read_ptr += d_buffer_size;
    }

    const uint8_t* di_read_ptr = reinterpret_cast<const uint8_t*>(read_ptr);

    for (size_t port_idx = 0; port_idx < port_buffers.size(); port_idx++) {
        port_buffers[port_idx] = std::span(di_read_ptr, d_buffer_size);
        di_read_ptr += d_buffer_size;
    }
}

work_return_t digitizer_block_impl::work_stream(work_io& wio)
{
    // used for debugging in order to see how often gr calls this block
    //     uint64_t now = get_timestamp_milli_utc();
    //     if( now - last_call_utc > 20)
    //       std::cout << "now - last_call_utc [ms]: " << now - last_call_utc <<
    //       std::endl;
    //     last_call_utc = get_timestamp_milli_utc();

    auto noutput_items = static_cast<int>(wio.outputs()[0].n_items);
    assert(noutput_items >= static_cast<int>(d_buffer_size));

    // process only one buffer per iteration
    noutput_items = d_buffer_size;

    // get previous chunk if any (if we were waiting for timing messages), or try to
    // obtain a new one
    app_buffer_t::data_chunk_ptr chunk;
    std::vector<std::size_t> trigger_offsets;
    std::swap(chunk, d_pending_data.data_chunk);
    std::swap(trigger_offsets, d_pending_data.trigger_offsets);

    if (!chunk) {
        // wait data on application buffer
        auto ec = d_app_buffer.wait_data_ready();

        if (ec) {
            add_error_code(ec);
        }

        if (ec == digitizer_block_errc::Stopped) {
            d_logger->info("stop requested");
            return work_return_t::DONE; // stop // TODO(PORT) was -1;
        }
        else if (ec == digitizer_block_errc::Watchdog) {
            d_logger->error("Watchdog triggered, rearming device...");
            // Rearm device
            disarm();
            arm();
            return work_return_t::OK; // TODO(PORT) was: 0; // work will be called again
        }
        if (ec) {
            d_logger->error("Error reading stream data: {}", ec);
            return work_return_t::ERROR; // stop // TODO(PORT) was -1;
        }

        chunk = d_app_buffer.get_data_chunk();
    }

    // Search data for timing triggers

    const auto timestamp_now_ns_utc = chunk->d_local_timestamp;
    const auto lost_count = chunk->d_lost_count;
    const auto channel_status = chunk->d_status;

    const auto enabled_aichan_count = get_enabled_aichan_count();
    const auto enabled_diport_count = get_enabled_diport_count();
    std::vector<std::span<const float>> ai_buffer_views(enabled_aichan_count);
    std::vector<std::span<const float>> ai_error_buffer_views(enabled_aichan_count);
    std::vector<std::span<const uint8_t>> port_buffer_views(enabled_diport_count);

    dissect_data_chunk(
        chunk->d_data, ai_buffer_views, ai_error_buffer_views, port_buffer_views);

    std::vector<tag_t> trigger_tags;

    if (d_trigger_settings.is_enabled()) {
        if (trigger_offsets.empty()) {
            // if we don't have the offsets from the pending data, search for triggers now
            if (d_trigger_settings.is_analog()) {
                // TODO: improve, check selected trigger on arm
                const auto aichan = convert_to_aichan_idx(d_trigger_settings.source);
                trigger_offsets = find_analog_triggers(ai_buffer_views[aichan]);
            }
            else if (d_trigger_settings.is_digital()) {
                auto port = d_trigger_settings.pin_number / 8;
                auto pin = d_trigger_settings.pin_number % 8;
                auto mask = 1 << pin;
                trigger_offsets = find_digital_triggers(port_buffer_views[port], mask);
            }
        }

        if (trigger_offsets.size() > d_timing_messages.size()) {
            // not enough timing messages received to process trigger, abort for now and
            // wait
            d_pending_data.data_chunk = std::move(chunk);
            d_pending_data.trigger_offsets = trigger_offsets;
            return work_return_t::OK;
        }

        // pair timing messages with trigger offsets and create tags
        trigger_tags.reserve(trigger_offsets.size());
        for (const auto& trigger_offset : trigger_offsets) {
            const auto timing = d_timing_messages.front();
            d_timing_messages.pop_front();
            trigger_tags.push_back(
                make_trigger_tag(wio.outputs()[0].nitems_written() + trigger_offset,
                                 timing.name,
                                 timing.timestamp,
                                 timing.offset));
        }
    }
    else {
        // no trigger channels, use the last timing message to tag the first sample
        if (!d_timing_messages.empty()) {
            const auto timing = d_timing_messages[0];
            trigger_tags.push_back(make_trigger_tag(wio.outputs()[0].nitems_written(),
                                                    timing.name,
                                                    timing.timestamp,
                                                    timing.offset));
        }
    }

    // copy data to output buffers and pass trigger tags
    for (auto i = 0; i < d_ai_channels; i++) {
        if (d_channel_settings[i].enabled) {
            const auto in = ai_buffer_views[i];
            const auto in_error = ai_error_buffer_views[i];
            auto out = wio.outputs()[2 * i].items<float>();
            auto out_error = wio.outputs()[2 * i + 1].items<float>();

            std::copy(in.begin(), in.end(), out);
            std::copy(in_error.begin(), in_error.end(), out_error);

            for (auto& trigger_tag : trigger_tags) {
                wio.outputs()[2 * i].add_tag(trigger_tag);
            }
        }
    }

    const auto port_offset = d_ai_channels * 2;

    for (auto i = 0; i < d_ports; i++) {
        if (d_port_settings[i].enabled) {
            auto out = wio.outputs()[port_offset + i];
            auto outbuf = out.items<uint8_t>();
            std::copy(port_buffer_views[i].begin(), port_buffer_views[i].end(), outbuf);
            for (auto& trigger_tag : trigger_tags) {
                out.add_tag(trigger_tag);
            }
        }
    }

    if (lost_count) {
        d_logger->error("{} digitizer data buffers lost. Usually the cause of this error "
                        "is, that the work method of the Digitizer block is called with "
                        "low frequency because of a 'traffic jam' in the flowgraph. (One "
                        "of the next blocks cannot process incoming data in time)",
                        lost_count);
    }

    // Compile acquisition info tag
    // TODO check what to do with these, they still rely on the old timestamp_now_ns_utc
    acq_info_t tag_info{};

    tag_info.timestamp = timestamp_now_ns_utc.count();
    tag_info.timebase = get_timebase_with_downsampling();
    tag_info.user_delay = 0.0;
    tag_info.actual_delay = 0.0;

    // Attach tags to the channel values...

    int output_idx = 0;

    for (auto i = 0; i < d_ai_channels; i++) {
        if (d_channel_settings[i].enabled) {
            // add channel specific status
            tag_info.status = channel_status.at(i);

            auto tag = make_acq_info_tag(tag_info, wio.outputs()[0].nitems_written());
            wio.outputs()[output_idx].add_tag(tag);
        }
        output_idx += 2;
    }

    // ...and to all digital ports
    tag_info.status = 0;
    auto tag = make_acq_info_tag(tag_info, wio.outputs()[0].nitems_written());

    for (auto i = 0; i < d_ports; i++) {
        if (d_port_settings[i].enabled) {
            wio.outputs()[output_idx].add_tag(tag);
        }
        output_idx++;
    }

    wio.produce_each(noutput_items);
    return work_return_t::OK;
}

work_return_t digitizer_block_impl::work(work_io& wio)
{
    work_return_t retval;

    if (d_acquisition_mode == acquisition_mode_t::STREAMING) {
        retval = work_stream(wio);
    }
    else {
        retval = work_rapid_block(wio);
    }

    if (retval == work_return_t::OK) {
        if (!d_timebase_published) {
            auto timebase_tag = make_timebase_info_tag(get_timebase_with_downsampling());
            timebase_tag.set_offset(wio.outputs()[0].nitems_written());

            for (std::size_t i = 0; i < wio.outputs().size(); i++) {
                wio.outputs()[i].add_tag(timebase_tag);
            }

            d_timebase_published = true;
        }
    }

    return retval;
}

template <typename T>
constexpr static std::chrono::nanoseconds convert_to_ns(T ns)
{
    using namespace std::chrono;
    return round<nanoseconds>(duration<T, std::nano>{ ns });
}

void digitizer_block_impl::handle_msg_timing(pmtf::pmt msg)
{
    const auto map = pmtf::get_as<std::map<std::string, pmtf::pmt>>(msg);

    try {
        if (!d_trigger_settings.is_enabled()) {
            d_timing_messages.clear();
        }

        d_timing_messages.push_back(
            { .name = pmtf::get_as<std::string>(map.at(tag::TRIGGER_NAME)),
              .timestamp =
                  convert_to_ns(pmtf::get_as<int64_t>(map.at(tag::TRIGGER_TIME))),
              .offset = convert_to_ns(pmtf::get_as<double>(map.at(tag::TRIGGER_OFFSET)) *
                                      1000000000) });

        const auto last = d_timing_messages.back();
        d_logger->debug("Received timing message: name='{}', timestamp={}, offset={}, "
                        "Already Queued={}",
                        last.name,
                        last.timestamp,
                        last.offset,
                        d_timing_messages.size() - 1);
    } catch (const std::out_of_range& e) {
        d_logger->error("Could not decode timing message: {}", e.what());
    }
}

} // namespace gr::digitizers
