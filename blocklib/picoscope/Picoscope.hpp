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

constexpr std::size_t kDriverBufferSize = 65536; // TODO: make driver buffer size configurable or dependant on settings?

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
    gr::HistoryBuffer<int16_t> unpublished{kDriverBufferSize}; // Size will be adjusted to fit n samples

    // settings
    std::string name;
    std::string unit         = "V";
    std::string quantity     = "Voltage";
    float       range        = 2.f;
    float       analogOffset = 0.f;
    float       scale        = 1.f;
    float       offset       = 0.f;
    Coupling    coupling     = Coupling::DC;

    [[nodiscard]] gr::property_map toTagMap(float sampleRate) const {
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
    std::string                triggerName{};
    std::optional<std::string> ctx{};
};

[[nodiscard]] static inline TriggerNameAndCtx createTriggerNameAndCtx(const std::string& triggerNameAndCtx) {
    if (triggerNameAndCtx.empty()) {
        return {};
    }
    const std::size_t pos = triggerNameAndCtx.find('/');
    if (pos != std::string::npos) { // trigger_name and ctx
        return {triggerNameAndCtx.substr(0, pos), (pos < triggerNameAndCtx.size() - 1) ? triggerNameAndCtx.substr(pos + 1) : ""};
    } else { // only trigger_name
        return {triggerNameAndCtx, std::nullopt};
    }
}

[[nodiscard]] static inline bool tagContainsTrigger(const gr::property_map& map, const TriggerNameAndCtx& triggerNameAndCtx) {
    if (triggerNameAndCtx.ctx) { // trigger_name and ctx
        if (map.contains(gr::tag::TRIGGER_NAME.shortKey()) && map.contains(gr::tag::CONTEXT.shortKey())) {
            const std::string tagTriggerName = std::get<std::string>(map.at(gr::tag::TRIGGER_NAME.shortKey()));
            return !tagTriggerName.empty() && tagTriggerName == triggerNameAndCtx.triggerName && std::get<std::string>(map.at(gr::tag::CONTEXT.shortKey())) == *triggerNameAndCtx.ctx;
        }
    } else { // only trigger_name
        if (map.contains(gr::tag::TRIGGER_NAME.shortKey())) {
            const std::string tagTriggerName = std::get<std::string>(map.at(gr::tag::TRIGGER_NAME.shortKey()));
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
struct Picoscope : gr::Block<Picoscope<T, TPSImpl, TTagMatcher>, gr::SupportedTypes<int16_t, float, gr::UncertainValue<float>, gr::DataSet<int16_t>, gr::DataSet<float>, gr::DataSet<gr::UncertainValue<float>>>> {
    using SuperT = gr::Block<Picoscope<T, TPSImpl, TTagMatcher>, gr::SupportedTypes<int16_t, float, gr::UncertainValue<float>, gr::DataSet<int16_t>, gr::DataSet<float>, gr::DataSet<gr::UncertainValue<float>>>>;
    static constexpr AcquisitionMode acquisitionMode = gr::DataSetLike<T> ? AcquisitionMode::RapidBlock : AcquisitionMode::Streaming;

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
    A<int16_t, "digital port threshold (ADC: –32767 (–5 V) to 32767 (+5 V))">        digital_port_threshold     = 0;     // only used if digital ports are available: 3000a, 5000a series
    A<bool, "invert digital port output">                                            digital_port_invert_output = false; // only used if digital ports are available: 3000a, 5000a series
    A<gr::Size_t, "Timeout after which to not match trigger pulses", gr::Unit<"ns">> matcher_timeout            = 10'000'000z;

    gr::PortIn<std::uint8_t, gr::Async> timingIn;

    std::array<gr::PortOut<T>, TPSImpl::N_ANALOG_CHANNELS> out;

    // only used for ports with digital ports (3000a + 5000a)
    using TDigitalOutput = std::conditional_t<gr::DataSetLike<T>, gr::DataSet<uint16_t>, uint16_t>;
    gr::PortOut<TDigitalOutput> digitalOut;

    float       _actualSampleRate  = 0;
    std::size_t _nSamplesPublished = 0; // for debugging purposes

    detail::TriggerNameAndCtx _armTriggerNameAndCtx; // store parsed information to optimise performance
    detail::TriggerNameAndCtx _disarmTriggerNameAndCtx;

    int _digitalChannelNumber = -1; // used only if digital trigger is set

    GR_MAKE_REFLECTABLE(Picoscope, timingIn, out, digitalOut, serial_number, sample_rate, pre_samples, post_samples, n_captures,                                //
        auto_arm, trigger_once, channel_ids, signal_names, signal_units, signal_quantities, channel_ranges, channel_analog_offsets, signal_scales, signal_offsets, channel_couplings, //
        trigger_source, trigger_threshold, trigger_direction, digital_port_threshold, digital_port_invert_output, trigger_arm, trigger_disarm, matcher_timeout);

private:
    std::optional<TPSImpl> _picoscope;

    bool                          _isArmed   = false;   // for RapidBlock mode only
    std::vector<gr::property_map> _currentTimingTags{}; // contains all tag maps for the currently running acquisition
    std::vector<gr::property_map> _nextTimingTags{};    // contains all tag maps for the next acquisition (this is needed if a disarm+arm sequence interrupts a running acquisition)
    std::vector<detail::Channel>  _channels;
    int16_t                       _maxValue  = 0; // maximum ADC count used for ADC conversion

    // only used for ports with digital ports (3000a + 5000a)
    std::array<std::vector<int16_t>, 2> _digitalBuffers;
    gr::HistoryBuffer<uint16_t>         _unpublishedDigital{detail::kDriverBufferSize};

    TTagMatcher tagMatcher{.timeout = std::chrono::nanoseconds(matcher_timeout.value), .sampleRate = sample_rate.value};

public:
    ~Picoscope() { stop(); }
    using SuperT::SuperT; // inherit Block constructor

    template<gr::OutputSpanLike TOutSpan>
    requires(acquisitionMode == AcquisitionMode::Streaming)
    gr::work::Status processStreaming(gr::InputSpanLike auto& timingInSpan, std::span<TOutSpan>& outputs, gr::OutputSpanLike auto& digitalOutSpan, TPSImpl::NSamplesType noOfSamples, uint32_t startIndex, int16_t overflow, uint32_t /*triggerAt*/, int16_t /*triggered*/, int16_t /*autoStop*/) {
        auto acquisitionTime = std::chrono::system_clock::now() - std::chrono::nanoseconds(static_cast<long>(1e9f / sample_rate * static_cast<float>(noOfSamples)));
        auto nLeftoverToProcess = std::min(_channels[0].unpublished.size(), outputs[0].size()); // check for leftover samples
        auto nDriverSamplesProcessable = std::min(noOfSamples, static_cast<TPSImpl::NSamplesType>(outputs[0].size() - nLeftoverToProcess));
        // find triggers and match
        fair::picoscope::timingmatcher::MatcherResult matchedTags;
        if (!trigger_source->empty()) { // find triggers
            auto triggerEdges = [&]() -> std::vector<std::size_t> {
                if (detail::isAnalogTrigger(trigger_source)) {
                    auto triggerSourceIndex = TPSImpl::convertToOutputIndex(trigger_source);
                    if (triggerSourceIndex) {
                        std::span samples(_channels[triggerSourceIndex.value()].unpublished.data(), nLeftoverToProcess);
                        return findAnalogTriggers(_channels[triggerSourceIndex.value()], samples);
                    } else {
                        return {};
                    }
                } else if (detail::isDigitalTrigger(trigger_source)) {
                    return {}; // todo: implement digital triggers
                }
            }();
            // match tags for leftover samples
            std::span<const gr::Tag> tag_span = std::span(timingInSpan.rawTags);
            auto tags = tag_span | std::views::transform([](const auto &t) { return t.map; }) | std::ranges::to<std::vector<gr::property_map>>();
            auto acquisitionTimeOffset = std::chrono::nanoseconds(static_cast<long>(static_cast<float>(_channels[0].unpublished.size()) * (1e9f / sample_rate)));
            matchedTags = tagMatcher.match(tags, triggerEdges, nLeftoverToProcess, (acquisitionTime - acquisitionTimeOffset).time_since_epoch());
            // match tags for new samples
            if (matchedTags.processedSamples >= _channels[0].unpublished.size()) {
                auto triggerEdgesDriver = [&]() -> std::vector<std::size_t> {
                    if (detail::isAnalogTrigger(trigger_source)) {
                        auto triggerSourceIndex = TPSImpl::convertToOutputIndex(trigger_source);
                        if (triggerSourceIndex) {
                            std::span samples(outputs[triggerSourceIndex.value()].data() + startIndex, nDriverSamplesProcessable);
                            return findAnalogTriggers(_channels[triggerSourceIndex.value()], samples);
                        } else {
                            return {};
                        }
                    } else if (detail::isDigitalTrigger(trigger_source)) {
                        return {}; // todo: implement digital triggers
                    } else {
                        return {};
                    }
                }();
                auto tagsDriver   = tag_span | std::views::drop(nLeftoverToProcess) | std::views::transform([](const auto &t) { return t.map; }) | std::ranges::to<std::vector<gr::property_map>>();
                auto matchedTags2 = tagMatcher.match(tagsDriver, triggerEdgesDriver, static_cast<std::size_t>(nDriverSamplesProcessable),
                                                     acquisitionTime.time_since_epoch());
                std::ranges::copy(matchedTags2.tags, std::back_inserter(matchedTags.tags));
                std::ranges::copy(matchedTags2.messages, std::back_inserter(matchedTags.messages));
                matchedTags.processedSamples += matchedTags2.processedSamples;
                matchedTags.processedTags += matchedTags2.processedTags;
            }
            // consume timing tags
            auto lastTimingSampleIndex = tag_span[matchedTags.processedTags - 1].index - timingInSpan.streamIndex + 1;
            timingInSpan.consumeTags(lastTimingSampleIndex);
            std::ignore = timingInSpan.consume(lastTimingSampleIndex);
            // emit matcher messages
            for (const auto &msg: matchedTags.messages) {
                this->emitErrorMessage(std::format("{}::TimingMatcher", this->name), msg);
            }
        } else { // end trigger detection
            matchedTags.processedSamples = nLeftoverToProcess + nDriverSamplesProcessable;
            std::ignore = timingInSpan.consume(timingInSpan.size()); // consume all timing tags if there are no triggers configured?
            // todo: publish tags on a best effort basis based on system time and sample counting
        }
        for (const auto &[channel, output]: std::views::zip(_channels, outputs)) { // copy and publish all analog channel data
            const float voltageMultiplier = channel.range / static_cast<float>(_maxValue);
            // copy leftover data from circular buffer
            for (std::size_t i = 0; i < nLeftoverToProcess; ++i) {
                if constexpr (std::is_same_v<T, float>) {
                    output[i] = channel.offset + channel.scale * voltageMultiplier * static_cast<float>(channel.unpublished.front());
                } else if constexpr (std::is_same_v<T, gr::UncertainValue<float>>) {
                    output[i] = gr::UncertainValue(channel.offset + channel.scale * voltageMultiplier * static_cast<float>(channel.unpublished.front()), _picoscope->uncertainty());
                } else if constexpr (std::is_same_v<T, int16_t>) {
                    output[i] = channel.unpublished.front();
                }
                channel.unpublished.pop_front();
            }
            // copy driver data to output
            auto nDriverDataToPublish = std::min(std::min(noOfSamples, static_cast<TPSImpl::NSamplesType>(output.size() - nLeftoverToProcess)), static_cast<TPSImpl::NSamplesType>(matchedTags.processedSamples - nLeftoverToProcess));
            const auto driverData = std::span(channel.driverBuffer).subspan(startIndex, nDriverDataToPublish);
            for (std::size_t i = 0; i < static_cast<std::size_t>(nDriverDataToPublish); ++i) {
                if constexpr (std::is_same_v<T, float>) {
                    output[i + nLeftoverToProcess] = channel.offset + channel.scale * voltageMultiplier * static_cast<float>(driverData[i]);
                } else if constexpr (std::is_same_v<T, gr::UncertainValue<float>>) {
                    output[i + nLeftoverToProcess] = gr::UncertainValue( channel.offset + channel.scale * voltageMultiplier * static_cast<float>(driverData[i]), _picoscope->uncertainty());
                } else if constexpr (std::is_same_v<T, int16_t>) {
                    output[i + nLeftoverToProcess] = driverData[i]; // std::ranges::copy(driverData, output.begin() + static_cast<std::ptrdiff_t>(channel.unpublished.size()));
                }
            }
            // copy leftover data to circular buffer
            std::ranges::copy(std::span(channel.driverBuffer).subspan(startIndex + nDriverDataToPublish, noOfSamples - nDriverDataToPublish), std::back_inserter(channel.unpublished));
            // publish tags
            if (!channel.signalInfoTagPublished) {
                output.publishTag(channel.toTagMap(sample_rate), 0);
                channel.signalInfoTagPublished = true;
            }
            if (overflow == 0xffff) {
                output.publishTag({{"Buffer Overrun", "Buffer overflow, samples might be lost"}}, 0); // TODO: verify correct tag to indicate buffer overrun
            } else if (overflow & (0x1 << TPSImpl::convertToOutputIndex(channel.id).value())) {
                output.publishTag({{"OVERFLOW", "Values in this channel might be out of range"}}, 0); // TODO: verify correct tag for out of range
            }
            for (auto &[index, map]: matchedTags.tags) {
                output.publishTag(map, index);
            }
            // publish samples
            output.publish(matchedTags.processedSamples);
            // diagnostics
        } // end foreach analog channel
        if constexpr (TPSImpl::N_DIGITAL_CHANNELS > 0) { // publish all data for digital channels
            // copy old data
            // todo: implement
            for (std::size_t i = 0; i < nLeftoverToProcess; ++i) {
                digitalOutSpan[i] = _unpublishedDigital.front();
                _unpublishedDigital.pop_front();
            }
            // copy new data
            for (std::size_t i = 0; i < nDriverSamplesProcessable; i++) {
                auto value = static_cast<uint16_t>(0x00FF & _digitalBuffers[1][i]);
                value <<= 8;
                value |= static_cast<uint16_t>(0x00FF & _digitalBuffers[0][i]);
                if (digital_port_invert_output) {
                    value = ~value;
                }
                digitalOutSpan[i] = value;
            }
            // publish TimingTags TODO: add channel metadata?
            for (auto &[index, map]: matchedTags.tags) {
                digitalOutSpan.publishTag(map, index);
            }
            // publishSamples
            digitalOutSpan.publish(matchedTags.processedSamples);
            // copy leftover driver data
            const auto nDriverDataToPublish = std::min(std::min(noOfSamples, static_cast<TPSImpl::NSamplesType>(digitalOutSpan.size() - nLeftoverToProcess)), static_cast<TPSImpl::NSamplesType>(matchedTags.processedSamples - nLeftoverToProcess));
            //std::ranges::copy(std::span(channel.driverBuffer).subspan(startIndex + nDriverDataToPublish, noOfSamples - nDriverDataToPublish), std::back_inserter(channel.unpublished));
            for (std::size_t i = 0; i < noOfSamples - nDriverDataToPublish; ++i) {
                const std::size_t sampleIndex = startIndex + nDriverDataToPublish + i;
                auto value = static_cast<uint16_t>(0x00FF & _digitalBuffers[1][sampleIndex]);
                value <<= 8;
                value |= static_cast<uint16_t>(0x00FF & _digitalBuffers[0][sampleIndex]);
                if (digital_port_invert_output) {
                    value = ~value;
                }
                _unpublishedDigital.push_back(value);
            }
        } // end digital ports
        _nSamplesPublished += matchedTags.processedSamples;
        return gr::work::Status::OK;
    }

    template<gr::OutputSpanLike TOutSpan>
    requires(acquisitionMode == AcquisitionMode::RapidBlock)
    gr::work::Status processTriggered(gr::InputSpanLike auto& timingInSpan, std::span<TOutSpan>& outputs, gr::OutputSpanLike auto& digitalOutSpan) {
        using TSample = T::value_type;
        // auto [lastArmedIndex, lastDisarmedIndex] = handleSoftwareArming(timingInSpan);
        auto armResult = processTagsTriggered(timingInSpan);
        if (!_isArmed) {
            if (armResult.arm) {
                arm();
            }
            for (auto& output : outputs) { output.publish(0); }
            digitalOutSpan.publish(0);
            return gr::work::Status::OK; // nothing to do here as the triggering is currently disabled
        }
        if (armResult.disarm) {
            disarm();
            _isArmed = false; // todo move inside disarm?
        }
        const auto nSamples = pre_samples + post_samples;
        const auto acquisitionTime = std::chrono::system_clock::now();

        uint32_t nCompletedCapturesIncomplete = 1;
        if (const auto status = _picoscope->getNoOfCaptures(&nCompletedCapturesIncomplete); status != PICO_OK) {
            this->emitErrorMessage(std::format("{}::processBulk getNoOfCaptures", this->name), detail::getErrorMessage(status));
            return gr::work::Status::ERROR;
        }
        uint32_t nCompletedCaptures = 1;
        if (const auto status = _picoscope->getNoOfProcessedCaptures(&nCompletedCaptures); status != PICO_OK) {
            this->emitErrorMessage(std::format("{}::processBulk getNoOfProcessedCaptures", this->name), detail::getErrorMessage(status));
            return gr::work::Status::ERROR;
        }
        int16_t ready = 0;
        if (!_isArmed) {
            ready = true;
        } else {
            if (const auto status = _picoscope->isReady(&ready); status != PICO_OK) {
                this->emitErrorMessage(std::format("{}::processBulk isReady", this->name), detail::getErrorMessage(status));
                return gr::work::Status::ERROR;
            }
        }
        std::uint32_t availableCaptures = std::min(nCompletedCapturesIncomplete, static_cast<std::uint32_t>(n_captures.value));
        for (auto &output: outputs) {
            availableCaptures = std::min(availableCaptures, static_cast<std::uint32_t>(output.size()));
        }
        availableCaptures = std::min(availableCaptures, static_cast<std::uint32_t>(digitalOutSpan.size()));
        if (availableCaptures == 0 || !ready) { // no new data to publish
            // todo: consume old timing tags while there are no updates happening
            for (auto& output : outputs) { output.publish(0); }
            digitalOutSpan.publish(0);
            if (nCompletedCaptures == 0 || !ready) {
                return gr::work::Status::INSUFFICIENT_INPUT_ITEMS; // TODO: do we need a new enum value here to indicate to the scheduler that we are blocked waiting on external input
            } else {
                return gr::work::Status::INSUFFICIENT_OUTPUT_ITEMS;
            }
        }

        std::uint32_t getValueSamples = nSamples;
        std::vector<std::int16_t> overflowBits{}; // for Rapid Block: overflow bit for the corresponding channel
        overflowBits.resize(_channels.size());
        if (const auto status = _picoscope->getValuesBulk(&getValueSamples, 0U, availableCaptures - 1U, 1U, _picoscope->ratioNone(), overflowBits.data()); status != PICO_OK || getValueSamples != nSamples) {
            this->emitErrorMessage(std::format("{}::processBulk isReady, samples(expected)={}({})", this->name, getValueSamples, nSamples), detail::getErrorMessage(status));
            return gr::work::Status::ERROR;
        }
        _isArmed = false; // getting the Values implies that the capture was stopped

        std::println("copying {} captures", availableCaptures);
        for (std::size_t i = 0; i < availableCaptures; i++) {
            std::print("capture {} - analog ", i);
            for (std::size_t channelIdx = 0; channelIdx < _channels.size(); channelIdx++) {
                auto& channel = _channels[channelIdx];
                const float voltageMultiplier = channel.range / static_cast<float>(_maxValue);
                outputs[channelIdx][i] = createDataset(_channels[channelIdx], nSamples);
                const auto driverData = std::span(channel.driverBuffer).subspan((i) * nSamples, static_cast<std::size_t>(nSamples));
                for (std::size_t j = 0; j < static_cast<std::size_t>(nSamples); ++j) {
                    if constexpr (std::is_same_v<TSample, float>) {
                        outputs[channelIdx][i].signal_values[j] = channel.offset + channel.scale * voltageMultiplier * static_cast<float>(driverData[j]);
                    } else if constexpr (std::is_same_v<TSample, gr::UncertainValue<float>>) {
                        outputs[channelIdx][i].signal_values[j] = gr::UncertainValue(channel.offset + channel.scale * voltageMultiplier * static_cast<float>(driverData[j]), _picoscope->uncertainty());
                    } else if constexpr (std::is_same_v<TSample, int16_t>) {
                        outputs[channelIdx][i].signal_values[j] = driverData[j]; // std::ranges::copy(driverData, output.begin() + static_cast<std::ptrdiff_t>(channel.unpublished.size()));
                    }
                }
                // add Tags
                if (overflowBits[channelIdx] | (1 << TPSImpl::convertToOutputIndex(channel.id).value())) { // picoscope overrange
                    outputs[channelIdx][i].metaInformation(0UZ).insert({"Overrange", true}); // todo: use correct tag string
                }
                if constexpr (std::is_same_v<TSample, float> || std::is_same_v<TSample, std::int16_t>) {
                    gr::dataset::updateMinMax<TSample>(outputs[channelIdx][i]);
                } else {
                    // TODO: fix UncertainValue, it requires changes in GR4
                }
            }
            std::print("- digital");
            if constexpr (TPSImpl::N_DIGITAL_CHANNELS > 0) {
                digitalOutSpan[i] = createDatasetDigital(nSamples);
                for (std::size_t j = 0; j < nSamples; ++j) {
                    auto value = static_cast<uint16_t>(0x00FF & _digitalBuffers[1][(i * nSamples) + j]);
                    value <<= 8;
                    value |= static_cast<uint16_t>(0x00FF & _digitalBuffers[0][(i * nSamples) + j]);
                    if (digital_port_invert_output) {
                        value = ~value;
                    }
                    digitalOutSpan[i].signal_values[j] = value;
                }
            }

            std::print(" - tags");
            // perform matching
            tagMatcher.reset(); // reset the tag matcher because for triggered acquisition there is always a gap in the data
            const auto triggerSourceIndex = _picoscope->convertToOutputIndex(trigger_source);
            if (triggerSourceIndex) {
                std::vector<std::size_t> triggerOffsets{};
                if (const auto externalChannelIndex = _picoscope->convertToOutputIndex("EXTERNAL"); externalChannelIndex && triggerSourceIndex == externalChannelIndex) {
                    // For the external trigger input, we only know that there was a trigger after `pre_samples` samples.
                    triggerOffsets = {pre_samples};
                } else {
                    std::span samples = std::span(outputs[triggerSourceIndex.value()].data(), nSamples);
                    triggerOffsets    = findAnalogTriggers(_channels[triggerSourceIndex.value()], outputs[triggerSourceIndex.value()][i].signalValues(0));
                }
                //std::span<const gr::Tag> tag_span    = std::span(timingInSpan.rawTags);
                //auto                     tags        = tag_span | std::views::transform([](const auto& t) { return t.map; }) | std::ranges::to<std::vector<gr::property_map>>();
                auto                     triggerTags = tagMatcher.match(_currentTimingTags, triggerOffsets, nSamples, acquisitionTime.time_since_epoch());
                if (!triggerTags.tags.empty()) {
                    for (std::size_t channelIdx = 0; channelIdx < _channels.size(); ++channelIdx) {
                        auto& output = outputs[channelIdx][i];
                        for (auto& [index, map] : triggerTags.tags) {
                            if (static_cast<std::ptrdiff_t>(index) >= 0u) {
                                output.timing_events[0].push_back({static_cast<std::ptrdiff_t>(index), map});
                            }
                        }
                    }
                }
                for (const auto& msg : triggerTags.messages) {
                    this->emitErrorMessage(std::format("{}::TimingMatcher", this->name), msg);
                }
            } else { // no trigger defined -> cannot match timing tags, drop all timing tags
                //const std::size_t nInputs = timingInSpan.size();
                //std::ignore               = timingInSpan.tryConsume(nInputs);
                //timingInSpan.consumeTags(nInputs);
            }
        }

        std::println(" - publish");
        for (std::size_t i = 0; i < outputs.size(); i++) {
            outputs[i].publish(availableCaptures);
        }
        digitalOutSpan.publish(availableCaptures);

        if (trigger_once) {
            return gr::work::Status::DONE;
        } else if (auto_arm || armResult.arm) { // restart the acquisition
            arm();
            _isArmed = true;
            _currentTimingTags = std::move(_nextTimingTags);
            _nextTimingTags.clear();
        }
        return gr::work::Status::OK;
    }

    template<gr::OutputSpanLike TOutSpan>
    gr::work::Status processBulk(gr::InputSpanLike auto& timingInSpan, std::span<TOutSpan>& outputs, gr::OutputSpanLike auto& digitalOutSpan) {
        if (!_picoscope || !_picoscope->isOpened()) { // early return if the picoscope was not yet opened correctly or settings are not yet applied
            // TODO: implement async open and retry
            return {};
        }
        if constexpr (acquisitionMode == AcquisitionMode::Streaming) {
            using self_t = decltype(this);
            struct StreamingContext { // helper class to pass data to the callback function called by the getLatestValues function
                decltype(timingInSpan)& timingIn;
                std::span<TOutSpan>& out;
                decltype(digitalOutSpan)& digitalOut;
                self_t self;
                gr::work::Status result = gr::work::Status::INSUFFICIENT_INPUT_ITEMS;
            } streamingCtx{timingInSpan, outputs, digitalOutSpan, this};

            // fetch new values // note: the callback gets executed inside this function call on the same thread
            const auto status = _picoscope->getStreamingLatestValues(static_cast<TPSImpl::StreamingReadyType>([](int16_t /*handle*/, TPSImpl::NSamplesType noOfSamples, uint32_t startIndex, int16_t overflow, uint32_t triggerAt, int16_t triggered, int16_t autoStop, void* data) {
                auto streamingContext = static_cast<StreamingContext*>(data);
                streamingContext->result = streamingContext->self->processStreaming(streamingContext->timingIn, streamingContext->out, streamingContext->digitalOut, noOfSamples, startIndex, overflow, triggerAt, triggered, autoStop);
            }), &streamingCtx);
            if (status == PICO_OK && streamingCtx.result == gr::work::Status::OK ) { // processed data
                return gr::work::Status::OK;
            } else if (status == PICO_BUSY || status == PICO_DRIVER_FUNCTION || streamingCtx.result == gr::work::Status::INSUFFICIENT_INPUT_ITEMS) { // no data to fetch yet => check and process leftover data and return
                if (_channels[0].unpublished.size() > 0) { // process unpublished data leftover from the latest invocation
                    return  processStreaming(timingInSpan, outputs, digitalOutSpan, 0, 0, 0, 0, 0, 0);
                } else {
                    for (auto& output : outputs) { output.publish(0); }
                    digitalOutSpan.publish(0);
                    return gr::work::Status::OK;
                }
            } else {
                this->emitErrorMessage(std::format("{}::processBulk: error while getting Latest streaming values: ", this->name), detail::getErrorMessage(status));
                return gr::work::Status::ERROR;
            }
        } else { // Triggered (Rapid Block) Acquisition
            return processTriggered(timingInSpan, outputs, digitalOutSpan);
        }
    }

    auto processTagsTriggered(gr::InputSpanLike auto& tagData) {
        struct tagProcessResult {
            bool arm = false;
            bool disarm = false;
        } result;
        bool armed = _isArmed;
        for (const auto& [i, tag_map] : tagData.tags()) {
            if (!trigger_arm.value.empty() || !trigger_disarm.value.empty()) {
                if (detail::tagContainsTrigger(tag_map, _armTriggerNameAndCtx)) {
                    result.arm = true;
                    armed = true;
                    _nextTimingTags.clear();
                }
                if (detail::tagContainsTrigger(tag_map, _disarmTriggerNameAndCtx)) {
                    result.disarm = true;
                    armed = false;
                }
                if (armed) {
                    if (!result.disarm) {
                        _currentTimingTags.push_back(tag_map);
                    } else {
                        _nextTimingTags.push_back(tag_map);
                    }
                }
            }
        }
        tagData.consumeTags(tagData.size());
        tagData.consume(tagData.size());
        if (tagData.rawTags.size() > 0) {
            std::println("processed {} timing tags, arm={}, disarm={}, armed={}, backwardTagForwarding={}", tagData.rawTags.size(), result.arm, result.disarm, armed, this->backwardTagForwarding);
        }
        return result;
    }

    void settingsChanged(const gr::property_map& /*oldSettings*/, const gr::property_map& newSettings) {
        _channels.clear();
        _channels.resize(channel_ids.value.size());

        for (std::size_t i = 0; i < channel_ids.value.size(); ++i) {
            detail::Channel& ch = _channels[i];
            ch.id               = channel_ids.value[i];

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

        // todo: check which changes need re-arming and which can be done during processing. e.g. the channel_ranges can be adjusted without restarting the acquisition

        if (needsReinit) {
            initialize();
            if (auto_arm) {
                disarm();
                arm();
            }
        }
    }

    void start() {
        try {
            open();
            initialize();
            if (auto_arm) {
                arm();
            }
            serial_number = serialNumber(); // todo: need to update setting?
            std::println("Picoscope serial number: {}", serial_number);
            std::println("Picoscope device variant: {}", deviceVariant());
        } catch (const std::exception& e) {
            std::println(std::cerr, "{}", e.what());
        }
    }

    void stop() {
        disarm();
        close();
    }

    void disarm() {
        std::println("PS - disarm A");
        if (!_picoscope || !_picoscope->isOpened()) {
            return;
        }

        if (const auto status = _picoscope->driverStop(); status != PICO_OK) {
            this->emitErrorMessage(std::format("{}::disarm()", this->name), gr::Error(detail::getErrorMessage(status)));
        }
    }

    void open() {
        if (_picoscope && _picoscope->isOpened()) {
            return;
        }

        _picoscope = TPSImpl{};
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
            if (const auto closeStatus = _picoscope->closeUnit(); closeStatus != PICO_OK) {
                this->emitErrorMessage(std::format("{}::initialize() maximumValue (also failed to properly close the picoscope: {})", this->name, detail::getErrorMessage(closeStatus)), gr::Error(detail::getErrorMessage(status)));
            } else {
                this->emitErrorMessage(std::format("{}::initialize() maximumValue", this->name), gr::Error(detail::getErrorMessage(status)));
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

            if (const auto status = _picoscope->setChannel(*channelEnum, true, coupling, range, static_cast<float>(channel.analogOffset)); status != PICO_OK) {
                this->emitErrorMessage(std::format("{}::initialize() SetChannel", this->name), gr::Error(detail::getErrorMessage(status)));
            }
        }

        // configure digital ports
        if constexpr (TPSImpl::N_DIGITAL_CHANNELS > 0) {
            if (const auto status = _picoscope->setDigitalPorts(digital_port_threshold); status != PICO_OK) {
                this->emitErrorMessage(std::format("{}::initialize() setDigitalPorts", this->name), gr::Error(detail::getErrorMessage(status)));
            }
        }

        // configure memory segments and number of captures for the RapidBlock mode
        int32_t maxSamples = detail::kDriverBufferSize;
        if constexpr (acquisitionMode == AcquisitionMode::RapidBlock) {
            if (const auto status = _picoscope->memorySegments(static_cast<uint32_t>(n_captures), &maxSamples); status != PICO_OK) {
                this->emitErrorMessage(std::format("{}::initialize() MemorySegments", this->name), gr::Error(detail::getErrorMessage(status)));
            }

            if (const auto status = _picoscope->setNoOfCaptures(static_cast<uint32_t>(n_captures)); status != PICO_OK) {
                this->emitErrorMessage(std::format("{}::initialize() SetNoOfCaptures", this->name), gr::Error(detail::getErrorMessage(status)));
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

        if constexpr (acquisitionMode == AcquisitionMode::RapidBlock) {
            for (const auto& [idx, channel]: std::views::zip(std::views::iota(0), _channels)) { // clang does not yet support std::views::enumerate
                auto bufferSize = std::min(static_cast<int32_t>(maxSamples), static_cast<int32_t>(channel.driverBuffer.size() / n_captures));
                for (uint32_t segmentIdx = 0; segmentIdx < n_captures; segmentIdx++) {
                    if (const auto status = _picoscope->setDataBufferForSegment(TPSImpl::convertToChannel(channel.id).value(), channel.driverBuffer.data() + (bufferSize * segmentIdx), bufferSize, segmentIdx, _picoscope->ratioNone()); status != PICO_OK) {
                        this->emitErrorMessage(std::format("{}::initialize() setDataBufferForSegment (analog)", this->name), gr::Error(detail::getErrorMessage(status)));
                    }
                }
            }
            if constexpr (TPSImpl::N_DIGITAL_CHANNELS > 0) {
                for (int i = 0; i < TPSImpl::N_DIGITAL_CHANNELS; i++) {
                    _digitalBuffers[i].resize(detail::kDriverBufferSize);
                    auto bufferSize = static_cast<int32_t>(detail::kDriverBufferSize) / n_captures;
                    // this is a bit annoying because the PS3000 defines the Enum for the digital Ports as a different type
                    for (uint32_t segmentIdx = 0; segmentIdx < n_captures; segmentIdx++) {
                        if (const auto status = _picoscope->setDataBufferForSegment(static_cast<TPSImpl::ChannelType>(TPSImpl::DIGI_PORT_0 + i), _digitalBuffers[i].data(), bufferSize, segmentIdx, _picoscope->ratioNone()); status != PICO_OK) {
                            this->emitErrorMessage(std::format("{}::initialize() setDataBufferForSegment (digital)", this->name), gr::Error(detail::getErrorMessage(status)));
                        }
                    }
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

        std::println("PS - arm");
        if constexpr (acquisitionMode == AcquisitionMode::RapidBlock) {
            if (const auto ec = setBuffers(pre_samples + post_samples, n_captures); ec) {
                this->emitErrorMessage(std::format("{}::arm() setBuffers", this->name), ec.message());
                std::format("{}::processBulk setBuffers, {}", this->name, ec.message());
            }
            std::uint32_t acquiredSamples = pre_samples + post_samples;
            const auto timebaseRes = _picoscope->convertSampleRateToTimebase(sample_rate);
            _actualSampleRate      = timebaseRes.actualFreq;
            static auto    redirector = [](int16_t, PICO_STATUS status, void* vobj) { static_cast<decltype(this)>(vobj)->rapidBlockCallback({status}); };
            int32_t        timeIndisposedMs;
            const uint32_t segmentIndex = 0; // only one segment for streaming mode
            if (const auto status = _picoscope->runBlock(static_cast<int32_t>(pre_samples), static_cast<int32_t>(post_samples), timebaseRes.timebase, &timeIndisposedMs, segmentIndex, static_cast<TPSImpl::BlockReadyType>(redirector), this); status != PICO_OK) {
                this->emitErrorMessage(std::format("{}::arm() RunBlock", this->name), gr::Error(detail::getErrorMessage(status)));
                std::format("{}::processBulk runBlock, {}", this->name, detail::getErrorMessage(status));
                return;
            }
            _isArmed = true;
        } else { // Streaming
            using fair::picoscope::detail::kDriverBufferSize;
            if (const auto ec = setBuffers(kDriverBufferSize); ec) {
                this->emitErrorMessage(std::format("{}::arm() setBuffers", this->name), ec.message());
            }

            TimeInterval timeInterval = detail::convertSampleRateToTimeInterval(sample_rate);

            const auto status = _picoscope->runStreaming(        //
                &timeInterval.interval,                          // in: desired interval, out: actual interval
                _picoscope->convertTimeUnits(timeInterval.unit), // time unit of the interval
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

    void rapidBlockCallback(Error /*ec*/) requires(acquisitionMode == AcquisitionMode::RapidBlock) { this->progress->incrementAndGet(); } // wake-up block processing

    constexpr T createDataset(detail::Channel& channel, std::size_t nSamples)
    requires(acquisitionMode == AcquisitionMode::RapidBlock)
    {
        using TSample = T::value_type;
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
                return static_cast<std::int16_t>(i++ - pre); // todo: alternative would be to multiply by 1e9 and export as nanoseconds instead of index
            } else {
                static_assert(false, "unsupported sample type");
            }
        });

        ds.meta_information.resize(1);
        ds.meta_information[0] = channel.toTagMap(sample_rate);

        return ds;
    }

    constexpr TDigitalOutput createDatasetDigital(std::size_t nSamples)
    requires(TPSImpl::N_DIGITAL_CHANNELS > 0 && acquisitionMode == AcquisitionMode::RapidBlock)
    {
        TDigitalOutput ds{};
        ds.extents = {static_cast<int32_t>(nSamples)};
        ds.layout  = gr::LayoutRight{};

        ds.signal_values.resize(nSamples);
        ds.signal_ranges.resize(1);
        ds.timing_events.resize(1);
        // todo: add time axis data

        return ds;
    }

    template <typename TSample>
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

    [[nodiscard]] Error setBuffers(size_t nSamples, uint32_t nSegments= 1U) {
        for (auto& channel : _channels) {
            const auto channelEnum = _picoscope->convertToChannel(channel.id);
            if (!channelEnum) {
                return {PICO_INVALID_CHANNEL};
            }

            if constexpr (acquisitionMode == AcquisitionMode::Streaming) {
                channel.driverBuffer.resize(nSamples);
                const auto status = _picoscope->setDataBuffer(*channelEnum, channel.driverBuffer.data(), static_cast<int32_t>(nSamples), _picoscope->ratioNone());
                if (status != PICO_OK) {
                    return {status};
                }
            } else { // rapid block
                channel.driverBuffer.resize(std::max(nSamples * nSegments, channel.driverBuffer.size()));
                for (std::uint32_t segmentIndex = 0; segmentIndex < nSegments; segmentIndex++) {
                    const auto status = _picoscope->setDataBufferForSegment(*channelEnum, channel.driverBuffer.data() + (segmentIndex * nSamples), static_cast<int32_t>(nSamples), segmentIndex, _picoscope->ratioNone());
                    if (status != PICO_OK) {
                        return {status};
                    }
                }
            }
        }

        if constexpr (TPSImpl::N_DIGITAL_CHANNELS > 0) {
            if constexpr (acquisitionMode == AcquisitionMode::Streaming) {
                for (std::size_t i = 0; i < TPSImpl::N_DIGITAL_CHANNELS; i++) {
                    _digitalBuffers[i].resize(nSamples);
                    // this is a bit annoying because the PS3000 defines the Enum for the digital Ports as a different type
                    if (const auto status = _picoscope->setDataBuffer(static_cast<TPSImpl::ChannelType>(TPSImpl::DIGI_PORT_0 + i), _digitalBuffers[i].data(), static_cast<int32_t>(nSamples), _picoscope->ratioNone()); status != PICO_OK) {
                        return {status};
                    }
                }
            } else {
                for (std::size_t i = 0; i < TPSImpl::N_DIGITAL_CHANNELS; i++) {
                    _digitalBuffers[i].resize(nSamples * nSegments);
                    for (std::uint32_t segmentIndex = 0; segmentIndex < nSegments; segmentIndex++) {
                        // this is a bit annoying because the PS3000 defines the Enum for the digital Ports as a different type
                        if (const auto status = _picoscope->setDataBufferForSegment(static_cast<TPSImpl::ChannelType>(TPSImpl::DIGI_PORT_0 + i), _digitalBuffers[i].data() + (i * nSamples), static_cast<int32_t>(nSamples), segmentIndex, _picoscope->ratioNone()); status != PICO_OK) {
                            return {status};
                        }
                    }
                }

            }
        }

        return {};
    }

    [[nodiscard]] std::string getUnitInfoTopic(PICO_INFO info) const {
        std::array<int8_t, 40> line{};
        int16_t                requiredSize;

        auto status = _picoscope->getUnitInfo(line.data(), line.size(), &requiredSize, info);
        if (status == PICO_OK) {
            return {reinterpret_cast<char*>(line.data()), static_cast<std::size_t>(requiredSize - 1)};
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
