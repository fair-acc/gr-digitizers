#ifndef FAIR_PICOSCOPE_PICOSCOPE_HPP
#define FAIR_PICOSCOPE_PICOSCOPE_HPP

#include "PicoscopeAPI.hpp"

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/algorithm/dataset/DataSetUtils.hpp>
#include <gnuradio-4.0/HistoryBuffer.hpp>

#include <format>

#include <chrono>
#include <functional>
#include <queue>
#include <string_view>

namespace fair::picoscope {
using namespace std::literals;

namespace detail {

constexpr std::size_t kDriverBufferSize = 65536;

[[nodiscard]] static inline bool isDigitalTrigger(std::string_view source) { return !source.empty() && source.starts_with("DI"); }
[[nodiscard]] static inline bool isAnalogTrigger(std::string_view source) { return !source.empty() && !source.starts_with("DI"); }

[[nodiscard]] static std::expected<int, gr::Error> parseDigitalTriggerSource(std::string_view triggerSrc) {
    if (!triggerSrc.starts_with("DI")) {
        return std::unexpected(gr::Error(std::format("Cannot parse digital trigger source (`{}`): it must start with `DI`.", triggerSrc)));
    }
    std::string_view numberPart = triggerSrc.substr(2);
    if (numberPart.empty()) {
        return std::unexpected(gr::Error(std::format("Cannot parse digital trigger source (`{}`): No digital channel number.", triggerSrc)));
    }
    int value{-1};
    auto [_, ec] = std::from_chars(numberPart.data(), numberPart.data() + numberPart.size(), value);
    if (ec != std::errc()) {
        return std::unexpected(gr::Error(std::format("Cannot parse digital trigger source (`{}`): Error parsing digital channel number.", triggerSrc)));
    }

    if (value < 0 || value > 15) {
        return std::unexpected(gr::Error(std::format("Cannot parse digital trigger source (`{}`): channel number is out of range [0, 15].", triggerSrc)));
    }

    return value;
}

struct Channel {
    std::string id;
    bool        signalInfoTagPublished = false;

    std::vector<int16_t>       driverBuffer;
    gr::HistoryBuffer<int16_t> unpublished{0}; // Size will be adjusted to fit n samples

    // settings
    std::string name;
    float       sampleRate   = 1.0f;
    std::string unit         = "V";
    std::string quantity     = "Voltage";
    float       range        = 2.f;
    float       analogOffset = 0.f;
    float       scale        = 1.f;
    float       offset       = 0.f;
    Coupling    coupling     = Coupling::DC;

    [[nodiscard]] gr::property_map toTagMap() const {
        return {
            { std::string(gr::tag::SIGNAL_NAME.shortKey()), name },
            { std::string(gr::tag::SAMPLE_RATE.shortKey()), sampleRate },
            { std::string(gr::tag::SIGNAL_QUANTITY.shortKey()), quantity },
            { std::string(gr::tag::SIGNAL_UNIT.shortKey()), unit },
            { std::string(gr::tag::SIGNAL_MIN.shortKey()), offset - range },
            { std::string(gr::tag::SIGNAL_MAX.shortKey()), offset + range },
        };
    }
};

struct TriggerNameAndCtx {
    std::string triggerName{};
    std::string ctx{};
    bool        isCtxSet{true}; // This is needed to differentiate whether an empty ("") context is set.
};

[[nodiscard]] static inline TriggerNameAndCtx createTriggerNameAndCtx(const std::string& triggerNameAndCtx) {
    if (triggerNameAndCtx.empty()) {
        return {};
    }
    const std::size_t pos = triggerNameAndCtx.find('/');
    if (pos != std::string::npos) { // trigger_name and ctx
        return {triggerNameAndCtx.substr(0, pos), (pos < triggerNameAndCtx.size() - 1) ? triggerNameAndCtx.substr(pos + 1) : "", true};
    } else { // only trigger_name
        return {triggerNameAndCtx, "", false};
    }
}

[[nodiscard]] static inline bool tagContainsTrigger(const gr::Tag& tag, const TriggerNameAndCtx& triggerNameAndCtx) {
    if (triggerNameAndCtx.isCtxSet) { // trigger_name and ctx
        if (tag.map.contains(gr::tag::TRIGGER_NAME.shortKey()) && tag.map.contains(gr::tag::CONTEXT.shortKey())) {
            const std::string tagTriggerName = std::get<std::string>(tag.map.at(gr::tag::TRIGGER_NAME.shortKey()));
            return !tagTriggerName.empty() && tagTriggerName == triggerNameAndCtx.triggerName && std::get<std::string>(tag.map.at(gr::tag::CONTEXT.shortKey())) == triggerNameAndCtx.ctx;
        }
    } else { // only trigger_name
        if (tag.map.contains(gr::tag::TRIGGER_NAME.shortKey())) {
            const std::string tagTriggerName = std::get<std::string>(tag.map.at(gr::tag::TRIGGER_NAME.shortKey()));
            return !tagTriggerName.empty() && tagTriggerName == triggerNameAndCtx.triggerName;
        }
    }
    return false;
}

template<typename TEnum>
[[nodiscard]] inline constexpr TEnum convertToEnum(std::string_view strEnum) {
    auto enumType = magic_enum::enum_cast<TEnum>(strEnum, magic_enum::case_insensitive);
    if (!enumType.has_value()) {
        throw std::invalid_argument(std::format("Unknown value. Cannot convert string '{}' to enum '{}'", strEnum, gr::meta::type_name<TEnum>()));
    }
    return enumType.value();
}
} // namespace detail

// optional shortening
template<typename T, gr::meta::fixed_string description = "", typename... Arguments>
using A = gr::Annotated<T, description, Arguments...>;

using gr::Visible;

/**
 * We allow only a small set of sample types (SampleType) to be used as the output type for the Picoscope block:
 * - `std::int16_t`: Outputs raw values as-is, without any scaling. Also used for 8-bit native ADC.
 * - `float`: Outputs the physically gain-scaled values of the measurements.
 * - `gr::UncertainValue<float>`: Similar to `float`, but also includes an estimated measurement error as an additional component.
 *
 * The output type also determines the acquisition mode:
 * - For `DataSet<SampleType>`, the acquisition mode is **RapidBlock**.
 * - For `SampleType`, the acquisition mode is **Streaming**.
 */
template<typename T>
concept PicoscopeOutput = std::disjunction_v<std::is_same<T, std::int16_t>, std::is_same<T, float>, std::is_same<T, gr::UncertainValue<float>>, //
    std::is_same<T, gr::DataSet<std::int16_t>>, std::is_same<T, gr::DataSet<float>>, std::is_same<T, gr::DataSet<gr::UncertainValue<float>>>>;

template<PicoscopeOutput T, PicoscopeImplementationLike TPSImpl, typename TTagMatcher = timingmatcher::TimingMatcher>
struct Picoscope : gr::Block<T, gr::SupportedTypes<int16_t, float, gr::UncertainValue<float>, gr::DataSet<int16_t>, gr::DataSet<float>, gr::DataSet<gr::UncertainValue<float>>>> {
    using SuperT = gr::Block<T, gr::SupportedTypes<int16_t, float, gr::UncertainValue<float>, gr::DataSet<int16_t>, gr::DataSet<float>, gr::DataSet<gr::UncertainValue<float>>>>;
    static constexpr AcquisitionMode acquisitionMode = gr::DataSetLike<T> ? AcquisitionMode::RapidBlock : AcquisitionMode::Streaming;
    using TSample = std::conditional_t<gr::DataSetLike<T>, gr::UncertainValueType_t<typename T::valueType>, gr::UncertainValueType_t<T>>;

    A<std::string, "serial number, empty selects first available device">            serial_number;
    A<float, "sample rate", Visible>                                                 sample_rate              = 10000.f;
    A<gr::Size_t, "pre-samples">                                                     pre_samples              = 1000;  // RapidBlock mode only
    A<gr::Size_t, "post-samples">                                                    post_samples             = 1000;  // RapidBlock mode only
    A<gr::Size_t, "no. captures (rapid block mode)">                                 n_captures               = 1;     // RapidBlock mode only
    A<bool, "trigger once (rapid block mode)">                                       trigger_once             = false; // RapidBlock mode only
    A<bool, "do arm at start?">                                                      auto_arm                 = true;
    A<std::vector<std::string>, "IDs of enabled channels: `A`, `B`, `C` etc.">       channel_ids;
    A<std::vector<float>, "Voltage range of enabled channels">                       channel_ranges;         // PS channel setting
    A<std::vector<float>, "Voltage offset of enabled channels">                      channel_analog_offsets; // PS channel setting
    A<std::vector<std::string>, "Coupling modes of enabled channels">                channel_couplings;
    A<std::vector<std::string>, "Signal names of enabled channels">                  signal_names;
    A<std::vector<std::string>, "Signal units of enabled channels">                  signal_units;
    A<std::vector<std::string>, "Signal quantity of enabled channels">               signal_quantities;
    A<std::vector<float>, "Signal scales of the enabled channels">                   signal_scales;  // only for floats and UncertainValues
    A<std::vector<float>, "Signal offset of the enabled channels">                   signal_offsets; // only for floats and UncertainValues
    A<std::string, "trigger channel (A, B, C, ... or DI1, DI2, DI3, ...)">           trigger_source;
    A<float, "trigger threshold, analog only">                                       trigger_threshold          = 0.f;
    A<std::string, "trigger direction">                                              trigger_direction          = "Rising";
    A<std::string, "trigger filter: `<trigger_name>[/<ctx>]`">                       trigger_filter             = "";
    A<std::string, "arm trigger: `<trigger_name>[/<ctx>]`, if empty not used">       trigger_arm                = ""; // RapidBlock mode only
    A<std::string, "disarm trigger: `<trigger_name>[/<ctx>]`, if empty not used">    trigger_disarm             = ""; // RapidBlock mode only
    A<gr::Size_t, "time between two systemtime tags in ms">                          systemtime_interval        = 1000UZ;
    A<int16_t, "digital port threshold (ADC: –32767 (–5 V) to 32767 (+5 V))">        digital_port_threshold     = 0;     // only used if digital ports are available: 3000a, 5000a series
    A<bool, "invert digital port output">                                            digital_port_invert_output = false; // only used if digital ports are available: 3000a, 5000a series
    A<gr::Size_t, "Timeout after which to not match trigger pulses", gr::Unit<"ns">> matcher_timeout            = 10'000'000z;

    gr::PortIn<std::uint8_t, gr::Async> timingIn;

    std::vector<gr::PortOut<T>> out{TPSImpl::N_ANALOG_CHANNELS};

    using TDigitalOutput = std::conditional<gr::DataSetLike<T>, gr::DataSet<uint16_t>, uint16_t>::type;
    gr::PortOut<TDigitalOutput> digitalOut;

    float       _actualSampleRate  = 0;
    std::size_t _nSamplesPublished = 0; // for debugging purposes

    detail::TriggerNameAndCtx _armTriggerNameAndCtx; // store parsed information to optimise performance
    detail::TriggerNameAndCtx _disarmTriggerNameAndCtx;

    int _digitalChannelNumber = -1; // used only if digital trigger is set

    GR_MAKE_REFLECTABLE(Picoscope, timingIn, out, digitalOut, serial_number, sample_rate, pre_samples, post_samples, n_captures,                                //
        auto_arm, trigger_once, channel_ids, signal_names, signal_units, signal_quantities, channel_ranges, channel_analog_offsets, signal_scales, signal_offsets, channel_couplings, //
        trigger_source, trigger_threshold, trigger_direction, digital_port_threshold, digital_port_invert_output, trigger_arm, trigger_disarm, systemtime_interval, matcher_timeout);

private:
    std::optional<TPSImpl> _picoscope;

    bool                                           _isArmed = false; // for RapidBlock mode only
    std::vector<detail::Channel>                   _channels;
    int16_t                                        _maxValue       = 0; // maximum ADC count used for ADC conversion
    std::chrono::high_resolution_clock::time_point _nextSystemtime = std::chrono::high_resolution_clock::now();

    std::array<std::vector<int16_t>, TPSImpl::N_DIGITAL_CHANNELS> _digitalBuffers;

    // TODO: use official message api instead of custom reader
    using ReaderType              = decltype(timingIn.buffer().tagBuffer.new_reader());
    ReaderType _tagReaderInternal = timingIn.buffer().tagBuffer.new_reader();

    TTagMatcher tagMatcher{.timeout = std::chrono::nanoseconds(matcher_timeout.value), .sampleRate = sample_rate.value};

public:
    ~Picoscope() { stop(); }

    explicit Picoscope(gr::property_map parameters) : SuperT(parameters) { }; // TODO: find out why this is necessary

    template<gr::OutputSpanLike TOutSpan>
    gr::work::Status processBulk(gr::InputSpanLike auto& timingInSpan, std::span<TOutSpan>& outputs, gr::OutputSpanLike auto& digitalOutSpan) {
        if (!_picoscope || !_picoscope->isOpened()) { // early return if the picoscope was not yet opened correctly or settings are not yet applied
            // TODO: implement async open and retry
            return {};
        }
        if constexpr (acquisitionMode == AcquisitionMode::Streaming) {
            using self_t = decltype(this);
            struct StreamingContext { // helper class to pass data to the callback function called by the getLatestValues function
                decltype(timingInSpan) timingInSpan;
                std::span<TOutSpan>& outputs;
                decltype(digitalOutSpan) digitalOutSpan;
                self_t self;
            } streamingContext{timingInSpan, outputs, digitalOutSpan, this};

            // fetch new values
            const auto status = _picoscope->getStreamingLatestValues(static_cast<TPSImpl::StreamingReadyType>([](int16_t /*handle*/, TPSImpl::NSamplesType noOfSamples, uint32_t startIndex, int16_t overflow, uint32_t /*triggerAt*/, int16_t /*triggered*/, int16_t /*autoStop*/, void* data) {
                if (static_cast<uint16_t>(overflow) == 0xffff) {
                    // TODO: send tag
                    std::println(std::cerr, "Buffer overrun detected, continue...");
                }
                auto streamingContext = static_cast<decltype(this)>(data);
                auto acquisitionTime = std::chrono::system_clock::now();
                auto nLeftoverToProcess = std::min(streamingContext.self->_channels[0].unpublished.size(), streamingContext.self->outputs[0].available()); // check for leftover samples
                auto nDriverSamplesProcessable = std::min(noOfSamples, streamingContext.outputs[0].available() - nLeftoverToProcess); // todo: limit by processed samples value from tag matching
                // find triggers & match
                std::vector<std::size_t> triggerEdges{};
                fair::picoscope::timingmatcher::MatcherResult matchedTags;
                if (!streamingContext.self->trigger_source->empty()) { // find triggers
                    std::span<std::int16_t> triggerSamples;
                    // match
                    std::span<const gr::Tag> tagspan = std::span(streamingContext.timingInSpan.rawTags);
                    auto tags                        = tagspan | std::views::transform([](const auto &t) { return t.map; }) | std::ranges::to<std::vector<gr::property_map>>();
                    // match tags for leftover samples
                    matchedTags                      = streamingContext.self->tagMatcher.match(tags, triggerEdges, nLeftoverToProcess, acquisitionTime - streamingContext.self->channels[0].size());
                    // match tags for new samples
                    auto tagsDriver                  = tagspan | std::views::drop(nLeftoverToProcess) | std::views::transform([](const auto &t) { return t.map; }) | std::ranges::to<std::vector<gr::property_map>>();
                    auto matchedTags2                = streamingContext.self->tagMatcher.match(tagsDriver, triggerEdges, nDriverSamplesProcessable, acquisitionTime);
                    // TODO: continue copying data
                    // consume timing tags
                }
                for (auto& [channel, output] : std::views::zip(streamingContext.self->_channels, streamingContext.outputs)) { // copy and publish all data
                    const float voltageMultiplier = channel.range / static_cast<float>(streamingContext.self->_maxValue);
                    // copy leftover data from circular buffer
                    for (std::size_t i = 0; i < nLeftoverToProcess; ++i) {
                        if constexpr (std::is_same_v<TSample, float>) {
                            output[i] = channel.offset + channel.scale * voltageMultiplier * static_cast<float>(channel.unpublishedSamples.pop_front());
                        } else if constexpr (std::is_same_v<TSample, gr::UncertainValue<float>>) {
                            output[i] = gr::UncertainValue(channel.offset + channel.scale * voltageMultiplier * static_cast<float>(channel.unpublishedSamples.pop_front()), streamingContext.self->uncertainty());
                        } else if constexpr (std::is_same_v<TSample, int16_t>) {
                            output[i] = channel.unpublishedSamples.pop_front();
                        }
                    }
                    // copy driver data to output
                    auto nDriverDataToPublish = std::min(noOfSamples, output.available() - nLeftoverToProcess, matchedTags.processedSamples); // todo: limit by processed samples value from tag matching
                    const auto driverData = std::span(channel.driverBuffer).subspan(startIndex, nDriverDataToPublish);
                    for (std::size_t i = 0; i < nDriverDataToPublish; ++i) {
                        if constexpr (std::is_same_v<TSample, float>) {
                            output[i + nLeftoverToProcess] = channel.offset + channel.scale * voltageMultiplier * static_cast<float>(driverData[i]);
                        } else if constexpr (std::is_same_v<TSample, gr::UncertainValue<float>>) {
                            output[i + nLeftoverToProcess] = gr::UncertainValue(channel.offset + channel.scale * voltageMultiplier * static_cast<float>(driverData[i]), streamingContext.self->uncertainty());
                        } else if constexpr (std::is_same_v<TSample, int16_t>) {
                            output[i + nLeftoverToProcess] = driverData[i]; // std::ranges::copy(driverData, output.begin() + static_cast<std::ptrdiff_t>(channel.unpublishedSamples.size()));
                        }
                    }
                    // copy leftover data to circular buffer
                    for (auto value : std::span(channel.driverBuffer).subspan(startIndex + nDriverDataToPublish)) {
                        channel.unpublishedSamples.push_back(value);
                    }
                }
                // publish
            }), this);
            if (status == PICO_OK) { // processed data
                return gr::work::Status::OK;
            } else if (status == PICO_BUSY || status == PICO_DRIVER_FUNCTION) { // no data to fetch yet => check and process leftover data and return
                auto acquisitionTime = 0UZ; // TODO: calculate from last index
                auto nLeftoverToProcess = std::min(_channels[0].unpublished.size(), outputs[0].available());
                if (nLeftoverToProcess == 0) return gr::work::Status::OK;
                if (!trigger_source->empty()) { // find triggers
                    std::span<std::int16_t> triggerSamples;
                    std::vector<std::size_t> triggerEdges{};
                    // match
                    std::span<const gr::Tag> tagspan = std::span(timingInSpan.rawTags);
                    auto tags                        = tagspan | std::views::transform([](const auto &t) { return t.map; }) | std::ranges::to<std::vector<gr::property_map>>();
                    auto matchedTags                 = tagMatcher.match(tags, triggerEdges, nLeftoverToProcess, acquisitionTime);
                }
                for (auto& [channel, output] : std::views::zip(_channels, outputs)) { // copy and publish all data
                    // copy samples
                    if constexpr (std::is_same_v<TSample, int16_t>) {
                        std::ranges::copy(channel.unpublishedSamples, output.begin());
                    } else {
                        const float voltageMultiplier = channel.range / static_cast<float>(_maxValue);
                        // TODO use SIMD
                        for (std::size_t i = 0; i < nLeftoverToProcess; ++i) {
                            if constexpr (std::is_same_v<TSample, float>) {
                                output[i] = channel.offset + channel.scale * voltageMultiplier * static_cast<float>(channel.unpublishedSamples[i]);
                            } else if constexpr (std::is_same_v<TSample, gr::UncertainValue<float>>) {
                                output[i] = gr::UncertainValue(channel.offset + channel.scale * voltageMultiplier * static_cast<float>(channel.unpublishedSamples[i]), _picoscope->uncertainty());
                            }
                        }
                    }
                    // publish
                }
                // {
                //     std::size_t unpublishedOffset = _channels[0].unpublishedSamples.size();
                //     std::size_t availableSamples = calculateAvailableOutputs(nSamples + unpublishedOffset, digitalOutSpan, outputs);

                //     for (std::size_t channelIdx = 0; channelIdx < _channels.size(); channelIdx++) {
                //         processSamplesOneChannel<T>(availableSamples, offset, _channels[channelIdx], outputs[channelIdx]);
                //     }

                //     auto acquisitionTimeOffset = std::chrono::nanoseconds(static_cast<long>(static_cast<float>(unpublishedOffset) * (1e9f / sample_rate)));

                //     const auto               triggerSourceIndex = _picoscope->convertToOutputIndex(trigger_source);
                //     auto triggerOffsets = [&]() -> std::vector<std::size_t> {
                //         if (triggerSourceIndex) {
                //             std::span samples(outputs[triggerSourceIndex.value()].data(), availableSamples);
                //             return findAnalogTriggers(_channels[triggerSourceIndex.value()], samples);
                //         } else {
                //             return {};
                //         }}();
                //     std::span<const gr::Tag> tagspan = std::span(timingInSpan.rawTags);
                //     auto tags = tagspan | std::views::transform([](const auto &t) { return t.map; }) | std::ranges::to<std::vector<gr::property_map>>();
                //     auto triggerTags = tagMatcher.match(tags, triggerOffsets, availableSamples, acquisitionTime - acquisitionTimeOffset);

                //     for (const auto& msg : triggerTags.messages) {
                //         this->emitErrorMessage(std::format("{}::TimingMatcher", this->name), msg);
                //     }

                //     // publish tags
                //     for (std::size_t channelIdx = 0; channelIdx < _channels.size(); ++channelIdx) {
                //         auto& channel = _channels[channelIdx];
                //         auto& output  = outputs[channelIdx];
                //         if (!channel.signalInfoTagPublished) {
                //             output.publishTag(channel.toTagMap(), 0);
                //             channel.signalInfoTagPublished = true;
                //         }
                //         if (triggerTags.tags.empty()) {
                //             continue;
                //         }
                //         for (auto& [index, map] : triggerTags.tags) {
                //             output.publishTag(map, index);
                //         }
                //     }

                //     if constexpr (TPSImpl::N_DIGITAL_CHANNELS > 0) {
                //         _picoscope->copyDigitalBuffersToOutput(digitalOutSpan, triggerTags.processedSamples, _digitalBuffers, digital_port_invert_output);
                //         for (auto &[index, map]: triggerTags.tags) {
                //             digitalOutSpan.publishTag(map, index);
                //         }
                //         digitalOutSpan.publish(triggerTags.processedSamples);
                //     }

                //     auto lastTimingSampleIndex = tagspan[triggerTags.processedTags - 1].index - timingInSpan.streamIndex + 1;
                //     timingInSpan.consumeTags(lastTimingSampleIndex);
                //     std::ignore = timingInSpan.consume(lastTimingSampleIndex);

                //     // publish samples
                //     for (std::size_t i = 0; i < outputs.size(); i++) {
                //         // TODO: The issue is that the Block class does not properly handle optional ports such that sample limits are calculated wrongly.
                //         // Therefore, we must connect all output ports of the picoscope and ensure that samples are published for all ports.
                //         // Otherwise, tags will not be propagated correctly.
                //         // const std::size_t nSamplesToPublish = i < _state.channels.size() ? availableSamples : 0UZ;
                //         outputs[i].publish(triggerTags.processedSamples);
                //     }

                //     // save the unpublished part of the chunk to be reprocessed in the next iteration
                //     for (auto & _channel : _channels) {
                //         if (triggerTags.processedSamples > unpublishedOffset) { // leftover data was fully processed
                //             _channel.unpublishedSamples.clear();
                //             if (triggerTags.processedSamples < availableSamples - unpublishedOffset) { // did not process all driver data -> copy to tmp storage
                //                 auto processedEnd = triggerTags.processedSamples - unpublishedOffset;
                //                 auto unusedDriverSamples = availableSamples - unpublishedOffset - triggerTags.processedSamples;
                //                 const auto driverData = std::span(_channel.driverBuffer).subspan(processedEnd, unusedDriverSamples);
                //                 _channel.unpublishedSamples.reserve(unusedDriverSamples);
                //                 std::ranges::copy(driverData, std::back_inserter(_channel.unpublishedSamples));
                //             }
                //         } else { // there is stil data left from last time -> add all the new driver data
                //             auto leftoverSamples = unpublishedOffset - triggerTags.processedSamples;
                //             std::copy(_channel.unpublishedSamples.begin() + triggerTags.processedSamples, _channel.unpublishedSamples.end(), _channel.unpublishedSamples.begin());
                //             _channel.unpublishedSamples.resize(leftoverSamples);
                //             auto unusedDriverSamples = availableSamples - unpublishedOffset;
                //             _channel.unpublishedSamples.reserve(leftoverSamples + unusedDriverSamples);
                //             const auto driverData = std::span(_channel.driverBuffer).subspan(offset, unusedDriverSamples);
                //             std::ranges::copy(driverData, std::back_inserter(_channel.unpublishedSamples));
                //         }
                //     }

                //     _nSamplesPublished += triggerTags.processedSamples;
                // }
                return gr::work::Status::OK;
            } else {
                this->emitErrorMessage(std::format("{}::processBulk: error while getting Latest streaiming values: ", this->name), detail::getErrorMessage(status));
                return gr::work::Status::ERROR;
            }
        } else { // Triggered (Rapid Block) Acquisition
            //{
            //    const auto nSamples = pre_samples + post_samples;

            //    uint32_t nCompletedCaptures = 1;

            //    if (const auto status = _picoscope->getNoOfCaptures(&nCompletedCaptures); status != PICO_OK) {
            //        this->emitErrorMessage(std::format("{}::processBulk getNofCaptures", this->name), detail::getErrorMessage(status));
            //        return gr::work::Status::ERROR;
            //    }

            //    nCompletedCaptures                  = std::min(static_cast<gr::Size_t>(nCompletedCaptures), n_captures.value);
            //    const std::size_t availableCaptures = calculateAvailableOutputs(nCompletedCaptures, digitalOutSpan, outputs);

            //    for (std::size_t iCapture = 0; iCapture < availableCaptures; iCapture++) {
            //        const auto getValuesResult = rapidBlockGetValues(iCapture, nSamples);
            //        if (nSamples != getValuesResult.nSamples) {
            //            this->emitErrorMessage(std::format("{}::processBulk", this->name), std::format("Number of retrieved samples ({}) doesn't equal to required samples: pre_samples + post_samples ({})", getValuesResult.nSamples, nSamples));
            //        }
            //        if (getValuesResult.error) {
            //            this->emitErrorMessage(std::format("{}::processBulk", this->name), getValuesResult.error.message());
            //            return gr::work::Status::ERROR;
            //        }
            //        auto now               = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch());
            //        auto acquisitionLength = std::chrono::nanoseconds(static_cast<long>(static_cast<float>(getValuesResult.nSamples) * (1e9f / sample_rate)));
            //        processDriverDataRapidBlock(iCapture, getValuesResult.nSamples, timingInSpan, digitalOutSpan, outputs, now - acquisitionLength);
                      // using TSample = T::value_type;
                      // for (std::size_t channelIdx = 0; channelIdx < _channels.size(); channelIdx++) {
                      //     outputs[channelIdx][iCapture] = createDataset(_channels[channelIdx], nSamples);
                      //     processSamplesOneChannel<TSample>(nSamples, 0, _channels[channelIdx], outputs[channelIdx][iCapture].signal_values);
                      //     // add Tags
                      //     if constexpr (std::is_same_v<TSample, float> || std::is_same_v<TSample, std::int16_t>) {
                      //         gr::dataset::updateMinMax<TSample>(outputs[channelIdx][iCapture]);
                      //     } else {
                      //         // TODO: fix UncertainValue, it requires changes in GR4
                      //     }
                      // }
                      // if constexpr (TPSImpl::N_DIGITAL_CHANNELS > 0) {
                      //     digitalOutSpan[iCapture] = createDatasetDigital(nSamples);
                      //     _picoscope->copyDigitalBuffersToOutput(digitalOutSpan[iCapture].signal_values, nSamples, _digitalBuffers, digital_port_invert_output);
                      // }

                      // // perform matching
                      // tagMatcher.reset(); // reset the tag matcher because for triggered acquisition there is always a gap in the data
                      // const auto triggerSourceIndex = _picoscope->convertToOutputIndex(trigger_source);
                      // if (triggerSourceIndex) {
                      //     std::span                samples(outputs[triggerSourceIndex.value()].data(), nSamples);
                      //     auto                     triggerOffsets = findAnalogTriggers(_channels[triggerSourceIndex.value()], outputs[triggerSourceIndex.value()][iCapture].signalValues(0));
                      //     std::span<const gr::Tag> tagspan        = std::span(timingInSpan.rawTags);
                      //     auto                     tags           = tagspan | std::views::transform([](const auto& t) { return t.map; }) | std::ranges::to<std::vector<gr::property_map>>();
                      //     auto                     triggerTags    = tagMatcher.match(tags, triggerOffsets, nSamples, acquisitionTime);
                      //     if (!triggerTags.tags.empty()) {
                      //         for (std::size_t channelIdx = 0; channelIdx < _channels.size(); ++channelIdx) {
                      //             auto& output = outputs[channelIdx][iCapture];
                      //             for (auto& [index, map] : triggerTags.tags) {
                      //                 if (static_cast<std::ptrdiff_t>(index) >= 0u) {
                      //                     output.timing_events[0].push_back({index, map});
                      //                 }
                      //             }
                      //         }
                      //     }
                      //     for (const auto& msg : triggerTags.messages) {
                      //         this->emitErrorMessage(std::format("{}::TimingMatcher", this->name), msg);
                      //     }
                      // }
            _nSamplesPublished++;
            //    }

            //    // if RapidBlock OR trigger_source is not set then consume all input tags
            //    if (acquisitionMode == AcquisitionMode::RapidBlock || _picoscope->convertToOutputIndex(trigger_source) == std::nullopt) {
            //        const std::size_t nInputs = timingInSpan.size();
            //        std::ignore               = timingInSpan.consume(nInputs);
            //        timingInSpan.consumeTags(nInputs);
            //    }

            //    for (std::size_t i = 0; i < outputs.size(); i++) {
            //        if (availableCaptures < nCompletedCaptures) {
            //            outputs[i].publishTag(gr::property_map{{gr::tag::N_DROPPED_SAMPLES.shortKey(), nSamples * (nCompletedCaptures - availableCaptures)}}, 0);
            //        }
            //        outputs[i].publish(nCompletedCaptures);
            //    }
            //    digitalOutSpan.publish(nCompletedCaptures);
            //}

            //if (_channels.empty()) {
            //    return gr::work::Status::OK;
            //}

            //if (trigger_once) {
            //    return gr::work::Status::DONE;
            //}
            // handle arming and disarming based on timing tag
            // if acquisition is active:
            //   check if data is ready
            //     fetch finished segments
            //     findTriggers
            //     match tags
            //     publish datasets
            //     restart running
            // if acquisition is not running but armed
            //   startAcquisition
        }
    }

    void handleSoftwareArming() {
        if (timingIn.isConnected() && _tagReaderInternal.available() > 0) {
            gr::ReaderSpanLike auto tagData = _tagReaderInternal.get();

            if (!trigger_arm.value.empty() || !trigger_disarm.value.empty()) {
                // find last arm trigger or last disarm trigger
                const std::size_t IndexNotSet            = std::numeric_limits<std::size_t>::max();
                std::size_t       lastArmTriggerIndex    = IndexNotSet;
                std::size_t       lastDisarmTriggerIndex = IndexNotSet;

                for (int i = static_cast<int>(tagData.size()) - 1; i >= 0; i--) {
                    const auto     iSizeT = static_cast<std::size_t>(i);
                    const gr::Tag& tag    = tagData[iSizeT];

                    if (lastArmTriggerIndex == IndexNotSet && detail::tagContainsTrigger(tag, _armTriggerNameAndCtx)) {
                        lastArmTriggerIndex = iSizeT;
                    }
                    if (lastDisarmTriggerIndex == IndexNotSet && detail::tagContainsTrigger(tag, _disarmTriggerNameAndCtx)) {
                        lastDisarmTriggerIndex = iSizeT;
                    }
                    // both arm/disarm triggers were found
                    if (lastArmTriggerIndex != IndexNotSet && lastDisarmTriggerIndex != IndexNotSet) {
                        break;
                    }
                }

                if (lastArmTriggerIndex != IndexNotSet && lastDisarmTriggerIndex == IndexNotSet) { // only arm
                    arm();
                }

                if (lastArmTriggerIndex == IndexNotSet && lastDisarmTriggerIndex != IndexNotSet) { // only disarm
                    disarm();
                }

                if (lastArmTriggerIndex != IndexNotSet && lastDisarmTriggerIndex != IndexNotSet) { // both arm and disarm
                    disarm();
                    if (lastArmTriggerIndex > lastDisarmTriggerIndex) { // disarm before arm
                        arm();
                    }
                }
            } // arm/disarm triggers
            std::ignore = tagData.consume(tagData.size());
        } // Tags are available
    }

    void settingsChanged(const gr::property_map& /*oldSettings*/, const gr::property_map& newSettings) {
        _channels.clear();
        _channels.resize(channel_ids.value.size());

        for (std::size_t i = 0; i < channel_ids.value.size(); ++i) {
            detail::Channel& ch = _channels[i];
            ch.id               = channel_ids.value[i];
            ch.sampleRate       = sample_rate;

            if (i < signal_names.value.size()) {
                ch.name = signal_names.value[i];
            } else {
                ch.name = std::format("signal {} ({})", i, channel_ids.value[i]);
            }
            if (i < signal_units.value.size()) {
                ch.unit = std::string(signal_units.value[i]);
            }
            if (i < signal_quantities.value.size()) {
                ch.quantity = std::string(signal_quantities.value[i]);
            }
            if (i < channel_ranges.value.size()) {
                ch.range = channel_ranges.value[i];
            }
            if (i < channel_analog_offsets.value.size()) {
                ch.analogOffset = channel_analog_offsets.value[i];
            }
            if (i < signal_scales.value.size()) {
                ch.scale = signal_scales.value[i];
            }
            if (i < signal_offsets.value.size()) {
                ch.offset = signal_offsets.value[i];
            }
            if (i < channel_couplings.value.size()) {
                ch.coupling = detail::convertToEnum<Coupling>(channel_couplings.value[i]);
            }

            ch.driverBuffer.resize(detail::kDriverBufferSize);
        }

        if (newSettings.contains("trigger_arm") || newSettings.contains("trigger_disarm")) {
            _armTriggerNameAndCtx    = detail::createTriggerNameAndCtx(trigger_arm);
            _disarmTriggerNameAndCtx = detail::createTriggerNameAndCtx(trigger_disarm);

            if (!trigger_arm.value.empty() && !trigger_disarm.value.empty() && trigger_arm == trigger_disarm) {
                this->emitErrorMessage(std::format("{}::settingsChanged()", this->name), gr::Error("Ill-formed settings: `trigger_arm` == `trigger_disarm`"));
            }
        }

        if (newSettings.contains("trigger_source") && detail::isDigitalTrigger(trigger_source)) {
            const auto parseRes = detail::parseDigitalTriggerSource(trigger_source);
            if (parseRes.has_value()) {
                _digitalChannelNumber = parseRes.value();
            } else {
                this->emitErrorMessage(std::format("{}::settingsChanged()", this->name), parseRes.error());
            }
        }

        const bool needsReinit = newSettings.contains("sample_rate") || newSettings.contains("pre_samples") || newSettings.contains("post_samples")                  //
                                 || newSettings.contains("n_captures") || newSettings.contains("streaming_mode_poll_rate") || newSettings.contains("auto_arm")       //
                                 || newSettings.contains("channel_ids") || newSettings.contains("channel_ranges") || newSettings.contains("channel_analog_offsets")  //
                                 || newSettings.contains("channel_couplings") || newSettings.contains("trigger_source") || newSettings.contains("trigger_threshold") //
                                 || newSettings.contains("trigger_direction") || newSettings.contains("digital_port_threshold") || newSettings.contains("matcher_timeout");

        if (needsReinit) {
            initialize();
            if (auto_arm) {
                disarm();
                arm();
            }
        }
    }

    void start() noexcept {
        if (_picoscope && _picoscope->isOpened()) {
            return;
        }

        try {
            open();
            initialize();
            if (auto_arm) {
                arm();
            }
            serial_number = serialNumber();
            std::println("Picoscope serial number: {}", serial_number);
            std::println("Picoscope device variant: {}", deviceVariant());
        } catch (const std::exception& e) {
            std::println(std::cerr, "{}", e.what());
        }
    }

    void stop() noexcept {
        disarm();
        close();
    }

    void disarm() {
        if (_picoscope && _picoscope->isOpened()) {
            return;
        }

        if (const auto status = _picoscope->driverStop(); status != PICO_OK) {
            this->emitErrorMessage(std::format("{}::disarm()", this->name), gr::Error(detail::getErrorMessage(status)));
        }
        _isArmed = false;
    }

    void open() {
        if (_picoscope && _picoscope->isOpened()) {
            return;
        }

        _picoscope = Picoscope{};
        if (const auto status = _picoscope->openUnit(serial_number); status == PICO_POWER_SUPPLY_NOT_CONNECTED || status == PICO_USB3_0_DEVICE_NON_USB3_0_PORT) {
            if (const auto statusPower = _picoscope->changePowerSource(status); statusPower != PICO_OK) {
                this->emitErrorMessage(std::format("{}::open() changePowerSource", this->name), gr::Error(detail::getErrorMessage(statusPower)));
            }
        }
    }

    void close() {
        if (!_picoscope || !_picoscope->isOpened()) {
            return;
        }
        if (const auto status = _picoscope->closeUnit(); status != PICO_OK) {
            this->emitErrorMessage(std::format("{}::close()", this->name), gr::Error(detail::getErrorMessage(status)));
        } else {
            _picoscope = std::nullopt;
        }
    }

    void initialize() {
        if (!_picoscope || !_picoscope->isOpened()) {
            return;
        }

        // maximum value is used for conversion to volts
        if (const auto status = _picoscope->maximumValue(&(_maxValue)); status != PICO_OK) {
            _picoscope->closeUnit();
            this->emitErrorMessage(std::format("{}::initialize() maximumValue", this->name), gr::Error(detail::getErrorMessage(status)));
        }

        // configure memory segments and number of capture fo RapidBlock mode
        if constexpr (acquisitionMode == AcquisitionMode::RapidBlock) {
            int32_t maxSamples;
            if (const auto status = _picoscope->memorySegments(static_cast<uint32_t>(n_captures), &maxSamples); status != PICO_OK) {
                this->emitErrorMessage(std::format("{}::initialize() MemorySegments", this->name), gr::Error(detail::getErrorMessage(status)));
            }

            if (const auto status = _picoscope->setNoOfCaptures(static_cast<uint32_t>(n_captures)); status != PICO_OK) {
                this->emitErrorMessage(std::format("{}::initialize() SetNoOfCaptures", this->name), gr::Error(detail::getErrorMessage(status)));
            }
        }

        // configure analog channels
        for (const auto& channel : _channels) {
            const auto channelEnum = _picoscope->convertToChannel(channel.id);
            if (!channelEnum) {
                this->emitErrorMessage(std::format("{}::initialize() SetChannel", this->name), gr::Error(std::format("could not convert channel id to channel: {}", channel.id)));
                return;
            }
            assert(channelEnum);
            const auto coupling = _picoscope->convertToCoupling(channel.coupling);
            const auto range    = _picoscope->convertToRange(channel.range);

            const auto status = _picoscope->setChannel(*channelEnum, true, coupling, range, static_cast<float>(channel.analogOffset));
            if (status != PICO_OK) {
                this->emitErrorMessage(std::format("{}::initialize() SetChannel", this->name), gr::Error(detail::getErrorMessage(status)));
            }
        }

        // configure digital ports
        if constexpr (TPSImpl::N_DIGITAL_CHANNELS > 0) {
            if (const auto status = _picoscope->setDigitalPorts(digital_port_threshold); status != PICO_OK) {
                this->emitErrorMessage(std::format("{}::initialize() setDigitalPorts", this->name), gr::Error(detail::getErrorMessage(status)));
            }
        }

        // apply trigger configuration
        if (detail::isAnalogTrigger(trigger_source) && acquisitionMode == AcquisitionMode::RapidBlock) {
            const auto channelEnum = _picoscope->convertToChannel(trigger_source);
            assert(channelEnum != std::nullopt);
            const auto    direction    = _picoscope->convertToThresholdDirection(detail::convertToEnum<TriggerDirection>(trigger_direction));
            const int16_t thresholdADC = convertVoltageToADCCount(trigger_threshold);

            const auto status = _picoscope->setSimpleTrigger(true, channelEnum.value(), thresholdADC, direction, 0 /* delay */, 0 /* auto trigger */);
            if (status != PICO_OK) {
                this->emitErrorMessage(std::format("{}::initialize() setSimpleTrigger", this->name), gr::Error(detail::getErrorMessage(status)));
            }
        } else if (detail::isDigitalTrigger(trigger_source) && acquisitionMode == AcquisitionMode::RapidBlock) {
            if constexpr (TPSImpl::N_DIGITAL_CHANNELS > 0) {
                const auto status = _picoscope->setTriggerDigitalPort(_digitalChannelNumber, detail::convertToEnum<TriggerDirection>(trigger_direction));
                if (status != PICO_OK) {
                    this->emitErrorMessage(std::format("{}::initialize() setTriggerDigitalPort", this->name), gr::Error(detail::getErrorMessage(status)));
                }
            }
        } else {
            // Disable any trigger conditions, so captures occur IMMEDIATELY without waiting for any event
            // Note: To prevent any trigger events, one needs to set the threshold for all channels to the maximum value
            // const int16_t thresholdADC = _picoscope->maxADCCount() - 255; // 255 seems to be a magic number, most probably it is used for additional meta information
            // const auto status = _picoscope->setSimpleTrigger(_handle, true, channelEnum.value(), thresholdADC, direction, 0, 0);
            for (std::size_t i = 0; i < static_cast<std::size_t>(_picoscope->maxChannel()); i++) {
                const auto channelEnum = _picoscope->convertToChannel(i);
                assert(channelEnum != std::nullopt);
                const auto    direction    = _picoscope->convertToThresholdDirection(detail::convertToEnum<TriggerDirection>("Rising"));
                const int16_t thresholdADC = 0;

                const auto status = _picoscope->setSimpleTrigger(false, channelEnum.value(), thresholdADC, direction, 0 /* delay */, 0 /* auto trigger */);
                if (status != PICO_OK) {
                    this->emitErrorMessage(std::format("{}::initialize() setSimpleTrigger", this->name), gr::Error(detail::getErrorMessage(status)));
                }
            }
        }

        tagMatcher.sampleRate = sample_rate;
        tagMatcher.timeout    = std::chrono::nanoseconds(matcher_timeout);
    }

    void arm() {
        if (!_picoscope || !_picoscope->isOpened()) {
            return;
        }

        if constexpr (acquisitionMode == AcquisitionMode::RapidBlock) {
            if (_isArmed) {
                const auto timebaseRes = _picoscope->convertSampleRateToTimebase(sample_rate);
                _actualSampleRate      = timebaseRes.actualFreq;

                static auto    redirector = [](int16_t, PICO_STATUS status, void* vobj) { static_cast<decltype(this)>(vobj)->rapidBlockCallback({status}); };
                int32_t        timeIndisposedMs;
                const uint32_t segmentIndex = 0; // only one segment for streaming mode

                const auto status = _picoscope->runBlock(static_cast<int32_t>(pre_samples), static_cast<int32_t>(post_samples), timebaseRes.timebase, &timeIndisposedMs, segmentIndex, static_cast<TPSImpl::BlockReadyType>(redirector), this);

                if (status != PICO_OK) {
                    this->emitErrorMessage(std::format("{}::arm() RunBlock", this->name), gr::Error(detail::getErrorMessage(status)));
                }
            }
        } else { // Streaming
            using fair::picoscope::detail::kDriverBufferSize;
            if (const auto ec = setBuffers(kDriverBufferSize); ec) {
                this->emitErrorMessage(std::format("{}::arm() setBuffers", this->name), ec.message());
            }

            TimeInterval timeInterval = detail::convertSampleRateToTimeInterval(sample_rate);

            const auto status = _picoscope->runStreaming(        //
                &timeInterval.interval,                          // in: desired interval, out: actual interval
                _picoscope->convertTimeUnits(timeInterval.unit), // time unit of interval
                0,                                               // pre-trigger-samples (unused)
                static_cast<uint32_t>(kDriverBufferSize),        // post-trigger-samples
                false,                                           // autoStop
                1,                                               // downsampling ratio
                _picoscope->ratioNone(),                         // downsampling ratio mode
                static_cast<uint32_t>(kDriverBufferSize));       // the size of the overview buffers

            if (status != PICO_OK) {
                this->emitErrorMessage(std::format("{}::arm() RunStreaming", this->name), gr::Error(detail::getErrorMessage(status)));
            }
            _actualSampleRate = detail::convertTimeIntervalToSampleRate(timeInterval);
        }
    }

    void rapidBlockCallback(Error /*ec*/) requires(acquisitionMode == AcquisitionMode::RapidBlock) { this->progress->incrementAndGet(1); } // wake-up block processing

    constexpr T createDataset(detail::Channel& channel, std::size_t nSamples)
    requires(acquisitionMode == AcquisitionMode::RapidBlock)
    {
        T ds{};
        ds.timestamp = 0;

        ds.axis_names = {channel.name};
        ds.axis_units = {channel.unit};

        ds.extents           = {static_cast<int32_t>(nSamples)};
        ds.layout            = gr::LayoutRight{};
        ds.signal_names      = {channel.name};
        ds.signal_units      = {channel.unit};
        ds.signal_quantities = {channel.quantity};

        ds.signal_values.resize(nSamples);
        ds.signal_ranges.resize(1);
        ds.timing_events.resize(1);
        ds.axis_values.resize(1);
        ds.axis_values[0].resize(nSamples);

        // generate time axis
        int         i            = 0;
        const auto  pre          = static_cast<int>(pre_samples);
        const float samplePeriod = 1.0f / sample_rate;
        std::ranges::generate(ds.axis_values[0], [&i, pre, samplePeriod]() {
            if constexpr (std::is_same_v<TSample, float> || std::is_same_v<TSample, gr::UncertainValue<float>>) {
                float t = static_cast<float>(i - pre) * samplePeriod;
                ++i;
                return TSample{t};
            } else if constexpr (std::is_same_v<TSample, std::int16_t>) {
                return static_cast<std::int16_t>(i++);
            }
        });

        ds.meta_information.resize(1);
        ds.meta_information[0] = channel.toTagMap();

        return ds;
    }

    constexpr TDigitalOutput createDatasetDigital(std::size_t nSamples)
    requires(TPSImpl::N_DIGITAL_CHANNELS > 0 && acquisitionMode == AcquisitionMode::RapidBlock)
    {
        TDigitalOutput ds{};
        ds.extents = {1, static_cast<int32_t>(nSamples)};
        ds.layout  = gr::LayoutRight{};

        ds.signal_values.resize(nSamples);
        ds.signal_ranges.resize(1);
        ds.timing_events.resize(1);

        return ds;
    }

    template<typename TSample>
    std::vector<std::size_t> findAnalogTriggers(const detail::Channel& triggerChannel, std::span<TSample> samples) {
        if (samples.empty()) {
            return {};
        }

        std::vector<std::size_t> triggerOffsets; // relative offset of detected triggers
        const float              band = triggerChannel.range / 100.f;

        const auto toFloat = [](TSample raw) {
            if constexpr (std::is_same_v<TSample, float>) {
                return raw;
            } else if constexpr (std::is_same_v<TSample, int16_t>) {
                return static_cast<float>(raw);
            } else if constexpr (std::is_same_v<TSample, gr::UncertainValue<float>>) {
                return raw.value;
            } else {
                static_assert(gr::meta::always_false<TSample>, "This type is not supported.");
            }
        };

        bool       triggerState         = toFloat(samples[0]) >= trigger_threshold;
        const auto triggerDirectionEnum = detail::convertToEnum<TriggerDirection>(trigger_direction);
        if (triggerDirectionEnum == TriggerDirection::Rising || triggerDirectionEnum == TriggerDirection::High) {
            for (std::size_t i = 0; i < samples.size(); i++) {
                const float value = toFloat(samples[i]);
                if (triggerState == 0 && value >= trigger_threshold) {
                    triggerState = 1;
                    triggerOffsets.push_back(i);
                } else if (triggerState == 1 && value <= trigger_threshold - band) {
                    triggerState = 0;
                }
            }
        } else if (triggerDirectionEnum == TriggerDirection::Falling || triggerDirectionEnum == TriggerDirection::Low) {
            for (std::size_t i = 0; i < samples.size(); i++) {
                const float value = toFloat(samples[i]);
                if (triggerState == 1 && value <= trigger_threshold) {
                    triggerState = 0;
                    triggerOffsets.push_back(i);
                } else if (triggerState == 0 && value >= trigger_threshold + band) {
                    triggerState = 1;
                }
            }
        }
        return triggerOffsets;
    }

    std::vector<std::size_t> findDigitalTriggers(int digitalChannelNumber, std::span<std::uint16_t> samples) requires(TPSImpl::N_DIGITAL_CHANNELS > 0) {
        if (samples.empty()) {
            return {};
        }
        const auto mask = static_cast<uint16_t>(1U << digitalChannelNumber);

        std::vector<std::size_t> triggerOffsets;

        const auto triggerDirectionEnum = detail::convertToEnum<TriggerDirection>(trigger_direction);
        bool       triggerState         = (samples[0] & mask) > 0;
        if (triggerDirectionEnum == TriggerDirection::Rising || triggerDirectionEnum == TriggerDirection::High) {
            for (std::size_t i = 0; i < samples.size(); i++) {
                if (!triggerState && (samples[i] & mask)) {
                    triggerState = 1;
                    triggerOffsets.push_back(i);
                } else if (triggerState && !(samples[i] & mask)) {
                    triggerState = 0;
                }
            }
        } else if (triggerDirectionEnum == TriggerDirection::Falling || triggerDirectionEnum == TriggerDirection::Low) {
            for (std::size_t i = 0; i < samples.size(); i++) {
                if (triggerState && !(samples[i] & mask)) {
                    triggerState = 0;
                    triggerOffsets.push_back(i);
                } else if (!triggerState && (samples[i] & mask)) {
                    triggerState = 1;
                }
            }
        }

        return triggerOffsets;
    }

    [[nodiscard]] constexpr int16_t convertVoltageToADCCount(float value) {
        value = std::clamp(value, _picoscope->extTriggerMinValueVoltage(), _picoscope->extTriggerMaxValueVoltage());
        return static_cast<int16_t>((value / _picoscope->extTriggerMaxValueVoltage()) * static_cast<float>(_picoscope->extTriggerMaxValue()));
    }

    [[nodiscard]] Error setBuffers(size_t nSamples, uint32_t segmentIndex = 0UZ) {
        for (auto& channel : _channels) {
            const auto channelEnum = _picoscope->convertToChannel(channel.id);
            if (!channelEnum) {
                return {PICO_INVALID_CHANNEL};
            }

            channel.driverBuffer.resize(std::max(nSamples, channel.driverBuffer.size()));
            if constexpr (acquisitionMode == AcquisitionMode::Streaming) {
                const auto status = _picoscope->setDataBuffer(*channelEnum, channel.driverBuffer.data(), static_cast<int32_t>(nSamples), _picoscope->ratioNone());
                if (status != PICO_OK) {
                    return {status};
                }
            } else {
                const auto status = _picoscope->setDataBufferForSegment(*channelEnum, channel.driverBuffer.data(), static_cast<int32_t>(nSamples), segmentIndex, _picoscope->ratioNone());
                if (status != PICO_OK) {
                    return {status};
                }
            }
        }

        if constexpr (TPSImpl::N_DIGITAL_CHANNELS > 0) {
            if (const auto status = _picoscope->setDigitalBuffers(nSamples, segmentIndex, _digitalBuffers); status != PICO_OK) {
                std::println(std::cerr, "setDigitalBuffers: {}", detail::getErrorMessage(status));
                return {status};
            }
        }

        return {};
    }

    [[nodiscard]] std::string getUnitInfoTopic(int16_t handle, PICO_INFO info) const {
        std::array<int8_t, 40> line{};
        int16_t                requiredSize;

        auto status = _picoscope->getUnitInfo(line.data(), line.size(), &requiredSize, info);
        if (status == PICO_OK) {
            return {reinterpret_cast<char*>(line.data()), static_cast<std::size_t>(requiredSize)};
        }

        return {};
    }

    [[nodiscard]] std::string driverVersion() const {
        const std::string prefix  = "Picoscope Linux Driver, ";
        auto              version = getUnitInfoTopic(PICO_DRIVER_VERSION);

        if (auto i = version.find(prefix); i != std::string::npos) {
            version.erase(i, prefix.length());
        }
        return version;
    }

    [[nodiscard]] std::string hardwareVersion() const { return getUnitInfoTopic(PICO_HARDWARE_VERSION); }

    [[nodiscard]] std::string serialNumber() const { return getUnitInfoTopic(PICO_BATCH_AND_SERIAL); }

    [[nodiscard]] std::string deviceVariant() const { return getUnitInfoTopic(PICO_VARIANT_INFO); }
};

} // namespace fair::picoscope

#endif
