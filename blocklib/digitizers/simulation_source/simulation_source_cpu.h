#pragma once

#include <gnuradio/digitizers/simulation_source.h>

#include "digitizer_block_impl.h"
#include "range.h"

#include <functional>
#include <future>
#include <system_error>

namespace gr::digitizers {

struct simulation_driver {
    digitizer_acquisition_mode_t         d_acquisition_mode;
    std::size_t                          d_buffer_size;
    meta_range_t                         d_ranges;
    std::vector<float>                   d_ch_a_data;
    std::vector<float>                   d_ch_b_data;
    std::vector<uint8_t>                 d_port_data;
    std::function<void(std::error_code)> d_notify_data_ready_cb;
    app_buffer_t                        *d_app_buffer = nullptr;

    simulation_driver(digitizer_acquisition_mode_t acquisition_mode, std::size_t buffer_size)
        : d_acquisition_mode(acquisition_mode)
        , d_buffer_size(buffer_size) {
        d_ranges.push_back(range_t(20));
    }

    std::error_code initialize() {
        return {};
    }

    std::error_code configure() {
        return {};
    }

    std::error_code arm() {
        if (d_acquisition_mode == digitizer_acquisition_mode_t::RAPID_BLOCK) {
            // rapid block data gets available after one second
            std::async(std::launch::async, [this]() {
                boost::this_thread::sleep_for(boost::chrono::seconds{ 1 });
                d_notify_data_ready_cb(std::error_code{});
            });
        }

        return std::error_code{};
    }

    std::error_code disarm() {
        return {};
    }

    std::error_code close() {
        return {};
    }

    std::error_code poll() {
        for (size_t i = 0; i < 1000; i++) {
            if (fill_in_data_chunk() == false)
                break;
        }

        return std::error_code{};
    }

    std::error_code prefetch_block(size_t length, size_t block_number) {
        return {};
    }

    std::error_code get_rapid_block_data(size_t offset, size_t length, size_t waveform, work_io &wio, std::vector<uint32_t> &status) {
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
        auto buffer = d_app_buffer->get_free_data_chunk();
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

        d_app_buffer->add_full_data_chunk(buffer);

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
    digitizer_block_impl<simulation_driver> d_impl;
};

} // namespace gr::digitizers
