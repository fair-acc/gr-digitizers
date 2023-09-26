#ifndef GR_PICOSCOPE4000A_PICOSCOPE4000A_H
#define GR_PICOSCOPE4000A_PICOSCOPE4000A_H

#include <picoscope.h>

namespace gr::picoscope4000a {

using fair::graph::PublishableSpan;

struct Picoscope4000a : public gr::picoscope::Picoscope<Picoscope4000a> {
    using AnalogPort = fair::graph::PortOut<float>;

    AnalogPort values0;
    AnalogPort errors0;
    AnalogPort values1;
    AnalogPort errors1;
    AnalogPort values2;
    AnalogPort errors2;
    AnalogPort values3;
    AnalogPort errors3;
    AnalogPort values4;
    AnalogPort errors4;
    AnalogPort values5;
    AnalogPort errors5;
    AnalogPort values6;
    AnalogPort errors6;
    AnalogPort values7;
    AnalogPort errors7;

    template <PublishableSpan AnalogSpan>
    fair::graph::work_return_status_t process_bulk(AnalogSpan& v0,
                                                   AnalogSpan& e0,
                                                   AnalogSpan& v1,
                                                   AnalogSpan& e1,
                                                   AnalogSpan& v2,
                                                   AnalogSpan& e2,
                                                   AnalogSpan& v3,
                                                   AnalogSpan& e3,
                                                   AnalogSpan& v4,
                                                   AnalogSpan& e4,
                                                   AnalogSpan& v5,
                                                   AnalogSpan& e5,
                                                   AnalogSpan& v6,
                                                   AnalogSpan& e6,
                                                   AnalogSpan& v7,
                                                   AnalogSpan& e7) noexcept
    {
        return this->process_bulk_impl<AnalogSpan, 8>({ { { v0, e0 },
                                                          { v1, e1 },
                                                          { v2, e2 },
                                                          { v3, e3 },
                                                          { v4, e4 },
                                                          { v5, e5 },
                                                          { v6, e6 },
                                                          { v7, e7 } } });
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
                  errors7,
                  serial_number,
                  sample_rate,
                  pre_samples,
                  post_samples,
                  acquisition_mode_string,
                  rapid_block_nr_captures,
                  streaming_mode_poll_rate,
                  auto_arm,
                  trigger_once);

#endif
