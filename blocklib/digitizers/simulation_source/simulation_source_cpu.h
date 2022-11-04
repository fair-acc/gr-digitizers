#pragma once

#include <gnuradio/digitizers/simulation_source.h>

#include "digitizer_block_impl.h"
#include "range.h"

#include <future>
#include <system_error>

namespace gr::digitizers {

class simulation_impl : public digitizer_block_impl {
public:
    meta_range_t                         d_ranges;
    std::vector<float>                   d_ch_a_data;
    std::vector<float>                   d_ch_b_data;
    std::vector<uint8_t>                 d_port_data;

    simulation_impl(const digitizer_args &args, logger_ptr logger)
        : digitizer_block_impl(args, logger) {
        d_ranges.push_back(range_t(20));
    }

    std::string get_driver_version() const override {
        return "simulation";
    }

    std::string get_hardware_version() const override {
        return "simulation";
    }

    std::vector<std::string> get_aichan_ids() override {
        return std::vector<std::string>{ "A", "B" };
    }

    meta_range_t get_aichan_ranges() override {
        return d_ranges;
    }

    std::error_code driver_initialize() override {
        return {};
    }

    std::error_code driver_configure() override {
        return {};
    }

    std::error_code driver_arm() override {
        if (d_acquisition_mode == acquisition_mode_t::RAPID_BLOCK) {
            // rapid block data gets available after one second
            std::async(std::launch::async, [this]() {
                boost::this_thread::sleep_for(boost::chrono::seconds{ 1 });
                notify_data_ready(std::error_code{});
            });
        }

        return std::error_code{};
    }

    std::error_code driver_disarm() override {
        return {};
    }

    std::error_code driver_close() override {
        return {};
    }

    std::error_code driver_poll() override {
        for (size_t i = 0; i < 1000; i++) {
            if (fill_in_data_chunk() == false)
                break;
        }

        return std::error_code{};
    }

    std::error_code driver_prefetch_block(size_t length, size_t block_number) override {
        return {};
    }

    std::error_code driver_get_rapid_block_data(size_t offset, size_t length, size_t waveform, work_io &wio, std::vector<uint32_t> &status) override {
        if (offset + length > d_ch_a_data.size()) {
            std::ostringstream message;
            message << "Exception in " << __FILE__ << ":" << __LINE__ << ": cannot fetch rapid block data, out of bounds";
            throw std::runtime_error(message.str());
        }

        if (wio.outputs().size() != 5) {
            std::ostringstream message;
            message << "Exception in " << __FILE__ << ":" << __LINE__ << ": all channels should be passed in";
            throw std::runtime_error(message.str());
        }

        // Update status first
        for (auto &s : status) {
            s = 0;
        };

        // Copy over the data
        float   *val_a = wio.outputs()[0].items<float>();
        float   *err_a = wio.outputs()[1].items<float>();
        float   *val_b = wio.outputs()[2].items<float>();
        float   *err_b = wio.outputs()[3].items<float>();
        uint8_t *port  = wio.outputs()[4].items<uint8_t>();

        for (size_t i = 0; i < length; i++) {
            val_a[i] = d_ch_a_data[offset + i];
            err_a[i] = 0.005;

            val_b[i] = d_ch_b_data[offset + i];
            err_b[i] = 0.005;

            port[i]  = d_port_data[offset + i];
        }

        return std::error_code{};
    }

    bool fill_in_data_chunk() {
        auto buffer_size_channel_bytes = (d_buffer_size * sizeof(float));

        auto buffer_size_bytes         = (buffer_size_channel_bytes * 2 * 2) // 2 channels, floats, errors & values
                               + (d_buffer_size);

        // resize and clear tmp buffer (errors are all zero)
        auto buffer = d_app_buffer.get_free_data_chunk();
        if (buffer == nullptr) {
            return false;
        }

        assert(buffer->d_data.size() == buffer_size_bytes);

        // just in case
        d_ch_a_data.resize(d_buffer_size);
        d_ch_b_data.resize(d_buffer_size);
        d_port_data.resize(d_buffer_size);

        // fill up tmp buffer
        memcpy(&buffer->d_data[buffer_size_channel_bytes * 0], &d_ch_a_data[0], buffer_size_channel_bytes);
        memcpy(&buffer->d_data[buffer_size_channel_bytes * 2], &d_ch_b_data[0], buffer_size_channel_bytes);
        memcpy(&buffer->d_data[buffer_size_channel_bytes * 4], &d_port_data[0], d_buffer_size);

        // error band
        float *err_a = reinterpret_cast<float *>(&buffer->d_data[buffer_size_channel_bytes * 1]);
        float *err_b = reinterpret_cast<float *>(&buffer->d_data[buffer_size_channel_bytes * 3]);
        for (uint32_t i = 0; i < d_buffer_size; i++) {
            err_a[i] = 0.005;
            err_b[i] = 0.005;
        }

        buffer->d_local_timestamp = get_timestamp_nano_utc();
        buffer->d_status          = std::vector<uint32_t>{ 0, 0 };

        d_app_buffer.add_full_data_chunk(buffer);

        return true;
    }
};

class simulation_source_cpu : public virtual simulation_source {
public:
    explicit simulation_source_cpu(block_args args);

    bool          start() override;

    bool          stop() override;

    void          set_data(std::vector<float> channel_a_data, std::vector<float> channel_b_data, std::vector<uint8_t> port_data) override;

    work_return_t work(work_io &wio) override;

private:
    simulation_impl d_impl;
};

} // namespace gr::digitizers
