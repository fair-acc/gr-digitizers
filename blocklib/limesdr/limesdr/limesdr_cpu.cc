#include "limesdr_cpu.h"
#include "limesdr_cpu_gen.h"

#include "utils.h"
#include <digitizers/status.h>

#include <cstring>
#include <optional>

using gr::digitizers::acquisition_mode_t;
using gr::digitizers::channel_status_t;
using gr::digitizers::coupling_t;
using gr::digitizers::downsampling_mode_t;
using gr::digitizers::meta_range_t;
using gr::digitizers::range_t;
using gr::digitizers::trigger_direction_t;

struct LimeSdrErrc : std::error_category {
    const char* name() const noexcept override;
    std::string message(int ev) const override;
};

const char* LimeSdrErrc::name() const noexcept { return "LimeSDR"; }

std::string LimeSdrErrc::message(int ev) const
{
    // TODO for details, we would have to register a log handler via
    // LMS_RegisterLogHandler
    return "Unknown error";
}

const LimeSdrErrc theErrCategory{};

static std::error_code make_error_code(int e) { return { e, theErrCategory }; }

namespace gr::limesdr {

limesdr_impl::limesdr_impl(const digitizers::digitizer_args& args,
                           std::string serial_number,
                           logger_ptr logger)
    : digitizer_block_impl(args, logger), d_serial_number{ std::move(serial_number) }
{
}

limesdr_impl::~limesdr_impl() {}

std::vector<std::string> limesdr_impl::get_aichan_ids() { return {}; }

meta_range_t limesdr_impl::get_aichan_ranges() { return {}; }

std::string limesdr_impl::get_driver_version() const
{
    return fmt::format("LimeSuite {}", LMS_GetLibraryVersion());
}

std::string limesdr_impl::get_hardware_version() const
{
    return d_device ? d_device->hardware_version : "NA";
}

static std::optional<std::string_view> parse_serial(std::string_view s)
{
    constexpr std::string_view serial_prefix = "serial=";
    const auto prefix_pos = s.find(serial_prefix);
    if (prefix_pos == std::string_view::npos) {
        return std::nullopt;
    }

    s.remove_prefix(prefix_pos + serial_prefix.size());
    const auto comma_pos = s.find(",");
    if (comma_pos == std::string_view::npos) { // serial is last entry
        return s;
    }

    s.remove_suffix(s.size() - comma_pos - 1);
    return s;
}

std::error_code limesdr_impl::driver_initialize()
{
    d_logger->info("LimeSuite version: {}", LMS_GetLibraryVersion());

    std::array<lms_info_str_t, 20>
        list; // TODO size not passed, what happens if there are 21 devices?

    const auto device_count = LMS_GetDeviceList(list.data());
    if (device_count < 1) {
        d_logger->error("No Lime devices found");
        return make_error_code(1);
    }

    d_logger->info("Device list:");
    for (int i = 0; i < device_count; i++) {
        d_logger->info("Nr.: {} device: {}", i, list[i]);
    }

    std::size_t device_index = 0;
    std::string_view serial_number = d_serial_number;

    if (serial_number.empty()) {
        const auto parsed = parse_serial(list[0]);
        if (!parsed) {
            d_logger->error(
                "No serial number given, could not parse serial number from '{}'",
                list[0]);
            return make_error_code(1);
        }
        serial_number = *parsed;
        d_logger->info("No serial number given, using first device found: '{}'",
                       serial_number);
    }
    else {
        auto serial_matches = [&serial_number](lms_info_str_t s) {
            const auto parsed = parse_serial(s);
            return parsed && *parsed == serial_number;
        };

        const auto it = std::find_if(list.begin(), list.end(), serial_matches);
        if (it == list.end()) {
            d_logger->error("Device with serial number '{}' not found", serial_number);
            return make_error_code(1);
        }

        device_index = std::distance(list.begin(), it);
    }

    lms_device_t* address;
    const auto open_rc = LMS_Open(&address, list[device_index], nullptr);
    if (open_rc != LMS_SUCCESS) {
        d_logger->error("Could not open device '{}'", serial_number);
        return make_error_code(1);
    }

    auto dev = std::make_unique<device>(address);
    const auto init_rc = LMS_Init(dev->handle);
    if (init_rc != LMS_SUCCESS) {
        d_logger->error("Could not initialize device '{}'", serial_number);
        return make_error_code(1);
    }

    const auto dev_info = LMS_GetDeviceInfo(dev->handle);
    assert(dev_info);
    dev->hardware_version = fmt::format("Hardware: {} Firmware: {}",
                                        dev_info->hardwareVersion,
                                        dev_info->firmwareVersion);

    d_device = std::move(dev);
    return std::error_code{};
}

std::error_code limesdr_impl::driver_configure() { return std::error_code{}; }

std::error_code limesdr_impl::driver_arm() { return std::error_code{}; }

std::error_code limesdr_impl::driver_disarm() { return std::error_code{}; }

std::error_code limesdr_impl::driver_close()
{
    d_device.release();
    return std::error_code{};
}

std::error_code limesdr_impl::driver_prefetch_block(size_t samples, size_t block_number)
{
    return std::error_code{};
}

std::error_code limesdr_impl::driver_get_rapid_block_data(size_t offset,
                                                          size_t length,
                                                          size_t waveform,
                                                          work_io& wio,
                                                          std::vector<uint32_t>& status)
{
    return std::error_code{};
}

std::error_code limesdr_impl::driver_poll() { return std::error_code{}; }

limesdr_cpu::limesdr_cpu(block_args args)
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
            .ai_channels = 10, // TODO
            .ports = 2 },      // TODO
          args.serial_number,
          d_logger)
{
    set_output_multiple(args.buffer_size);
}

bool limesdr_cpu::start() { return d_impl.start(); }

bool limesdr_cpu::stop() { return d_impl.stop(); }

work_return_t limesdr_cpu::work(work_io& wio) { return d_impl.work(wio); }

void limesdr_cpu::initialize() { d_impl.initialize(); }

void limesdr_cpu::close() { d_impl.close(); }

} // namespace gr::limesdr
