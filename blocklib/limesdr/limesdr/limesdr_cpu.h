#pragma once

#include <gnuradio/limesdr/limesdr.h>

#include "digitizer_block_impl.h"
#include "utils.h"

#include <system_error>
#include <memory>

#include <lime/LimeSuite.h>

namespace gr::limesdr {

class limesdr_impl : public digitizers::digitizer_block_impl
{
private:
    struct device {
        lms_device_t* handle = nullptr;
        std::string hardware_version;

        explicit device(lms_device_t* h) : handle(h) {}

        ~device()
        {
            if (handle) {
                LMS_Close(handle);
            }
        }
    };

    std::unique_ptr<device> d_device;
    std::string d_serial_number;

public:
    limesdr_impl(const digitizers::digitizer_args& args,
                 std::string serial_number,
                 logger_ptr logger);

    ~limesdr_impl();

    std::vector<std::string> get_aichan_ids() override;

    gr::digitizers::meta_range_t get_aichan_ranges() override;

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
};

class limesdr_cpu : public virtual limesdr
{
public:
    explicit limesdr_cpu(block_args args);

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

    void set_diport(std::string id, bool enabled, double thresh_voltage) override
    {
        d_impl.set_diport(id, enabled, thresh_voltage);
    }

    void set_aichan_trigger(std::string id,
                            digitizers::trigger_direction_t trigger_direction,
                            double threshold) override
    {
        d_impl.set_aichan_trigger(id, trigger_direction, threshold);
    }

    void set_di_trigger(uint8_t pin,
                        digitizers::trigger_direction_t trigger_direction) override
    {
        d_impl.set_di_trigger(pin, trigger_direction);
    }

private:
    limesdr_impl d_impl;
};

} // namespace gr::limesdr
