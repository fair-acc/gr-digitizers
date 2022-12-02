#pragma once

#include <gnuradio/picoscope4000a/picoscope4000a.h>

#include "picoscope_impl.h"
#include "utils.h"

#include <ps4000aApi.h>

#include <system_error>

namespace gr::picoscope4000a {

class picoscope_4000a_impl : public digitizers::picoscope_impl
{
private:
    int16_t d_handle;   // picoscope handle
    int16_t d_overflow; // status returned from getValues

public:
    picoscope_4000a_impl(const digitizers::digitizer_args& args,
                         std::string serial_number,
                         logger_ptr logger);

    ~picoscope_4000a_impl();

    std::string get_driver_version() const override;

    std::string get_hardware_version() const override;

    std::error_code driver_initialize() override;

    std::error_code driver_configure() override;

    std::error_code driver_arm() override;

    std::error_code driver_disarm() override;

    std::error_code driver_close() override;

    std::error_code driver_prefetch_block(size_t length, size_t block_number) override;

    std::error_code driver_get_rapid_block_data(size_t offset,
                                                size_t length,
                                                size_t waveform,
                                                work_io& wio,
                                                std::vector<uint32_t>& status) override;

    std::error_code driver_poll() override;

    void rapid_block_callback(int16_t handle, PICO_STATUS status);

private:
    std::string get_unit_info_topic(PICO_INFO info) const;

    std::error_code set_buffers(size_t samples, uint32_t block_number);

    uint32_t convert_frequency_to_ps4000a_timebase(double desired_freq,
                                                   double& actual_freq);
};

class picoscope4000a_cpu : public virtual picoscope4000a
{
public:
    explicit picoscope4000a_cpu(block_args args);

    bool start() override;

    bool stop() override;

    work_return_t work(work_io& wio) override;

    std::string hardware_version() const override
    {
        return d_impl.get_hardware_version();
    }

    std::string driver_version() const override { return d_impl.get_driver_version(); }

    double actual_sample_rate() const override { return d_impl.get_samp_rate(); }

    void initialize() override;

    void close() override;

    void set_aichan(std::string id,
                    bool enabled,
                    double range,
                    digitizers::coupling_t coupling,
                    double range_offset = 0) override
    {
        d_impl.set_aichan(id, enabled, range, coupling, range_offset);
    }

    void set_diport(std::string id, bool enabled, double thresh_voltage)
    {
        d_impl.set_diport(id, enabled, thresh_voltage);
    }

    void set_aichan_trigger(std::string id,
                            digitizers::trigger_direction_t trigger_direction,
                            double threshold)
    {
        d_impl.set_aichan_trigger(id, trigger_direction, threshold);
    }

private:
    picoscope_4000a_impl d_impl;
};

} // namespace gr::picoscope4000a
