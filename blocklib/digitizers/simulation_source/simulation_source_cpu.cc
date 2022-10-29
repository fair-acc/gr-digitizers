#include "simulation_source_cpu.h"
#include "simulation_source_cpu_gen.h"

namespace gr::digitizers {

simulation_source_cpu::simulation_source_cpu(block_args args)
    : INHERITED_CONSTRUCTORS
    , d_impl({ .sample_rate                    = args.sample_rate,
                     .buffer_size              = args.buffer_size,
                     .nr_buffers               = args.nr_buffers,
                     .driver_buffer_size       = args.driver_buffer_size,
                     .pre_samples              = args.pre_samples,
                     .post_samples             = args.post_samples,
                     .auto_arm                 = args.auto_arm,
                     .trigger_once             = args.trigger_once,
                     .rapid_block_nr_captures  = args.rapid_block_nr_captures,
                     .streaming_mode_poll_rate = args.streaming_mode_poll_rate,
                     .acquisition_mode         = args.acquisition_mode,
                     .downsampling_mode        = args.downsampling_mode,
                     .downsampling_factor      = args.downsampling_factor,
                     .ai_channels              = 2,
                     .ports                    = 1 },
              simulation_driver(args.acquisition_mode, args.buffer_size), d_logger) {
    auto notify = [&](const std::error_code &errc) {
        d_impl.notify_data_ready(errc);
    };

    d_impl._driver.d_notify_data_ready_cb = notify;
    d_impl._driver.d_app_buffer           = d_impl.app_buffer();

    set_output_multiple(args.buffer_size);

    // Enable all channels and ports
    d_impl.set_aichan("A", true, 20.0, coupling_t::AC_1M);
    d_impl.set_aichan("B", true, 20.0, coupling_t::AC_1M);
    d_impl.set_diport("port0", true, 0.7);
}

bool simulation_source_cpu::start() {
    return d_impl.start();
}

bool simulation_source_cpu::stop() {
    return d_impl.stop();
}

void simulation_source_cpu::set_data(std::vector<float> channel_a_data, std::vector<float> channel_b_data, std::vector<uint8_t> port_data) {
    d_impl._driver.d_ch_a_data = std::move(channel_a_data);
    d_impl._driver.d_ch_b_data = std::move(channel_b_data);
    d_impl._driver.d_port_data = std::move(port_data);
}

work_return_t simulation_source_cpu::work(work_io &wio) {
    return d_impl.work(wio);
}

} // namespace gr::digitizers
