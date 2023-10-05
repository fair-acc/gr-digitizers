#ifndef FAIR_PICOSCOPE4000A_PICOSCOPE4000A_HPP
#define FAIR_PICOSCOPE4000A_PICOSCOPE4000A_HPP

#include <picoscope.hpp>

namespace fair::picoscope4000a {

struct Picoscope4000a : public fair::picoscope::Picoscope<Picoscope4000a> {
    using AnalogPort = fair::graph::PortOut<float>;

    std::array<fair::graph::PortOut<float>, 8> values;
    std::array<fair::graph::PortOut<float>, 8> errors;

    fair::graph::work_return_t
    work(std::size_t requested_work = 0) {
        std::ignore = requested_work;
        return this->work_impl();
    }

    std::error_code
                           set_buffers(size_t samples, uint32_t block_number);

    static constexpr float DRIVER_VERTICAL_PRECISION = 0.01f;

    static std::optional<std::size_t>
    driver_channel_id_to_index(std::string_view id);
    std::string
    driver_driver_version() const;
    std::string
    driver_hardware_version() const;
    fair::picoscope::GetValuesResult
    driver_rapid_block_get_values(std::size_t capture, std::size_t nr_samples);
    std::error_code
    driver_initialize();
    std::error_code
    driver_close();
    std::error_code
    driver_configure();
    std::error_code
    driver_arm();
    std::error_code
    driver_disarm() noexcept;
    std::error_code
    driver_poll();
};

} // namespace fair::picoscope4000a

ENABLE_REFLECTION(fair::picoscope4000a::Picoscope4000a, values, errors, serial_number, sample_rate, pre_samples, post_samples, acquisition_mode_string, rapid_block_nr_captures, streaming_mode_poll_rate, auto_arm, trigger_once);

#endif
