#include "simulation_source_cpu.h"
#include "simulation_source_cpu_gen.h"

namespace gr::digitizers {

simulation_source_cpu::simulation_source_cpu(block_args args)
    : INHERITED_CONSTRUCTORS,
      d_impl(
          { .sample_rate = args.sample_rate,
            .buffer_size = args.buffer_size,
            .nr_buffers = args.nr_buffers,
            .driver_buffer_size = args.driver_buffer_size,
            .pre_samples = args.pre_samples,
            .post_samples = args.post_samples,
            // TODO(PORT) assumes that these enums (digitizer_impl header and enums.yml)
            // are identical, find out how to share enums.yml between modules
            .acquisition_mode = static_cast<acquisition_mode_t>(args.acquisition_mode),
            .rapid_block_nr_captures = args.rapid_block_nr_captures,
            .streaming_mode_poll_rate = args.streaming_mode_poll_rate,
            .downsampling_mode = static_cast<downsampling_mode_t>(args.downsampling_mode),
            .downsampling_factor = args.downsampling_factor,
            .auto_arm = args.auto_arm,
            .trigger_once = args.trigger_once,
            .ai_channels = 2,
            .ports = 1 },
          d_logger)
{
    set_output_multiple(args.buffer_size);
}

bool simulation_source_cpu::start() { return d_impl.start(); }

bool simulation_source_cpu::stop() { return d_impl.stop(); }

void simulation_source_cpu::set_data(std::vector<float> channel_a_data,
                                     std::vector<float> channel_b_data,
                                     std::vector<uint8_t> port_data)
{
    d_impl.d_ch_a_data = std::move(channel_a_data);
    d_impl.d_ch_b_data = std::move(channel_b_data);
    d_impl.d_port_data = std::move(port_data);

    // Enable channels with data set
    if (!d_impl.d_ch_a_data.empty()) {
        d_impl.set_aichan("A", true, 20.0, coupling_t::AC_1M);
    }
    if (!d_impl.d_ch_b_data.empty()) {
        d_impl.set_aichan("B", true, 20.0, coupling_t::AC_1M);
    }
    if (!d_impl.d_port_data.empty()) {
        d_impl.set_diport("port0", true, 0.7);
    }
}

void simulation_source_cpu::set_aichan_trigger(std::string channel_id,
                                               trigger_direction_t direction,
                                               double threshold)
{
    d_impl.set_aichan_trigger(channel_id, direction, threshold);
}

void simulation_source_cpu::set_di_trigger(uint8_t pin, trigger_direction_t direction)
{
    d_impl.set_di_trigger(pin, direction);
}

work_return_t simulation_source_cpu::work(work_io& wio) { return d_impl.work(wio); }

void simulation_source_cpu::handle_msg_timing(pmtv::pmt msg)
{
    d_impl.handle_msg_timing(std::move(msg));
}

} // namespace gr::digitizers
