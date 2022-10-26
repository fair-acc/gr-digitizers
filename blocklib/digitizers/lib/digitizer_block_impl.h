#pragma once

#include "app_buffer.h"
#include "enums.h"
#include "tags.h"
#include "utils.h"

#include <gnuradio/block.h>

#include <boost/chrono.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

#include <chrono>
#include <cstring>
#include <error.h>

#include <sstream>
#include <system_error>
#include <thread>

namespace gr::digitizers {

// TODO(PORT) error_info_t should be in block API
/*!
 * \brief Error information.
 * \ingroup digitizers
 */
struct DIGITIZERS_API error_info_t {
    uint64_t        timestamp;
    std::error_code code;
};

enum class digitizer_block_errc {
    Stopped     = 1,
    Interrupted = 10, // did not respond in time,
    Watchdog    = 11, // no or too little samples received in time
};

std::error_code make_error_code(digitizer_block_errc e);
} // namespace gr::digitizers

// Hook in our error code visible to the std system error
namespace std {
template<>
struct is_error_code_enum<gr::digitizers::digitizer_block_errc> : true_type {};
} // namespace std

namespace gr::digitizers {

/**********************************************************************
 * Hardcoded values
 **********************************************************************/

// Watchdog is triggered if estimated sample rate falls below 75%
static constexpr float WATCHDOG_SAMPLE_RATE_THRESHOLD = 0.75;

static constexpr int   AVERAGE_HISTORY_LENGTH         = 100000;

/**********************************************************************
 * Helpers and struct definitions
 **********************************************************************/

/*!
 * A helper struct for keeping track which samples have been already processes
 * in rapid block mode.
 */
struct rapid_block_state_t {
    enum State { WAITING,
        READING_PART1,
        READING_THE_REST };

    rapid_block_state_t()
        : state(WAITING), waveform_count(0), waveform_idx(0), offset(0), samples_left(0) {}

    State state;

    int   waveform_count;
    int   waveform_idx; // index of the waveform we are currently reading
    int   offset;       // reading offset
    int   samples_left;

    void  to_wait() {
         state = rapid_block_state_t::WAITING;
    }

    void initialize(int nr_waveforms) {
        state          = rapid_block_state_t::READING_PART1;
        waveform_idx   = 0;
        waveform_count = nr_waveforms;
    }

    void set_waveform_params(uint32_t offset_samps, uint32_t samples_to_read) {
        offset       = offset_samps;
        samples_left = samples_to_read;
    }

    // update state
    void update_state(uint32_t nsamples) {
        offset += nsamples;
        samples_left -= nsamples;

        if (samples_left > 0) {
            state = rapid_block_state_t::READING_THE_REST;
        } else {
            waveform_idx++;
            if (waveform_idx >= waveform_count) {
                state = rapid_block_state_t::WAITING;
            } else {
                state = rapid_block_state_t::READING_PART1;
            }
        }
    }
};

/*!
 * A struct holding AI channel settings.
 */
struct channel_setting_t {
    channel_setting_t()
        : range(2.0), offset(0.0), enabled(false), coupling(coupling_t::AC_1M) {}

    double     range;
    float      offset;
    bool       enabled;
    coupling_t coupling;
};

struct port_setting_t {
    port_setting_t()
        : logic_level(1.5), enabled(false) {}

    float logic_level;
    bool  enabled;
};

static const std::string TRIGGER_NONE_SOURCE    = "NONE";
static const std::string TRIGGER_DIGITAL_SOURCE = "DI"; // DI is as well used as "AUX" for p6000 scopes

struct trigger_setting_t {
    trigger_setting_t()
        : source(TRIGGER_NONE_SOURCE), threshold(0), direction(trigger_direction_t::RISING), pin_number(0) {}

    bool
    is_enabled() const {
        return source != TRIGGER_NONE_SOURCE;
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
    float               threshold; // AI only
    trigger_direction_t direction;
    int                 pin_number; // DI only
};

/*!
 * \brief A simple circular buffer for keeping last N errors.
 */
class error_buffer_t {
    boost::circular_buffer<error_info_t> cb;
    boost::mutex                         access;

public:
    error_buffer_t(int n)
        : cb(n), access() {
    }

    ~error_buffer_t() {
    }

    void push(error_info_t err) {
        boost::mutex::scoped_lock lg(access);
        cb.push_back(err);
    }

    void push(std::error_code ec) {
        push(error_info_t{ get_timestamp_nano_utc(), ec });
    }

    // Note circular buffer is cleared at get
    std::vector<error_info_t> get() {
        boost::mutex::scoped_lock lg(access);
        std::vector<error_info_t> ret;
        ret.insert(ret.begin(), cb.begin(), cb.end());
        cb.clear();
        return ret;
    }
};

/*!
 * \brief The state of the poller worker function.
 */
enum class poller_state_t {
    IDLE = 0,
    RUNNING,
    EXIT,
    PEND_EXIT,
    PEND_IDLE
};

/**********************************************************************
 * Error codes
 *********************************************************************/

struct digitizer_block_err_category : std::error_category {
    const char *name() const noexcept override;
    std::string message(int ev) const override;
};

const char *
digitizer_block_err_category::name() const noexcept {
    return "digitizer_block";
}

std::string
digitizer_block_err_category::message(int ev) const {
    switch (static_cast<digitizer_block_errc>(ev)) {
    case digitizer_block_errc::Interrupted:
        return "Wit interrupted";

    default:
        return "(unrecognized error)";
    }
}

const digitizer_block_err_category __digitizer_block_category{};

std::error_code                    make_error_code(digitizer_block_errc e) {
                       return { static_cast<int>(e), __digitizer_block_category };
}

template<typename T>
concept Driver = requires(T a, std::size_t sz0, std::size_t sz1, std::size_t sz2, work_io &wio, std::vector<uint32_t> &status) {
    /*!
     * Note offset and length is in non-decimated samples
     */
    { a.prefetch_block(sz0, sz1) } -> std::convertible_to<std::error_code>;

    /*!
     * By offset and length we mean decimated samples, and offset is offset within the subparts of data.
     */
    { a.get_rapid_block_data(sz0, sz1, sz2, wio, status) } -> std::convertible_to<std::error_code>;
    { a.initialize() } -> std::convertible_to<std::error_code>;
    { a.configure() } -> std::convertible_to<std::error_code>;
    { a.arm() } -> std::convertible_to<std::error_code>;
    { a.disarm() } -> std::convertible_to<std::error_code>;
    { a.close() } -> std::convertible_to<std::error_code>;
    { a.poll() } -> std::convertible_to<std::error_code>;
};

struct digitizer_args {
    double                       sample_rate              = 10000;
    std::size_t                  buffer_size              = 8192;
    std::size_t                  nr_buffers               = 100;
    std::size_t                  driver_buffer_size       = 100000;
    std::size_t                  pre_samples              = 1000;
    std::size_t                  post_samples             = 9000;
    bool                         auto_arm                 = true;
    bool                         trigger_once             = false;
    std::size_t                  rapid_block_nr_captures  = 1;
    double                       streaming_mode_poll_rate = 0.001;
    digitizer_acquisition_mode_t acquisition_mode         = digitizer_acquisition_mode_t::STREAMING;
    downsampling_mode_t          downsampling_mode        = downsampling_mode_t::NONE;
    std::size_t                  downsampling_factor      = 1;
    std::size_t                  ai_channels              = 0;
    std::size_t                  ports                    = 0;
};

template<Driver T>
class digitizer_block_impl {
public:
    /**********************************************************************
     * Public API calls (see digitizer_block.h for docs)
     *************************************s*********************************/

public:
    static const int MAX_SUPPORTED_AI_CHANNELS = 16;
    static const int MAX_SUPPORTED_PORTS       = 8;

    // TODO(PORT) remove setters/getters

    void set_nr_buffers(int buffer_size);

    void set_driver_buffer_size(int driver_buffer_size);

    void set_rapid_block(int nr_captures) {
        if (nr_captures < 1) {
            std::ostringstream message;
            message << "Exception in " << __FILE__ << ":" << __LINE__ << ": nr waveforms should be at least one" << nr_captures;
            throw std::invalid_argument(message.str());
        }

        d_acquisition_mode = digitizer_acquisition_mode_t::RAPID_BLOCK;
        d_nr_captures      = static_cast<uint32_t>(nr_captures);
    }

    void set_streaming(double poll_rate = 0.001) {
        if (poll_rate < 0.0) {
            std::ostringstream message;
            message << "Exception in " << __FILE__ << ":" << __LINE__ << ": poll rate can't be negative:" << poll_rate;
            throw std::invalid_argument(message.str());
        }

        d_acquisition_mode = digitizer_acquisition_mode_t::STREAMING;
        d_poll_rate        = poll_rate;

        // just in case
        d_nr_captures = 1;
    }

    void set_downsampling(downsampling_mode_t mode, int downsample_factor) {
        if (mode == downsampling_mode_t::NONE) {
            downsample_factor = 1;
        } else if (downsample_factor < 2) {
            std::ostringstream message;
            message << "Exception in " << __FILE__ << ":" << __LINE__ << ": downsampling factor should be at least 2: " << downsample_factor;
            throw std::invalid_argument(message.str());
        }

        d_downsampling_mode   = mode;
        d_downsampling_factor = static_cast<uint32_t>(downsample_factor);
    }

    void set_aichan(const std::string &id, bool enabled, double range, coupling_t coupling, double range_offset = 0) {
        auto idx                         = convert_to_aichan_idx(id);
        d_channel_settings[idx].range    = range;
        d_channel_settings[idx].offset   = range_offset;
        d_channel_settings[idx].enabled  = enabled;
        d_channel_settings[idx].coupling = coupling;
    }

    /*!
     * \brief Returns number of enabled analog channels.
     */
    int get_enabled_aichan_count() const {
        auto count = 0;
        for (const auto &c : d_channel_settings) {
            count += c.enabled;
        }
        return count;
    }

    void set_aichan_range(const std::string &id, double range, double range_offset = 0) {
        auto idx                       = convert_to_aichan_idx(id);
        d_channel_settings[idx].range  = range;
        d_channel_settings[idx].offset = range_offset;
    }

    void set_aichan_trigger(const std::string &id, trigger_direction_t direction, double threshold) {
        // Some scopes have an dedicated AUX Trigger-Input. Skip id verification for them
        if (id != "AUX")
            convert_to_aichan_idx(id); // Just to verify id

        d_trigger_settings.source     = id;
        d_trigger_settings.threshold  = threshold;
        d_trigger_settings.direction  = direction;
        d_trigger_settings.pin_number = 0; // not used
    }

    void set_diport(const std::string &id, bool enabled, double thresh_voltage) {
        auto port_number                         = convert_to_port_idx(id);

        d_port_settings[port_number].logic_level = thresh_voltage;
        d_port_settings[port_number].enabled     = enabled;
    }

    /*!
     * \brief Returns number of enabled ports;
     */
    int get_enabled_diport_count() const {
        auto count = 0;
        for (const auto &p : d_port_settings) {
            count += p.enabled;
        }
        return count;
    }

    void set_di_trigger(uint32_t pin, trigger_direction_t direction) {
        d_trigger_settings.source     = TRIGGER_DIGITAL_SOURCE;
        d_trigger_settings.threshold  = 0.0; // not used
        d_trigger_settings.direction  = direction;
        d_trigger_settings.pin_number = pin;
    }

    void disable_triggers() {
        d_trigger_settings.source = TRIGGER_NONE_SOURCE;
    }

    void initialize() {
        if (d_initialized) {
            return;
        }

        auto ec = _driver->initialize();
        if (ec) {
            add_error_code(ec);
            std::ostringstream message;
            message << "Exception in " << __FILE__ << ":" << __LINE__ << ": initialize failed. ErrorCode: " << ec;
            throw std::runtime_error(message.str());
        }

        d_initialized = true;
    }

    void configure() {
        if (!d_initialized) {
            std::ostringstream message;
            message << "Exception in " << __FILE__ << ":" << __LINE__ << ": initialize first!";
            throw std::runtime_error(message.str());
        }

        if (d_armed) {
            std::ostringstream message;
            message << "Exception in " << __FILE__ << ":" << __LINE__ << ": disarm first!";
            throw std::runtime_error(message.str());
        }

        auto ec = _driver->configure();
        if (ec) {
            add_error_code(ec);
            std::ostringstream message;
            message << "Exception in " << __FILE__ << ":" << __LINE__ << ": configure failed. ErrorCode: " << ec;
            throw std::runtime_error(message.str());
        }
        // initialize application buffer
        d_app_buffer.initialize(get_enabled_aichan_count(),
                get_enabled_diport_count(), d_buffer_size, d_nr_buffers);
    }

    void arm() {
        if (d_armed) {
            return;
        }

        // set estimated sample rate to expected
        float expected = static_cast<float>(d_actual_samp_rate);
        for (auto i = 0; i < AVERAGE_HISTORY_LENGTH; i++) {
            d_estimated_sample_rate.add(expected);
        }

        // arm the driver
        auto ec = _driver->arm();
        if (ec) {
            add_error_code(ec);
            std::ostringstream message;
            message << "Exception in " << __FILE__ << ":" << __LINE__ << ": arm failed. ErrorCode: " << ec;
            throw std::runtime_error(message.str());
        }

        d_armed                             = true;
        d_timebase_published                = false;
        d_was_last_callback_timestamp_taken = false;

        // clear error condition in the application buffer
        d_app_buffer.notify_data_ready(std::error_code{});

        // notify poll thread to start with the poll request
        if (d_acquisition_mode == digitizer_acquisition_mode_t::STREAMING) {
            transit_poll_thread_to_running();
        }

        // allocate buffer pointer vectors.
        int num_enabled_ai_channels = 0;
        int num_enabled_di_ports    = 0;
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
        ai_buffers.resize(num_enabled_ai_channels);
        ai_error_buffers.resize(num_enabled_ai_channels);
        port_buffers.resize(num_enabled_di_ports);
    }

    bool is_armed() {
        return d_armed;
    }

    void disarm() {
        if (!d_armed) {
            return;
        }

        if (d_acquisition_mode == digitizer_acquisition_mode_t::STREAMING) {
            transit_poll_thread_to_idle();
        }

        auto ec = _driver->disarm();
        if (ec) {
            add_error_code(ec);
            d_logger->warn("disarm failed: {}", ec);
        }

        d_armed = false;
    }

    void close() {
        auto ec = _driver->close();
        if (ec) {
            add_error_code(ec);
            d_logger->warn("close failed: {}", ec);
        }
        d_closed      = true;
        d_initialized = false;
    }

    std::vector<error_info_t> get_errors() {
        return d_errors.get();
    }

    bool start() {
        try {
            initialize();
            configure();

            // Needed in case start/run is called multiple times without destructing the flowgraph
            d_was_triggered_once = false;
            d_data_rdy_errc      = std::error_code{};
            d_data_rdy           = false;

            if (d_acquisition_mode == digitizer_acquisition_mode_t::STREAMING) {
                start_poll_thread();
            }

            if (d_auto_arm && d_acquisition_mode == digitizer_acquisition_mode_t::STREAMING) {
                arm();
            }
        } catch (const std::exception &ex) {
            d_configure_exception_message = ex.what();
            d_logger->error("{}", d_configure_exception_message);

            // No matter if true or false is returned here, gnuradio will continue to run the block, so we stop in manually
            // Re-throwing the exception would result in the binary getting stuck
            stop();
            return false;

        } catch (...) {
            d_configure_exception_message = "Unknown Exception received in digitizer_block_impl<T>::start";
            d_logger->error("{}", d_configure_exception_message);

            stop();
            return false;
        }

        return true;
    }

    bool stop() {
        if (!d_initialized) {
            return true;
        }

        if (d_armed) {
            // Interrupt waiting function (workaround). From the scheduler point of view this is not
            // needed because it makes sure that the worker thread gets interrupted before the stop
            // method is called. But we have this in place in order to allow for manual intervention.
            notify_data_ready(digitizer_block_errc::Stopped);

            disarm();
        }

        if (d_acquisition_mode == digitizer_acquisition_mode_t::STREAMING) {
            stop_poll_thread();
        }

        d_configure_exception_message = "";

        return true;
    }

    // Where all the action really happens
    work_return_t work(work_io &wio) {
        work_return_t retval = work_return_t::ERROR;

        if (d_acquisition_mode == digitizer_acquisition_mode_t::STREAMING) {
            retval = work_stream(wio);
        } else if (d_acquisition_mode == digitizer_acquisition_mode_t::RAPID_BLOCK) {
            retval = work_rapid_block(wio);
        }

        if (retval == work_return_t::OK && !d_timebase_published) {
            auto timebase_tag = make_timebase_info_tag(get_timebase_with_downsampling());
            timebase_tag.set_offset(wio.outputs()[0].nitems_written());

            for (std::size_t i = 0; i < wio.outputs().size(); i++) {
                wio.outputs()[i].add_tag(timebase_tag);
            }

            d_timebase_published = true;
        }

        return retval;
    }

    std::string getConfigureExceptionMessage() {
        return d_configure_exception_message;
    }

    /**********************************************************************
     * Structors
     **********************************************************************/

    explicit digitizer_block_impl(const digitizer_args &args, std::shared_ptr<T> driver, gr::logger_ptr logger)
        : _driver{ std::move(driver) }
        , d_logger{ std::move(logger) }
        , d_samp_rate(args.sample_rate)
        , d_actual_samp_rate(args.sample_rate)
        , d_time_per_sample_ns(1000000000. / args.sample_rate)
        , d_pre_samples(args.pre_samples)
        , d_post_samples(args.post_samples)
        , d_nr_captures(args.rapid_block_nr_captures)
        , d_buffer_size(static_cast<uint32_t>(args.buffer_size))
        , d_nr_buffers(static_cast<uint32_t>(args.nr_buffers))
        , d_driver_buffer_size(static_cast<uint32_t>(args.driver_buffer_size))
        , d_acquisition_mode(args.acquisition_mode)
        , d_poll_rate(args.streaming_mode_poll_rate)
        , d_downsampling_mode(args.downsampling_mode)
        , d_downsampling_factor(args.downsampling_factor)
        , d_ai_channels(args.ai_channels)
        , d_ports(args.ports)
        , d_channel_settings()
        , d_port_settings()
        , d_trigger_settings()
        , d_status(args.ai_channels)
        , d_app_buffer()
        , d_was_last_callback_timestamp_taken(false)
        , d_estimated_sample_rate(AVERAGE_HISTORY_LENGTH)
        , d_initialized(false)
        , d_closed(false)
        , d_armed(false)
        , d_auto_arm(args.auto_arm)
        , d_trigger_once(args.trigger_once)
        , d_was_triggered_once(false)
        , d_timebase_published(false)
        , ai_buffers(args.ai_channels)
        , ai_error_buffers(args.ai_channels)
        , port_buffers(args.ports)
        , d_data_rdy(false)
        , d_trigger_state(0)
        , d_read_idx(0)
        , d_buffer_samples(0)
        , d_errors(128)
        , d_poller_state(poller_state_t::IDLE) {
        d_ai_buffers       = std::vector<std::vector<float>>(d_ai_channels);
        d_ai_error_buffers = std::vector<std::vector<float>>(d_ai_channels);

        if (args.ports) {
            d_port_buffers = std::vector<std::vector<uint8_t>>(args.ports);
        }

        assert(d_ai_channels < MAX_SUPPORTED_AI_CHANNELS);
        assert(d_ports < MAX_SUPPORTED_PORTS);

        // TODO(PORT) check: baseline offered set_buffer_size to set bs directly, and set_samples(pre, post), which set bs = pre + post
        if (d_buffer_size == 0) {
            std::ostringstream message;
            message << "Exception in " << __FILE__ << ":" << __LINE__ << ": buffer size can't be zero:" << d_buffer_size;
            throw std::invalid_argument(message.str());
        }

        if (d_nr_buffers == 1) {
            std::ostringstream message;
            message << "Exception in " << __FILE__ << ":" << __LINE__ << ": number of buffers can't be zero:" << d_nr_buffers;
            throw std::invalid_argument(message.str());
        }

        if (d_driver_buffer_size == 0) {
            std::ostringstream message;
            message << "Exception in " << __FILE__ << ":" << __LINE__ << ": driver buffer size can't be zero:" << d_driver_buffer_size;
            throw std::invalid_argument(message.str());
        }

        if (d_post_samples == 0) {
            std::ostringstream message;
            message << "Exception in " << __FILE__ << ":" << __LINE__ << ": post-trigger samples can't be zero";
            throw std::invalid_argument(message.str());
        }
    }

    /**********************************************************************
     * Driver interface and handlers
     **********************************************************************/

    app_buffer_t *app_buffer() {
        return &d_app_buffer;
    }

    /*!
     * This function should be called when data is ready (rapid block only).
     */
    void notify_data_ready(std::error_code ec) {
        if (ec) {
            add_error_code(ec);
        }

        {
            boost::mutex::scoped_lock lock(d_mutex);
            d_data_rdy      = true;
            d_data_rdy_errc = ec;
        }

        d_data_rdy_cv.notify_one();
    }

    std::error_code wait_data_ready() {
        boost::mutex::scoped_lock lock(d_mutex);

        d_data_rdy_cv.wait(lock, [this] { return d_data_rdy; });
        return d_data_rdy_errc;
    }

    void clear_data_ready() {
        boost::mutex::scoped_lock lock(d_mutex);

        d_data_rdy      = false;
        d_data_rdy_errc = std::error_code{};
    }

    work_return_t work_rapid_block(work_io &wio) {
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
            } else if (ec) {
                d_logger->error("error occurred while waiting for data: {}", ec);
                return work_return_t::ERROR; // TODO(PORT) was 0
            }

            // we assume all the blocks are ready
            d_bstate.initialize(d_nr_captures);
        }

        if (d_bstate.state == rapid_block_state_t::READING_PART1) {
            // If d_trigger_once is true we will signal all done in the next iteration
            // with the block state set to WAITING
            d_was_triggered_once     = true;

            auto samples_to_fetch    = get_block_size();
            auto downsampled_samples = get_block_size_with_downsampling();

            // Instruct the driver to prefetch samples. Drivers might choose to ignore this call
            auto ec = _driver->prefetch_block(samples_to_fetch, d_bstate.waveform_idx);
            if (ec) {
                add_error_code(ec);
                return work_return_t::ERROR; // TODO(PORT) was -1
            }

            // Initiate state machine for the current waveform. Note state machine track
            // and adjust the waveform index.
            d_bstate.set_waveform_params(0, downsampled_samples);

            timespec start_time;
            clock_gettime(CLOCK_REALTIME, &start_time);
            uint64_t timestamp_now_ns_utc = (start_time.tv_sec * 1000000000) + (start_time.tv_nsec);
            // We are good to read first batch of samples
            noutput_items = std::min(noutput_items, d_bstate.samples_left);

            ec            = _driver->get_rapid_block_data(d_bstate.offset,
                               noutput_items, d_bstate.waveform_idx, wio, d_status);
            if (ec) {
                add_error_code(ec);
                return work_return_t::ERROR; // TODO(PORT) was -1
            }

            // Attach trigger info to value outputs and to all ports
            auto     vec_idx                               = 0;
            uint32_t pre_trigger_samples_with_downsampling = get_pre_trigger_samples_with_downsampling();
            double   time_per_sample_with_downsampling_ns  = d_time_per_sample_ns * d_downsampling_factor;

            for (auto i = 0; i < d_ai_channels && vec_idx < (int) wio.outputs().size(); i++, vec_idx += 2) {
                if (!d_channel_settings[i].enabled) {
                    continue;
                }

                auto trigger_tag = make_trigger_tag(
                        d_downsampling_factor,
                        timestamp_now_ns_utc + (pre_trigger_samples_with_downsampling * time_per_sample_with_downsampling_ns),
                        wio.outputs()[0].nitems_written() + pre_trigger_samples_with_downsampling,
                        d_status[i]);

                wio.outputs()[vec_idx].add_tag(trigger_tag);
            }

            auto trigger_tag = make_trigger_tag(
                    d_downsampling_factor,
                    timestamp_now_ns_utc + (pre_trigger_samples_with_downsampling * time_per_sample_with_downsampling_ns),
                    wio.outputs()[0].nitems_written() + pre_trigger_samples_with_downsampling,
                    0); // status

            // Add tags to digital port
            for (auto i = 0; i < d_ports && vec_idx < (int) wio.outputs().size(); i++, vec_idx++) {
                if (d_port_settings[i].enabled)
                    wio.outputs()[vec_idx].add_tag(trigger_tag);
            }

            // update state
            d_bstate.update_state(noutput_items);
            wio.produce_each(noutput_items);
            return work_return_t::OK;
        } else if (d_bstate.state == rapid_block_state_t::READING_THE_REST) {
            noutput_items = std::min(noutput_items, d_bstate.samples_left);

            auto ec       = _driver->get_rapid_block_data(d_bstate.offset, noutput_items,
                          d_bstate.waveform_idx, wio, d_status);
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

    work_return_t work_stream(work_io &wio) {
        // used for debugging in order to see how often gr calls this block
        //     uint64_t now = get_timestamp_milli_utc();
        //     if( now - last_call_utc > 20)
        //       std::cout << "now - last_call_utc [ms]: " << now - last_call_utc << std::endl;
        //     last_call_utc = get_timestamp_milli_utc();

        auto noutput_items = static_cast<int>(wio.outputs()[0].n_items);
        assert(noutput_items >= static_cast<int>(d_buffer_size));

        // process only one buffer per iteration
        noutput_items = d_buffer_size;

        // wait data on application buffer
        auto ec = d_app_buffer.wait_data_ready();

        if (ec) {
            add_error_code(ec);
        }

        if (ec == digitizer_block_errc::Stopped) {
            d_logger->info("stop requested");
            return work_return_t::DONE; // stop // TODO(PORT) was -1;
        } else if (ec == digitizer_block_errc::Watchdog) {
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

        int output_items_idx = 0;
        int buff_idx         = 0;
        int port_idx         = 0;

        for (auto i = 0; i < d_ai_channels; i++) {
            if (d_channel_settings[i].enabled) {
                ai_buffers[buff_idx] = wio.outputs()[output_items_idx].items<float>();
                output_items_idx++;
                ai_error_buffers[buff_idx] = wio.outputs()[output_items_idx].items<float>();
                output_items_idx++;
                buff_idx++;
            } else {
                output_items_idx += 2; // Skip disabled channels
            }
        }

        for (auto i = 0; i < d_ports; i++) {
            if (d_port_settings[i].enabled) {
                port_buffers[port_idx] = wio.outputs()[output_items_idx].items<uint8_t>();
                output_items_idx++;
                port_idx++;
            } else {
                output_items_idx++;
            }
        }

        // This will write samples directly into GR output buffers
        std::vector<uint32_t> channel_status;

        int64_t               timestamp_now_ns_utc;
        auto                  lost_count = d_app_buffer.get_data_chunk(ai_buffers, ai_error_buffers, port_buffers, channel_status, timestamp_now_ns_utc);
        // std::cout << "timestamp_now_ns_utc: " << int64_t(timestamp_now_ns_utc) << std::endl;

        if (lost_count) {
            d_logger->error("{} digitizer data buffers lost. Usually the cause of this error is, that the work method of the Digitizer block is called with low frequency because of a 'traffic jam' in the flowgraph. (One of the next blocks cannot process incoming data in time)", lost_count);
        }

        // Compile acquisition info tag
        acq_info_t tag_info{};

        tag_info.timestamp    = timestamp_now_ns_utc;
        tag_info.timebase     = get_timebase_with_downsampling();
        tag_info.user_delay   = 0.0;
        tag_info.actual_delay = 0.0;

        // Attach tags to the channel values...

        int output_idx = 0;

        for (auto i = 0; i < d_ai_channels; i++) {
            if (d_channel_settings[i].enabled) {
                // add channel specific status
                tag_info.status = channel_status.at(i);

                auto tag        = make_acq_info_tag(tag_info, wio.outputs()[0].nitems_written());
                wio.outputs()[output_idx].add_tag(tag);
            }
            output_idx += 2;
        }

        // ...and to all digital ports
        tag_info.status = 0;
        auto tag        = make_acq_info_tag(tag_info, wio.outputs()[0].nitems_written());

        for (auto i = 0; i < d_ports; i++) {
            if (d_port_settings[i].enabled) {
                wio.outputs()[output_idx].add_tag(tag);
            }
            output_idx++;
        }

        // Software-based trigger detection
        std::vector<int> trigger_offsets;

        if (d_trigger_settings.is_analog()) {
            // TODO: improve, check selected trigger on arm
            const auto aichan     = convert_to_aichan_idx(d_trigger_settings.source);
            auto       output_idx = aichan * 2; // ignore error outputs

            auto       buffer     = static_cast<const float *>(wio.outputs()[output_idx].raw_items());
            trigger_offsets       = find_analog_triggers(buffer, d_buffer_size);
        } else if (d_trigger_settings.is_digital()) {
            auto port       = d_trigger_settings.pin_number / 8;
            auto pin        = d_trigger_settings.pin_number % 8;
            auto mask       = 1 << pin;

            auto buffer     = wio.outputs()[wio.outputs().size() - d_ports + port].items<uint8_t>();
            trigger_offsets = find_digital_triggers(buffer, d_buffer_size, mask);
        }

        double time_per_sample_with_downsampling_ns = d_time_per_sample_ns * d_downsampling_factor;

        // Attach trigger tags
        for (auto trigger_offset : trigger_offsets) {
            //       std::cout << "trigger_offset       : " << trigger_offset<<std::endl;
            //       std::cout << "noutput_items        : " << noutput_items <<std::endl;
            //       std::cout << "d_time_per_sample_ns : " << time_per_sample_with_downsampling_ns <<std::endl;
            //       std::cout << "local_timstamp        : " << local_timstamp << std::endl;
            //       std::cout << "another now           :" << now << std::endl;
            //       std::cout << "timestamp_now_ns_utc  : " << timestamp_now_ns_utc<<std::endl;
            //       std::cout << "diff[ns]             : " << uint64_t((noutput_items - trigger_offset ) * time_per_sample_with_downsampling_ns )<<std::endl;
            //       std::cout << "stamp added           : " << uint64_t(timestamp_now_ns_utc - (( noutput_items - trigger_offset ) * time_per_sample_with_downsampling_ns )) <<std::endl;
            //       std::cout << "tag offset: " << nitems_written(0) + trigger_offset <<std::endl;
            auto trigger_tag = make_trigger_tag(
                    d_downsampling_factor,
                    timestamp_now_ns_utc - uint64_t((noutput_items - trigger_offset) * time_per_sample_with_downsampling_ns),
                    wio.outputs()[0].nitems_written() + trigger_offset,
                    0); // status

            int output_idx = 0;

            for (auto i = 0; i < d_ai_channels; i++) {
                if (d_channel_settings[i].enabled) {
                    wio.outputs()[output_idx].add_tag(trigger_tag);
                }
                output_idx += 2;
            }

            for (auto i = 0; i < d_ports; i++) {
                if (d_port_settings[i].enabled) {
                    wio.outputs()[output_idx].add_tag(trigger_tag);
                }
                output_idx++;
            }
        }

        wio.produce_each(noutput_items);
        return work_return_t::OK;
    }

    /**********************************************************************
     * Helpers
     **********************************************************************/

    uint32_t get_pre_trigger_samples_with_downsampling() const {
        if (d_downsampling_mode != downsampling_mode_t::NONE)
            return d_pre_samples / d_downsampling_factor;
        return d_pre_samples;
    }

    uint32_t get_post_trigger_samples_with_downsampling() const {
        if (d_downsampling_mode != downsampling_mode_t::NONE)
            return d_post_samples / d_downsampling_factor;
        return d_post_samples;
    }

    /*!
     * Returns number of pre-triggers samples plus number of post-trigger samples.
     */
    uint32_t get_block_size() const {
        return d_post_samples + d_pre_samples;
    }

    uint32_t get_block_size_with_downsampling() const {
        return get_pre_trigger_samples_with_downsampling() + get_post_trigger_samples_with_downsampling();
    }

    int convert_to_aichan_idx(const std::string &id) const {
        if (id.length() != 1) {
            std::ostringstream message;
            message << "Exception in " << __FILE__ << ":" << __LINE__ << ": aichan id should be a single character: " << id;
            throw std::invalid_argument(message.str());
        }

        int idx = std::toupper(id[0]) - 'A';
        if (idx < 0 || idx > MAX_SUPPORTED_AI_CHANNELS) {
            std::ostringstream message;
            message << "Exception in " << __FILE__ << ":" << __LINE__ << ": invalid aichan id: " << id;
            throw std::invalid_argument(message.str());
        }

        return idx;
    }

    int convert_to_port_idx(const std::string &id) const {
        if (id.length() != 5) {
            std::ostringstream message;
            message << "Exception in " << __FILE__ << ":" << __LINE__ << ": invalid port id: " << id << ", should be of the following format 'port<d>'";
            throw std::invalid_argument(message.str());
        }

        int idx = boost::lexical_cast<int>(id[4]);
        if (idx < 0 || idx > MAX_SUPPORTED_PORTS) {
            std::ostringstream message;
            message << "Exception in " << __FILE__ << ":" << __LINE__ << ": invalid port number: " << id;
            throw std::invalid_argument(message.str());
        }

        return idx;
    }

    /*!
     * \brief Distance between output items in seconds.
     *
     * This function takes into account downsampling factor (if set).
     */
    double get_timebase_with_downsampling() const {
        if (d_downsampling_mode == downsampling_mode_t::NONE) {
            return 1.0 / d_actual_samp_rate;
        } else {
            return d_downsampling_factor / d_actual_samp_rate;
        }
    }

    /*!
     * \brief This function searches for an edge (trigger) in the streaming buffer. It returns
     * relative offsets of all detected edges.
     */
    std::vector<int> find_analog_triggers(float const *const samples, int nsamples) {
        std::vector<int> trigger_offsets; // relative offset of detected triggers

        assert(nsamples >= 0);

        if (!d_trigger_settings.is_enabled() || nsamples == 0) {
            return trigger_offsets;
        }

        assert(d_trigger_settings.is_analog());

        auto aichan = convert_to_aichan_idx(d_trigger_settings.source);

        if (d_trigger_settings.direction == trigger_direction_t::RISING
                || d_trigger_settings.direction == trigger_direction_t::HIGH) {
            float band = d_channel_settings[aichan].range / 100.0;
            float lo   = static_cast<float>(d_trigger_settings.threshold - band);

            for (auto i = 0; i < nsamples; i++) {
                if (!d_trigger_state && samples[i] >= d_trigger_settings.threshold) {
                    d_trigger_state = 1;
                    trigger_offsets.push_back(i);
                } else if (d_trigger_state && samples[i] <= lo) {
                    d_trigger_state = 0;
                }
            }
        } else if (d_trigger_settings.direction == trigger_direction_t::FALLING
                   || d_trigger_settings.direction == trigger_direction_t::LOW) {
            float band = d_channel_settings[aichan].range / 100.0;
            float hi   = static_cast<float>(d_trigger_settings.threshold + band);

            for (auto i = 0; i < nsamples; i++) {
                if (d_trigger_state && samples[i] <= d_trigger_settings.threshold) {
                    d_trigger_state = 0;
                    trigger_offsets.push_back(i);
                } else if (!d_trigger_state && samples[i] >= hi) {
                    d_trigger_state = 1;
                }
            }
        }

        return trigger_offsets;
    }

    std::vector<int> find_digital_triggers(uint8_t const *const samples, int nsamples, uint8_t mask) {
        std::vector<int> trigger_offsets;

        if (d_trigger_settings.direction == trigger_direction_t::RISING
                || d_trigger_settings.direction == trigger_direction_t::HIGH) {
            for (auto i = 0; i < nsamples; i++) {
                if (!d_trigger_state && (samples[i] & mask)) {
                    d_trigger_state = 1;
                    trigger_offsets.push_back(i);
                } else if (d_trigger_state && !(samples[i] & mask)) {
                    d_trigger_state = 0;
                }
            }
        } else if (d_trigger_settings.direction == trigger_direction_t::FALLING
                   || d_trigger_settings.direction == trigger_direction_t::LOW) {
            for (auto i = 0; i < nsamples; i++) {
                if (d_trigger_state && !(samples[i] & mask)) {
                    d_trigger_state = 0;
                    trigger_offsets.push_back(i);
                } else if (!d_trigger_state && (samples[i] & mask)) {
                    d_trigger_state = 1;
                }
            }
        }

        return trigger_offsets;
    }

    /*!
     * \brief Poll worker function. The thread exits if stop is requested or call to driver_poll
     * returns with error.
     */
    void poll_work_function() {
        boost::unique_lock<boost::mutex> lock(d_poller_mutex, boost::defer_lock);
        std::chrono::duration<float>     poll_duration(d_poll_rate);
        std::chrono::duration<float>     sleep_time;

#ifdef PORT_DISABLED // use std::thread API?
        gr::thread::set_thread_name(pthread_self(), "poller");
#endif

        // relax cpu with less lock calls.
        unsigned int   check_every_n_times        = 10;
        unsigned int   poller_state_check_counter = check_every_n_times;
        poller_state_t state                      = poller_state_t::IDLE;

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
                auto ec         = _driver->poll();
                if (ec) {
                    // Only print out an error message
                    d_logger->error("poll failed with: {}", ec);
                    // Notify work method about the error... Work method will re-arm the driver if required.
                    d_app_buffer.notify_data_ready(ec);

                    // Prevent error-flood on close
                    if (d_closed)
                        return;
                }

                // Watchdog is "turned on" only some time after the acquisition start for two reasons:
                // - to avoid false positives
                // - to avoid fast rearm attempts
                float estimated_samp_rate = 0.0;
                {
                    // Note, mutex is not needed in case of PicoScope implementations but in order to make
                    // the base class relatively generic we use mutex (streaming callback is called from this
                    //  thread).
                    boost::mutex::scoped_lock watchdog_guard(d_watchdog_mutex);
                    estimated_samp_rate = d_estimated_sample_rate.get_avg_value();
                }

                if (estimated_samp_rate < (d_actual_samp_rate * WATCHDOG_SAMPLE_RATE_THRESHOLD)) {
                    // This will wake up the worker thread (see do_work method), and that thread will
                    // then rearm the device...
                    d_logger->error("Watchdog: estimated sample rate {}Hz, expected: {}Hz", estimated_samp_rate, d_actual_samp_rate);
                    d_app_buffer.notify_data_ready(digitizer_block_errc::Watchdog);
                }

                // Substract the time each iteration itself took in order to get closer to the desired poll duration
                std::chrono::duration<float> elapsed_poll_duration = std::chrono::high_resolution_clock::now() - poll_start;
                if (elapsed_poll_duration > poll_duration) {
                    sleep_time = std::chrono::duration<float>::zero();
                } else {
                    sleep_time = poll_duration - elapsed_poll_duration;
                }
                std::this_thread::sleep_for(sleep_time);
            } else {
                if (state == poller_state_t::PEND_IDLE) {
                    lock.lock();
                    d_poller_state = state = poller_state_t::IDLE;
                    lock.unlock();

                    d_poller_cv.notify_all();
                } else if (state == poller_state_t::PEND_EXIT) {
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

    /*!
     * \brief Start the poller thread if it isn't running yet.
     */
    void start_poll_thread() {
        if (!d_poller.joinable()) {
            boost::mutex::scoped_lock guard(d_poller_mutex);
            d_poller_state = poller_state_t::IDLE;
            d_poller       = boost::thread(&digitizer_block_impl::poll_work_function, this);
        }
    }

    /*!
     * \brief Stop & join the poller thread.
     */
    void stop_poll_thread() {
        if (!d_poller.joinable()) {
            return;
        }

        boost::unique_lock<boost::mutex> lock(d_poller_mutex);
        d_poller_state = poller_state_t::PEND_EXIT;
        d_poller_cv.wait_for(lock, boost::chrono::seconds(5),
                [this] { return d_poller_state == poller_state_t::EXIT; });
        lock.unlock();

        d_poller.join();
    }

    /*!
     * \brief To prevent poll requests during re-arm.
     */
    void transit_poll_thread_to_idle() {
        boost::unique_lock<boost::mutex> lock(d_poller_mutex);

        if (d_poller_state == poller_state_t::EXIT) {
            return; // nothing to do
        }

        d_poller_state = poller_state_t::PEND_IDLE;
        d_poller_cv.wait(lock, [this] { return d_poller_state == poller_state_t::IDLE; });
    }

    /*!
     * \brief Change state to running. Poll work function will start executing driver_poll
     * requests.
     */
    void transit_poll_thread_to_running() {
        boost::mutex::scoped_lock guard(d_poller_mutex);
        d_poller_state = poller_state_t::RUNNING;
    }

    /*!
     * \brief Add error code to the circular buffer. Clients are able to read last N error
     * codes together with timestamps by using the get_errors method.
     *
     * This method is meant to be used by the digitizer class-level code and all the driver
     * implementations.
     *
     * This method is thread safe.
     */
    void add_error_code(std::error_code ec) {
        d_errors.push(ec);
    }

    /**********************************************************************
     * Members
     *********************************************************************/

    std::shared_ptr<T> _driver;
    gr::logger_ptr     d_logger;

    // Sample rate in Hz
    double d_samp_rate;
    double d_actual_samp_rate;

    double d_time_per_sample_ns;

    // Number of pre- and post-trigger samples the user wants to see on the outputs.
    // Note when calculating actual number of pre- and post-trigger samples one should
    // take into account the user delay and realignment delay.
    uint32_t d_pre_samples;
    uint32_t d_post_samples;

    // Number of captures in rapid block mode
    uint32_t d_nr_captures;

    // Buffer size and number of buffers in streaming mode
    uint32_t                     d_buffer_size;
    uint32_t                     d_nr_buffers;

    uint32_t                     d_driver_buffer_size;

    digitizer_acquisition_mode_t d_acquisition_mode;
    double                       d_poll_rate;
    downsampling_mode_t          d_downsampling_mode;
    uint32_t                     d_downsampling_factor;

    // Number of channels and ports
    int d_ai_channels;
    int d_ports;

    // Channel and trigger settings
    std::array<channel_setting_t, MAX_SUPPORTED_AI_CHANNELS> d_channel_settings;
    std::array<port_setting_t, MAX_SUPPORTED_PORTS>          d_port_settings;
    trigger_setting_t                                        d_trigger_settings;

    std::vector<uint32_t>                                    d_status;

    // application buffer
    app_buffer_t d_app_buffer;

    // watchdog
    using hr_time_point = boost::chrono::high_resolution_clock::time_point;

    // For samp_rate estimation
    hr_time_point         d_last_callback_timestamp;
    bool                  d_was_last_callback_timestamp_taken;

    average_filter<float> d_estimated_sample_rate;
    boost::mutex          d_watchdog_mutex;

    // Flags
    bool d_initialized;
    bool d_closed;
    bool d_armed;
    bool d_auto_arm;
    bool d_trigger_once;
    bool d_was_triggered_once;
    bool d_timebase_published;

    // copy analog channel data array addresses to local application reference for enabled channels
    std::vector<float *> ai_buffers;
    std::vector<float *> ai_error_buffers;

    // copy digital channel data array addresses to local application reference for enabled ports
    std::vector<uint8_t *> port_buffers;

private:
    // Acquisition, note boost constructs are used in order for the GR
    // scheduler to be able to interrupt worker thread on stop.
    boost::condition_variable d_data_rdy_cv;
    bool                      d_data_rdy;
    boost::mutex              d_mutex;
    std::error_code           d_data_rdy_errc;

    // Worker stuff
    rapid_block_state_t               d_bstate;

    int                               d_trigger_state;

    std::vector<std::vector<float>>   d_ai_buffers;
    std::vector<std::vector<float>>   d_ai_error_buffers;
    std::vector<std::vector<uint8_t>> d_port_buffers;

    // A vector holding status information for pre-trigger number of samples located in
    // the buffer
    std::vector<uint32_t> d_status_pre;

    int                   d_read_idx;
    uint32_t              d_buffer_samples;

    error_buffer_t        d_errors;

    // Poller
    boost::thread             d_poller;
    poller_state_t            d_poller_state;
    boost::mutex              d_poller_mutex;
    boost::condition_variable d_poller_cv;

    std::string               d_configure_exception_message;

    // Relevant for debugging. If gnuradio calls this block not often enough, we will get "WARN: XX digitizer data buffers lost"
    // The rate in which this block is called is given by the number of free slots on its output buffer. So we choose a big value via set_min_output_buffer
    uint64_t last_call_utc = 0;
};

} // namespace gr::digitizers
