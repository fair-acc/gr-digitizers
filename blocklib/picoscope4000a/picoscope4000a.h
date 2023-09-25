#ifndef GR_PICOSCOPE4000A_PICOSCOPE4000A_H
#define GR_PICOSCOPE4000A_PICOSCOPE4000A_H

#include <picoscope.h>

namespace gr::picoscope4000a {

struct Picoscope4000a : public gr::picoscope::Picoscope<Picoscope4000a> {
    fair::graph::OUT<float> values0;
    fair::graph::OUT<float> errors0;
    fair::graph::OUT<float> values1;
    fair::graph::OUT<float> errors1;
    fair::graph::OUT<float> values2;
    fair::graph::OUT<float> errors2;
    fair::graph::OUT<float> values3;
    fair::graph::OUT<float> errors3;
    fair::graph::OUT<float> values4;
    fair::graph::OUT<float> errors4;
    fair::graph::OUT<float> values5;
    fair::graph::OUT<float> errors5;
    fair::graph::OUT<float> values6;
    fair::graph::OUT<float> errors6;
    fair::graph::OUT<float> values7;
    fair::graph::OUT<float> errors7;

    explicit Picoscope4000a(gr::picoscope::Settings settings = {})
        : gr::picoscope::Picoscope<Picoscope4000a>(std::move(settings))
    {
    }

    fair::graph::work_return_status_t process_bulk(std::span<float> v0,
                                                   std::span<float> e0,
                                                   std::span<float> v1,
                                                   std::span<float> e1,
                                                   std::span<float> v2,
                                                   std::span<float> e2,
                                                   std::span<float> v3,
                                                   std::span<float> e3,
                                                   std::span<float> v4,
                                                   std::span<float> e4,
                                                   std::span<float> v5,
                                                   std::span<float> e5,
                                                   std::span<float> v6,
                                                   std::span<float> e6,
                                                   std::span<float> v7,
                                                   std::span<float> e7) noexcept
    {
        return this->process_bulk_impl<8>({ { { v0, e0 },
                                              { v1, e1 },
                                              { v2, e2 },
                                              { v3, e3 },
                                              { v4, e4 },
                                              { v5, e5 },
                                              { v6, e6 },
                                              { v7, e7 } } });
    }

    std::make_signed_t<std::size_t> available_samples(const Picoscope&) const noexcept
    {
        return this->available_samples_impl();
    }

    std::error_code set_buffers(size_t samples, uint32_t block_number);

    static constexpr float DRIVER_VERTICAL_PRECISION = 0.01f;

    static std::optional<std::size_t> driver_channel_id_to_index(std::string_view id);
    std::string driver_driver_version() const;
    std::string driver_hardware_version() const;
    gr::picoscope::GetValuesResult driver_rapid_block_get_values(std::size_t capture,
                                                                 std::size_t nr_samples);
    std::error_code driver_initialize();
    std::error_code driver_close();
    std::error_code driver_configure();
    std::error_code driver_arm();
    std::error_code driver_disarm() noexcept;
    std::error_code driver_poll();
};

} // namespace gr::picoscope4000a

ENABLE_REFLECTION(gr::picoscope4000a::Picoscope4000a,
                  values0,
                  errors0,
                  values1,
                  errors1,
                  values2,
                  errors2,
                  values3,
                  errors3,
                  values4,
                  errors4,
                  values5,
                  errors5,
                  values6,
                  errors6,
                  values7,
                  errors7);

#endif
