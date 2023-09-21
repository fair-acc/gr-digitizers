#ifndef GR_PICOSCOPE4000A_PICOSCOPE4000A_H
#define GR_PICOSCOPE4000A_PICOSCOPE4000A_H

#include <picoscope.h>

namespace gr::picoscope4000a {

struct Picoscope4000a : public gr::picoscope::Picoscope<Picoscope4000a> {
    fair::graph::OUT<float> values0;
    fair::graph::OUT<float> errors0;

    explicit Picoscope4000a(gr::picoscope::Settings settings = {}) : gr::picoscope::Picoscope<Picoscope4000a>(std::move(settings)) {}

    fair::graph::work_return_status_t process_bulk(std::span<float> values,
                                                   std::span<float> errors) noexcept
    {
        return this->process_bulk_impl<1>({{values, errors}});
    }

    std::make_signed_t<std::size_t>
    available_samples(const Picoscope&) const noexcept
    {
        return this->available_samples_impl();
    }

    std::error_code set_buffers(size_t samples, uint32_t block_number);

    static constexpr float DRIVER_VERTICAL_PRECISION = 0.01f;

    std::string driver_driver_version() const;
    std::string driver_hardware_version() const;
    std::error_code driver_initialize();
    std::error_code driver_close();
    std::error_code driver_configure();
    std::error_code driver_arm();
    std::error_code driver_disarm();
    std::error_code driver_poll();
};

} // namespace gr::picoscope4000a

ENABLE_REFLECTION(gr::picoscope4000a::Picoscope4000a, values0, errors0);

#endif
