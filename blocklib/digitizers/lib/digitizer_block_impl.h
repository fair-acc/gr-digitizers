#pragma once

#include "app_buffer.h"
#include "error.h"
#include "range.h"
#include "utils.h"

#include <gnuradio/digitizers/digitizer_enums.h>

#include <gnuradio/block.h>

#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

#include <system_error>
#include <span>

namespace gr::digitizers {

enum class digitizer_block_errc {
    Stopped = 1,
    Interrupted = 10, // did not respond in time,
    Watchdog = 11,    // no or too little samples received in time
};

std::error_code make_error_code(digitizer_block_errc e);

} // namespace gr::digitizers

// Hook in our error code visible to the std system error
namespace std {
template <>
struct is_error_code_enum<gr::digitizers::digitizer_block_errc> : true_type {
};
} // namespace std

namespace gr::digitizers {

/**********************************************************************
 * Hardcoded values
 **********************************************************************/

// Watchdog is triggered if estimated sample rate falls below 75%
static const float WATCHDOG_SAMPLE_RATE_THRESHOLD = 0.75;

/**********************************************************************
 * Helpers and struct definitions
 **********************************************************************/

/*!
 * A helper struct for keeping track which samples have been already processes
 * in rapid block mode.
 */
struct rapid_block_state_t {
    enum State { WAITING, READING_PART1, READING_THE_REST };

    rapid_block_state_t()
        : state(WAITING), waveform_count(0), waveform_idx(0), offset(0), samples_left(0)
    {
    }

    State state;

    int waveform_count;
    int waveform_idx; // index of the waveform we are currently reading
    int offset;       // reading offset
    int samples_left;

    void to_wait() { state = rapid_block_state_t::WAITING; }

    void initialize(int nr_waveforms)
    {
        state = rapid_block_state_t::READING_PART1;
        waveform_idx = 0;
        waveform_count = nr_waveforms;
    }

    void set_waveform_params(uint32_t offset_samps, uint32_t samples_to_read)
    {
        offset = offset_samps;
        samples_left = samples_to_read;
    }

    // update state
    void update_state(uint32_t nsamples)
    {
        offset += nsamples;
        samples_left -= nsamples;

        if (samples_left > 0) {
            state = rapid_block_state_t::READING_THE_REST;
        }
        else {
            waveform_idx++;
            if (waveform_idx >= waveform_count) {
                state = rapid_block_state_t::WAITING;
            }
            else {
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
        : range(2.0), offset(0.0), enabled(false), coupling(coupling_t::AC_1M)
    {
    }

    double range;
    float offset;
    bool enabled;
    coupling_t coupling;
};

struct port_setting_t {
    port_setting_t() : logic_level(1.5), enabled(false) {}

    float logic_level;
    bool enabled;
};

static const std::string TRIGGER_NONE_SOURCE = "NONE";
static const std::string TRIGGER_DIGITAL_SOURCE =
    "DI"; // DI is as well used as "AUX" for p6000 scopes

struct trigger_setting_t {
    trigger_setting_t()
        : source(TRIGGER_NONE_SOURCE),
          threshold(0),
          direction(trigger_direction_t::RISING),
          pin_number(0)
    {
    }

    bool is_enabled() const { return source != TRIGGER_NONE_SOURCE; }

    bool is_digital() const { return is_enabled() && source == TRIGGER_DIGITAL_SOURCE; }

    bool is_analog() const { return is_enabled() && source != TRIGGER_DIGITAL_SOURCE; }

    std::string source;
    float threshold; // AI only
    trigger_direction_t direction;
    int pin_number; // DI only
};

/*!
 * \brief A simple circular buffer for keeping last N errors.
 */
class error_buffer_t
{
    boost::circular_buffer<error_info_t> cb;
    boost::mutex access;

public:
    error_buffer_t(int n) : cb(n), access() {}

    ~error_buffer_t() {}

    void push(error_info_t err)
    {
        boost::mutex::scoped_lock lg(access);
        cb.push_back(err);
    }

    void push(std::error_code ec) { push(error_info_t{ get_timestamp_nano_utc(), ec }); }

    // Note circular buffer is cleared at get
    std::vector<error_info_t> get()
    {
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
enum class poller_state_t { IDLE = 0, RUNNING, EXIT, PEND_EXIT, PEND_IDLE };

struct DIGITIZERS_API digitizer_args {
    double sample_rate = 10000;
    std::size_t buffer_size = 8192;
    std::size_t nr_buffers = 100;
    std::size_t driver_buffer_size = 100000;
    std::size_t pre_samples = 1000;
    std::size_t post_samples = 9000;
    acquisition_mode_t acquisition_mode = acquisition_mode_t::STREAMING;
    std::size_t rapid_block_nr_captures = 1;
    double streaming_mode_poll_rate = 0.001; // poll rate is in seconds
    downsampling_mode_t downsampling_mode = downsampling_mode_t::NONE;
    std::size_t downsampling_factor = 1;
    bool auto_arm = true;
    bool trigger_once = false;
    std::size_t ai_channels = 0;
    std::size_t ports = 0;
};

class DIGITIZERS_API digitizer_block_impl
{
    /**********************************************************************
     * Public API calls (see digitizer_block.h for docs)
     **********************************************************************/

public:
    static const int MAX_SUPPORTED_AI_CHANNELS = 16;
    static const int MAX_SUPPORTED_PORTS = 8;

    acquisition_mode_t get_acquisition_mode() const;

    double get_samp_rate() const;

    void set_aichan(const std::string& id,
                    bool enabled,
                    double range,
                    coupling_t coupling,
                    double range_offset = 0);

    /*!
     * \brief Returns number of enabled analog channels.
     */
    int get_enabled_aichan_count() const;

    void set_aichan_range(const std::string& id, double range, double range_offset = 0);

    void set_aichan_trigger(const std::string& id,
                            trigger_direction_t direction,
                            double threshold);

    void set_diport(const std::string& id, bool enabled, double thresh_voltage);

    /*!
     * \brief Returns number of enabled ports;
     */
    int get_enabled_diport_count() const;

    void set_di_trigger(uint32_t pin, trigger_direction_t direction);

    void disable_triggers();

    void initialize();

    void configure();

    void arm();

    bool is_armed();

    void disarm();

    void close();

    std::vector<error_info_t> get_errors();

    bool start();

    bool stop();

    // Where all the action really happens
    work_return_t work(work_io& wio);

    void handle_msg_timing(pmtv::pmt msg);

    std::string getConfigureExceptionMessage();

    /**********************************************************************
     * Structors
     **********************************************************************/

    virtual ~digitizer_block_impl();

protected:
    digitizer_block_impl(const digitizer_args& args, gr::logger_ptr logger);

    /**********************************************************************
     * Driver interface and handlers
     **********************************************************************/
    /*!
     * \brief Version number of device driver.
     *
     * \return Version number
     */
    virtual std::string get_driver_version() const = 0;

    /*!
     * \brief Hardware or firmware version number.
     *
     * \return Version number
     */
    virtual std::string get_hardware_version() const = 0;
    /*!
     * \brief Get AI channel names.
     * \return a vector of channel names, e.g. "A", "B", ...
     */
    virtual std::vector<std::string> get_aichan_ids() = 0;

    /*!
     * \brief Get available AI ranges.
     * \return available ranges
    s */
    virtual meta_range_t get_aichan_ranges() = 0;

    virtual std::error_code driver_initialize() = 0;

    virtual std::error_code driver_configure() = 0;

    virtual std::error_code driver_arm() = 0;

    virtual std::error_code driver_disarm() = 0;

    virtual std::error_code driver_close() = 0;

    virtual std::error_code driver_poll() = 0;

    /*!
     * This function should be called when data is ready (rapid block only).
     */
    void notify_data_ready(std::error_code errc);

    std::error_code wait_data_ready();

    void clear_data_ready();

    /*!
     * Note offset and length is in non-decimated samples
     */
    virtual std::error_code driver_prefetch_block(size_t length, size_t block_number) = 0;

    /*!
     * By offset and length we mean decimated samples, and offset is offset within the
     * subparts of data.
     */
    virtual std::error_code
    driver_get_rapid_block_data(size_t offset,
                                size_t length,
                                size_t waveform,
                                work_io& wio,
                                std::vector<uint32_t>& status) = 0;

    work_return_t work_rapid_block(work_io& wio);

    work_return_t work_stream(work_io& wio);

    /**********************************************************************
     * Helpers
     **********************************************************************/

    uint32_t get_pre_trigger_samples_with_downsampling() const;

    uint32_t get_post_trigger_samples_with_downsampling() const;

    /*!
     * Returns number of pre-triggers samples plus number of post-trigger samples.
     */
    uint32_t get_block_size() const;

    uint32_t get_block_size_with_downsampling() const;

    int convert_to_aichan_idx(const std::string& id) const;

    int convert_to_port_idx(const std::string& id) const;

    /*!
     * \brief Distance between output items in seconds.
     *
     * This function takes into account downsampling factor (if set).
     */
    double get_timebase_with_downsampling() const;

    /*!
     * \brief This function searches for an edge (trigger) in the streaming buffer. It
     * returns relative offsets of all detected edges.
     */
    std::vector<std::size_t> find_analog_triggers(std::span<const float> samples);

    std::vector<std::size_t> find_digital_triggers(std::span<const uint8_t> samples,
                                                   uint8_t pin_mask);

    void dissect_data_chunk(std::span<const uint8_t> chunk_data,
                            std::vector<std::span<const float>>& ai_buffers,
                            std::vector<std::span<const float>>& ai_error_buffers,
                            std::vector<std::span<const uint8_t>>& port_buffers);

    /*!
     * \brief Poll worker function. The thread exits if stop is requested or call to
     * driver_poll returns with error.
     */
    void poll_work_function();

    /*!
     * \brief Start the poller thread if it isn't running yet.
     */
    void start_poll_thread();

    /*!
     * \brief Stop & join the poller thread.
     */
    void stop_poll_thread();

    /*!
     * \brief To prevent poll requests during re-arm.
     */
    void transit_poll_thread_to_idle();

    /*!
     * \brief Change state to running. Poll work function will start executing driver_poll
     * requests.
     */
    void transit_poll_thread_to_running();

    /*!
     * \brief Add error code to the circular buffer. Clients are able to read last N error
     * codes together with timestamps by using the get_errors method.
     *
     * This method is meant to be used by the digitizer class-level code and all the
     * driver implementations.
     *
     * This method is thread safe.
     */
    void add_error_code(std::error_code ec);

    /**********************************************************************
     * Members
     *********************************************************************/

    logger_ptr d_logger;

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
    uint32_t d_buffer_size;
    uint32_t d_nr_buffers;

    uint32_t d_driver_buffer_size;

    acquisition_mode_t d_acquisition_mode;
    double d_poll_rate;
    downsampling_mode_t d_downsampling_mode;
    uint32_t d_downsampling_factor;

    // Number of channels and ports
    int d_ai_channels;
    int d_ports;

    // Channel and trigger settings
    std::array<channel_setting_t, MAX_SUPPORTED_AI_CHANNELS> d_channel_settings;
    std::array<port_setting_t, MAX_SUPPORTED_PORTS> d_port_settings;
    trigger_setting_t d_trigger_settings;

    std::vector<uint32_t> d_status;

    // application buffer
    app_buffer_t d_app_buffer;

    // watchdog
    using hr_time_point = boost::chrono::high_resolution_clock::time_point;

    // For samp_rate estimation
    hr_time_point d_last_callback_timestamp;
    bool d_was_last_callback_timestamp_taken;

    average_filter<float> d_estimated_sample_rate;
    boost::mutex d_watchdog_mutex;

    // Flags
    bool d_initialized;
    bool d_closed;
    bool d_armed;
    bool d_auto_arm;
    bool d_trigger_once;
    bool d_was_triggered_once;
    bool d_timebase_published;

    // data held while waiting for timing messages needed to create trigger tags
    struct {
        app_buffer_t::data_chunk_ptr data_chunk;
        std::vector<std::size_t> trigger_offsets;
    } d_pending_data;

    struct timing_message_t {
        std::string name;
        std::chrono::nanoseconds timestamp;
        std::chrono::nanoseconds offset;
    };

    std::deque<timing_message_t> d_timing_messages;

private:
    // Acquisition, note boost constructs are used in order for the GR
    // scheduler to be able to interrupt worker thread on stop.
    boost::condition_variable d_data_rdy_cv;
    bool d_data_rdy;
    boost::mutex d_mutex;
    std::error_code d_data_rdy_errc;

    // Worker stuff
    rapid_block_state_t d_bstate;

    int d_trigger_state;

    std::vector<std::vector<float>> d_ai_buffers;
    std::vector<std::vector<float>> d_ai_error_buffers;
    std::vector<std::vector<uint8_t>> d_port_buffers;

    // A vector holding status information for pre-trigger number of samples located in
    // the buffer
    std::vector<uint32_t> d_status_pre;

    int d_read_idx;
    uint32_t d_buffer_samples;

    error_buffer_t d_errors;

    // Poller
    boost::thread d_poller;
    poller_state_t d_poller_state;
    boost::mutex d_poller_mutex;
    boost::condition_variable d_poller_cv;

    std::string d_configure_exception_message;
};

} // namespace gr::digitizers
