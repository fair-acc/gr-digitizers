#include "limesdr_cpu.h"
#include "limesdr_cpu_gen.h"

#include "utils.h"
#include <digitizers/status.h>
#include <cstring>

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
    return "not implemented";
}

const LimeSdrErrc theErrCategory{};


namespace gr::limesdr {

std::vector<std::string> limesdr_impl::get_aichan_ids()
{
    return {};
}

meta_range_t limesdr_impl::get_aichan_ranges()
{
    return {};
}

std::string limesdr_impl::get_driver_version() const
{
    return "not implemented";
}

std::string limesdr_impl::get_hardware_version() const
{
    if (!d_initialized)
        return "NA";
    return "not implemented";
}

std::error_code limesdr_impl::driver_initialize()
{

    return std::error_code{};
}

std::error_code limesdr_impl::driver_configure()
{
    return std::error_code{};
}

std::error_code limesdr_impl::driver_arm()
{
    return std::error_code{};
}

std::error_code limesdr_impl::driver_disarm()
{
    return std::error_code{};
}

std::error_code limesdr_impl::driver_close()
{
    return std::error_code{};
}

std::error_code limesdr_impl::driver_prefetch_block(size_t samples,
                                                            size_t block_number)
{
    return std::error_code{};
}

std::error_code
limesdr_impl::driver_get_rapid_block_data(size_t offset,
                                                  size_t length,
                                                  size_t waveform,
                                                  work_io& wio,
                                                  std::vector<uint32_t>& status)
{
    return std::error_code{};
}

std::error_code limesdr_impl::driver_poll()
{
    return std::error_code{};
}

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
            .ports = 2 }, // TODO
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
