#pragma once

#include <digitizers/tags.h>

#include <gnuradio/circular_buffer.hpp>
#include <gnuradio/limesdr/limesdr.h>

#include <chrono>
#include <deque>
#include <limits>
#include <memory>
#include <optional>

#include <lime/LimeSuite.h>

namespace gr::limesdr {

namespace detail {

constexpr std::optional<std::string_view> parse_serial(std::string_view s)
{
    constexpr std::string_view serial_prefix = "serial=";
    const auto prefix_pos = s.find(serial_prefix);
    if (prefix_pos == std::string_view::npos) {
        return std::nullopt;
    }

    s.remove_prefix(prefix_pos + serial_prefix.size());
    const auto comma_pos = s.find(",");
    if (comma_pos == std::string_view::npos) { // serial is last entry
        return s;
    }

    s.remove_suffix(s.size() - comma_pos - 1);
    return s;
}

template <typename T>
inline constexpr std::chrono::nanoseconds convert_to_ns(T ns)
{
    using namespace std::chrono;
    return round<nanoseconds>(duration<T, std::nano>{ ns });
}

} // namespace detail

class limesdr_cpu : public virtual limesdr
{
private:
    struct stream {
        lms_device_t* device_handle = nullptr;
        lms_stream_t handle;
        std::vector<uint16_t> read_buffer;
        std::size_t samples_read = 0; // note that a sample consists of *two* uint16_t
        uint16_t trigger_mask = 0;

        explicit stream(lms_stream_t s,
                        lms_device_t* device,
                        uint16_t trigger_mask,
                        std::size_t buffer_size)
            : device_handle(device), handle{ std::move(s) }, read_buffer(buffer_size * 2)
        {
        }

        ~stream() { LMS_DestroyStream(device_handle, &handle); }
    };

    struct device {
        lms_device_t* handle = nullptr;
        std::string serial_number;
        std::string hardware_version;
        std::vector<std::unique_ptr<stream>> streams;

        explicit device(lms_device_t* h, std::string sn)
            : handle(h), serial_number{ std::move(sn) }
        {
        }

        ~device()
        {
            if (handle) {
                streams.clear();
                LMS_Close(handle);
            }
        }

        lms_stream_t* stream_handle(std::size_t index) const
        {
            return streams[index] ? &streams[index]->handle : nullptr;
        }
    };

    struct timing_message_t {
        std::string name;
        std::chrono::nanoseconds timestamp; ///< timestamp from the timing receiver
        std::chrono::nanoseconds
            offset; ///< timing hardware offset condition for the timestamp in seconds
                    ///< (e.g.compensating analog group delays, default '0')
    };

    std::deque<timing_message_t> d_timing_messages;
    bool d_last_sample_had_trigger_set = false;
    std::vector<std::size_t> d_pending_triggers;
    std::unique_ptr<device> d_device;
    std::string d_serial_number;
    bool d_configured = false;

public:
    explicit limesdr_cpu(block_args args);

    bool start() override
    {
        try {
            initialize_device();
            configure_streams();
            start_streaming();
            d_configured = true;
        } catch (const std::exception& e) {
            d_logger->error("{}", e.what());
            return false;
        }

        return true;
    }

    bool stop() override
    {
        try {
            stop_streaming();
        } catch (const std::exception& e) {
            d_logger->error("{}", e.what());
        }

        d_device.reset();
        d_timing_messages.clear();
        d_configured = false;

        return true;
    }

    work_return_t work(work_io& wio) override
    {
        if (!d_configured) {
            // workaround: start() returning false is ignored by GR, abort processing here
            // instead
            d_logger->error("Device initialization failed, aborting");
            return work_return_t::DONE;
        }

        auto noutput_items = wio.outputs()[0].n_items;
        std::size_t produced = 0;

        if (has_timing_triggers()) {
            if (d_timing_messages.size() < d_pending_triggers.size()) {
                // we have timing triggers without timing message, wait for message
                return work_return_t::OK;
            }

            const auto result = flush_samples(wio, 0, d_pending_triggers);
            if (!result) {
                return work_return_t::INSUFFICIENT_OUTPUT_ITEMS;
            }

            d_pending_triggers.clear();
            produced += *result;
            noutput_items -= *result;
        }

        auto available = std::min(noutput_items,
                                  std::get<std::size_t>(*this->param_driver_buffer_size));

        for (std::size_t i = 0; i < d_device->streams.size(); ++i) {
            auto& stream = d_device->streams[i];
            if (!stream) {
                continue;
            }

            lms_stream_status_t status;
            const auto status_rc = LMS_GetStreamStatus(&stream->handle, &status);
            if (status_rc != LMS_SUCCESS) {
                d_logger->error("Could not get stream status for {}", i);
                return work_return_t::ERROR;
            }

            available = std::min(available,
                                 stream->samples_read +
                                     static_cast<std::size_t>(status.fifoFilledCount));
        }

        if (available == 0) {
            wio.produce_each(produced);
            return produced == 0 ? work_return_t::ERROR : work_return_t::OK;
        }

        for (std::size_t i = 0; i < d_device->streams.size(); ++i) {
            auto& stream = d_device->streams[i];
            if (!stream) {
                continue;
            }

            while (stream->samples_read < available) {
                const auto received =
                    LMS_RecvStream(&stream->handle,
                                   stream->read_buffer.data() + stream->samples_read * 2,
                                   available - stream->samples_read,
                                   nullptr,
                                   0);
                assert(received >= 0);
                stream->samples_read += received;
            }
        }

        std::vector<std::size_t> triggers;
        if (has_timing_triggers()) {
            triggers = find_triggers();
            // we have more triggers than messages, wait for messages
            if (triggers.size() > d_timing_messages.size()) {
                d_pending_triggers = triggers;
                return work_return_t::OK;
            }
        }

        const auto result = flush_samples(wio, produced, triggers);
        assert(result && *result == available); // must succeed
        produced += *result;
        wio.produce_each(produced);
        return work_return_t::OK;
    }

    void handle_msg_timing(pmtv::pmt msg) override
    {
        const auto map = std::get<std::map<std::string, pmtv::pmt>>(msg);

        try {
            if (!has_timing_triggers()) {
                d_timing_messages.clear();
            }

            d_timing_messages.push_back(
                { .name = std::get<std::string>(map.at(tag::TRIGGER_NAME)),
                  .timestamp =
                      detail::convert_to_ns(std::get<int64_t>(map.at(tag::TRIGGER_TIME))),
                  .offset = detail::convert_to_ns(
                      std::get<double>(map.at(tag::TRIGGER_OFFSET)) * 1000000000) });

            const auto last = d_timing_messages.back();
            d_logger->debug(
                "Received timing message: name='{}', timestamp={}, offset={}, "
                "Already Queued={}",
                last.name,
                last.timestamp,
                last.offset,
                d_timing_messages.size() - 1);
        } catch (const std::out_of_range& e) {
            d_logger->error("Could not decode timing message: {}", e.what());
        }
    }

private:
    std::optional<std::size_t> flush_samples(
        work_io& wio, std::size_t offset, const std::vector<std::size_t>& trigger_offsets)
    {
        assert(d_device);

        const auto noutput_items = wio.outputs()[0].n_items - offset;

        std::optional<std::size_t> produced;

        for (std::size_t channel_idx = 0; channel_idx < d_device->streams.size();
             ++channel_idx) {
            auto& stream = d_device->streams[channel_idx];
            if (!stream) {
                continue;
            }

            if (noutput_items < stream->samples_read) {
                return std::nullopt;
            }

            auto out = wio.outputs()[channel_idx * 2].items<uint16_t>();
            assert(!produced || *produced == stream->samples_read);
            produced = stream->samples_read;
            memcpy(out + offset * 2,
                   stream->read_buffer.data(),
                   *produced * 2 * sizeof(uint16_t));
            // Right-shift by 4 to get the analog 12-bit values and drop the digital 4 bit
            // (LSBs)
            for (std::size_t i = 0; i < *produced * 2; ++i) {
                out[offset * 2 + i] >>= 4;
            }
            stream->samples_read = 0;
        }

        if (!produced) {
            return 0;
        }

        using gr::digitizers::make_trigger_tag;

        const auto tag_offset0 = wio.outputs()[0].nitems_written() + offset;
        std::vector<tag_t> trigger_tags;

        if (has_timing_triggers()) {
            // pair timing messages with trigger offsets and create tags
            trigger_tags.reserve(trigger_offsets.size());
            for (const auto& trigger_offset : trigger_offsets) {
                const auto timing = d_timing_messages.front();
                d_timing_messages.pop_front();
                trigger_tags.push_back(make_trigger_tag(tag_offset0 + trigger_offset,
                                                        timing.name,
                                                        timing.timestamp,
                                                        timing.offset));
            }
        }
        else {
            // no trigger channels, use the last timing message to tag the first sample
            if (!d_timing_messages.empty()) {
                const auto timing = d_timing_messages[0];
                trigger_tags.push_back(make_trigger_tag(
                    tag_offset0, timing.name, timing.timestamp, timing.offset));
            }
        }

        for (std::size_t channel_idx = 0; channel_idx < d_device->streams.size();
             ++channel_idx) {
            if (d_device->streams[channel_idx]) {
                for (auto& trigger_tag : trigger_tags) {
                    wio.outputs()[channel_idx * 2].add_tag(trigger_tag);
                }
            }
        }

        return *produced;
    }

    bool has_timing_triggers() const
    {
        auto has_trigger = [](const auto& stream) { return stream->trigger_mask != 0; };

        return d_device && std::any_of(d_device->streams.begin(),
                                       d_device->streams.end(),
                                       has_trigger);
    }

    std::vector<std::size_t> find_triggers()
    {
        // TODO this currently assumes that only one channel has a mask != 0. If we need
        // more complex setups, implement that, otherwise simplify the API to reflect this
        if (!d_device) {
            return {};
        }

        for (const auto& stream : d_device->streams) {
            if (stream->trigger_mask == 0) {
                continue;
            }

            std::vector<std::size_t> triggers;
            for (std::size_t sample = 0; sample < stream->samples_read; ++sample) {
                const auto has_trigger_set =
                    (stream->read_buffer[sample * 2] & stream->trigger_mask) ==
                    stream->trigger_mask;
                if (has_trigger_set) {
                    if (!d_last_sample_had_trigger_set) {
                        triggers.push_back(sample);
                    }
                }

                d_last_sample_had_trigger_set = has_trigger_set;
            }

            return triggers;
        }

        return {};
    }

    void initialize_device()
    {
        d_logger->info("LimeSuite version: {}", LMS_GetLibraryVersion());

        std::array<lms_info_str_t, 20>
            list; // TODO size not passed, what happens if there are 21 devices?

        const auto device_count = LMS_GetDeviceList(list.data());
        if (device_count < 1) {
            throw std::runtime_error("No Lime devices found");
        }

        d_logger->info("Device list:");
        for (int i = 0; i < device_count; i++) {
            d_logger->info("Nr.: {} device: {}", i, list[i]);
        }

        std::size_t device_index = 0;
        std::string serial_number = std::get<std::string>(*this->param_serial_number);

        if (serial_number.empty()) {
            const auto parsed = detail::parse_serial(list[0]);
            if (!parsed) {
                throw std::runtime_error(fmt::format(
                    "No serial number given, could not parse serial number from '{}'",
                    list[0]));
            }
            serial_number = std::string(*parsed);
            d_logger->info("No serial number given, using first device found: '{}'",
                           serial_number);
        }
        else {
            auto serial_matches = [&serial_number](lms_info_str_t s) {
                const auto parsed = detail::parse_serial(s);
                return parsed && *parsed == serial_number;
            };

            const auto it = std::find_if(list.begin(), list.end(), serial_matches);
            if (it == list.end()) {
                throw std::runtime_error(fmt::format(
                    "Device with serial number '{}' not found", serial_number));
            }

            device_index = std::distance(list.begin(), it);
        }

        lms_device_t* address;
        const auto open_rc = LMS_Open(&address, list[device_index], nullptr);
        if (open_rc != LMS_SUCCESS) {
            throw std::runtime_error(
                fmt::format("Could not open device '{}'", serial_number));
        }

        auto dev = std::make_unique<device>(address, serial_number);
        const auto init_rc = LMS_Init(dev->handle);
        if (init_rc != LMS_SUCCESS) {
            throw std::runtime_error(
                fmt::format("Could not initialize device '{}'", serial_number));
        }

        const auto dev_info = LMS_GetDeviceInfo(dev->handle);
        assert(dev_info);
        dev->hardware_version = fmt::format("Hardware: {} Firmware: {}",
                                            dev_info->hardwareVersion,
                                            dev_info->firmwareVersion);

        d_logger->info("Versions: {}", dev->hardware_version);

        d_device = std::move(dev);
    }

    void configure_streams()
    {
        const auto num_device_channels = LMS_GetNumChannels(d_device->handle, LMS_CH_RX);
        if (num_device_channels < 0) {
            throw std::runtime_error(
                fmt::format("Could not retrieve number of channels for '{}'",
                            d_device->serial_number));
        }
        d_logger->info("Number of channels: {}", num_device_channels);

        lms_range_t sr_range;
        const auto sample_range_rc =
            LMS_GetSampleRateRange(d_device->handle, LMS_CH_RX, &sr_range);
        if (sample_range_rc == LMS_SUCCESS) {
            d_logger->info("Sample rate range: Min: {} Max: {}, Step: {}",
                           sr_range.min,
                           sr_range.max,
                           sr_range.step);
        }

        const auto sample_rate = std::get<double>(*this->param_sample_rate);
        const auto set_samp_rc = LMS_SetSampleRate(d_device->handle, sample_rate, 0);
        if (set_samp_rc != LMS_SUCCESS) {
            throw std::runtime_error(
                fmt::format("Could not set sample rate ({})", sample_rate));
        }

        d_logger->info("Set sample rate {}", sample_rate);

        static constexpr int MAX_CHANNELS = 2;
        if (num_device_channels > MAX_CHANNELS) {
            d_logger->warn("Device has more channels than block allows! ({} vs {})",
                           num_device_channels,
                           MAX_CHANNELS);
        }

        const auto n_channels =
            static_cast<std::size_t>(std::min(num_device_channels, MAX_CHANNELS));
        d_device->streams.resize(n_channels);

        const auto enabled_channels =
            pmtv::get_vector<std::size_t>(*this->param_enabled_channels);
        const auto trigger_mask = std::get<int>(*this->param_timing_trigger_mask);

        for (std::size_t channel_idx = 0; channel_idx < n_channels; channel_idx++) {
            const auto enable = std::find(enabled_channels.begin(),
                                          enabled_channels.end(),
                                          channel_idx) != enabled_channels.end();

            const auto enable_channel_rc =
                LMS_EnableChannel(d_device->handle, LMS_CH_RX, channel_idx, enable);
            if (enable_channel_rc != LMS_SUCCESS) {
                throw std::runtime_error(
                    fmt::format("Could not enable/disable channel {} (enable: {})",
                                channel_idx,
                                enable));
            }

            d_logger->info("Set channel {} (enabled={})", channel_idx, enable);

            if (!enable) {
                continue;
            }

            const auto driver_buffer_size =
                std::get<std::size_t>(*this->param_driver_buffer_size);

            lms_stream_t stream_cfg{ .isTx = LMS_CH_RX,
                                     .channel = static_cast<uint32_t>(channel_idx),
                                     .fifoSize =
                                         static_cast<uint32_t>(driver_buffer_size),
                                     .throughputVsLatency =
                                         0.5, // TODO what is wanted here? 0 is lowest
                                              // latency, 1 highest throughput
                                     .dataFmt = lms_stream_t::LMS_FMT_I16 };

            const auto setup_rc = LMS_SetupStream(d_device->handle, &stream_cfg);
            if (setup_rc != LMS_SUCCESS) {
                throw std::runtime_error(
                    fmt::format("Could not set up stream for channel {}", channel_idx));
            }

            const auto mask = (trigger_mask >> (channel_idx * 4)) & 0xf;
            d_logger->debug(
                "Timing trigger mask for channel {}: {:4b}", channel_idx, mask);

            d_device->streams[channel_idx] = std::make_unique<stream>(
                stream_cfg, d_device->handle, mask, driver_buffer_size);
        }
    }

    void start_streaming()
    {
        for (std::size_t i = 0; i < d_device->streams.size(); ++i) {
            const auto handle = d_device->stream_handle(i);
            if (!handle) {
                continue;
            }
            const auto start_rc = LMS_StartStream(handle);
            if (start_rc != LMS_SUCCESS) {
                throw std::runtime_error(fmt::format("Could not start stream {}", i));
            }
        }
    }

    void stop_streaming()
    {
        if (!d_device) {
            return;
        }

        std::vector<std::size_t> unstoppables;

        for (std::size_t i = 0; i < d_device->streams.size(); ++i) {
            const auto handle = d_device->stream_handle(i);
            if (!handle) {
                continue;
            }
            const auto start_rc = LMS_StopStream(handle);
            if (start_rc != LMS_SUCCESS) {
                unstoppables.push_back(i);
            }
        }

        if (!unstoppables.empty()) {
            throw std::runtime_error(fmt::format("Could not stop streams: ({})",
                                                 fmt::join(unstoppables, ", ")));
        }
    }
};

} // namespace gr::limesdr
