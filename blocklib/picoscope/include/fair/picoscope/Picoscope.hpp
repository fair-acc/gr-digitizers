#ifndef FAIR_PICOSCOPE_PICOSCOPE_HPP
#define FAIR_PICOSCOPE_PICOSCOPE_HPP

#include <fair/picoscope/PicoscopeAPI.hpp>

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/HistoryBuffer.hpp>
#include <gnuradio-4.0/algorithm/dataset/DataSetUtils.hpp>

#include <format>

#include <chrono>
#include <string_view>

namespace fair::picoscope {
using namespace std::literals;

namespace detail {

[[nodiscard]] static bool isDigitalTrigger(const std::string_view source) { return !source.empty() && source.starts_with("DI"); }
[[nodiscard]] static bool isAnalogTrigger(const std::string_view source) { return !source.empty() && !source.starts_with("DI"); }

[[nodiscard]] static std::expected<uint, gr::Error> parseDigitalTriggerSource(std::string_view triggerSrc) {
    if (!triggerSrc.starts_with("DI")) {
        return std::unexpected(gr::Error(std::format("Cannot parse digital trigger source (`{}`): it must start with `DI`.", triggerSrc)));
    }
    const std::string_view numberPart = triggerSrc.substr(2);
    if (numberPart.empty()) {
        return std::unexpected(gr::Error(std::format("Cannot parse digital trigger source (`{}`): No digital channel number.", triggerSrc)));
    }
    int value = -1;
    if (const auto [_, ec] = std::from_chars(numberPart.data(), numberPart.data() + numberPart.size(), value); ec != std::errc()) {
        return std::unexpected(gr::Error(std::format("Cannot parse digital trigger source (`{}`): Error parsing digital channel number.", triggerSrc)));
    }
    if (value < 0 || value > 15) {
        return std::unexpected(gr::Error(std::format("Cannot parse digital trigger source (`{}`): channel number is out of range [0, 15].", triggerSrc)));
    }
    return static_cast<uint>(value);
}

struct TriggerNameAndCtx {
    std::string                triggerName{};
    std::optional<std::string> ctx{};
};

[[nodiscard]] static TriggerNameAndCtx createTriggerNameAndCtx(const std::string& triggerNameAndCtx) {
    if (triggerNameAndCtx.empty()) {
        return {};
    }
    if (const std::size_t pos = triggerNameAndCtx.find('/'); pos != std::string::npos) { // trigger_name and ctx
        return {triggerNameAndCtx.substr(0, pos), (pos < triggerNameAndCtx.size() - 1) ? triggerNameAndCtx.substr(pos + 1) : ""};
    }
    return {triggerNameAndCtx, std::nullopt}; // only trigger_name
}

[[nodiscard, maybe_unused]] static bool tagContainsTrigger(const gr::property_map& map, const TriggerNameAndCtx& triggerNameAndCtx) { // unused if only streaming acq is used
    if (triggerNameAndCtx.ctx) {                                                                                                      // trigger_name and ctx
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
[[nodiscard]] constexpr TEnum convertToEnum(std::string_view strEnum) {
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
    using SuperT                                     = gr::Block<Picoscope, gr::SupportedTypes<int16_t, float, gr::UncertainValue<float>, gr::DataSet<int16_t>, gr::DataSet<float>, gr::DataSet<gr::UncertainValue<float>>>>;
    static constexpr AcquisitionMode acquisitionMode = gr::DataSetLike<T> ? AcquisitionMode::RapidBlock : AcquisitionMode::Streaming;

    A<std::string, "serial number, empty selects first available device">            serial_number;
    A<float, "sample rate", gr::Visible>                                             sample_rate  = 10000.f;
    A<gr::Size_t, "pre-samples">                                                     pre_samples  = 1000;  // RapidBlock mode only
    A<gr::Size_t, "post-samples">                                                    post_samples = 1000;  // RapidBlock mode only
    A<gr::Size_t, "no. captures (rapid block mode)">                                 n_captures   = 1;     // RapidBlock mode only
    A<bool, "trigger once (rapid block mode)">                                       trigger_once = false; // RapidBlock mode only
    A<bool, "do arm at start?">                                                      auto_arm     = true;
    A<std::vector<std::string>, "IDs of enabled channels: `A`, `B`, `C` etc.">       channel_ids;
    A<std::vector<float>, "Voltage range of enabled channels">                       channel_ranges;         // PS channel setting
    A<std::vector<float>, "Voltage offset of enabled channels">                      channel_analog_offsets; // PS channel setting
    A<std::vector<std::string>, "Coupling modes of enabled channels">                channel_couplings;
    A<std::vector<std::string>, "Signal names of enabled channels">                  signal_names;
    A<std::vector<std::string>, "Signal units of enabled channels">                  signal_units;
    A<std::vector<std::string>, "Signal quantity of enabled channels">               signal_quantities;
    A<std::vector<float>, "Signal scales of the enabled channels">                   signal_scales;  // only for floats and UncertainValues
    A<std::vector<float>, "Analog offsets of the channels">                          signal_offsets; // only for floats and UncertainValues
    A<std::string, "trigger channel (A, B, C, ... or DI1, DI2, DI3, ... EXTERNAL)">  trigger_source;
    A<float, "trigger threshold, analog only">                                       trigger_threshold          = 0.f;
    A<TriggerDirection, "trigger direction">                                         trigger_direction          = TriggerDirection::Rising;
    A<std::string, "trigger filter: `<trigger_name>[/<ctx>]`">                       trigger_filter             = "";
    A<std::string, "arm trigger: `<trigger_name>[/<ctx>]`, if empty not used">       trigger_arm                = "";    // RapidBlock mode only
    A<std::string, "disarm trigger: `<trigger_name>[/<ctx>]`, if empty not used">    trigger_disarm             = "";    // RapidBlock mode only
    A<bool, "Enable digital inputs">                                                 digital_port_enable        = false; // only used if digital ports are available: 3000a, 5000a series
    A<bool, "invert digital port output">                                            digital_port_invert_output = false; // only used if digital ports are available: 3000a, 5000a series
    A<gr::Size_t, "Timeout after which to not match trigger pulses", gr::Unit<"ns">> matcher_timeout            = 10'000'000z;
    A<bool, "verbose console">                                                       verbose_console            = false;

    gr::PortIn<std::uint8_t, gr::Async> timingIn;

    std::array<gr::PortOut<T>, TPSImpl::N_ANALOG_CHANNELS> out;

    // only used for ports with digital ports (3000a + 5000a)
    using TDigitalOutput = std::conditional_t<gr::DataSetLike<T>, gr::DataSet<uint16_t>, uint16_t>;
    gr::PortOut<TDigitalOutput> digitalOut;

    float       _actualSampleRate  = 0; // todo: find a way to properly update this property and make it reflectable
    std::size_t _nSamplesPublished = 0; // for debugging purposes

    detail::TriggerNameAndCtx _armTriggerNameAndCtx; // store parsed information to optimise performance
    detail::TriggerNameAndCtx _disarmTriggerNameAndCtx;

    GR_MAKE_REFLECTABLE(Picoscope, timingIn, out, digitalOut, serial_number, sample_rate, pre_samples, post_samples, n_captures, auto_arm, trigger_once, channel_ids, signal_names, signal_units, signal_quantities, //
        channel_ranges, channel_analog_offsets, signal_scales, signal_offsets, channel_couplings, trigger_source, trigger_threshold, trigger_direction, digital_port_enable, digital_port_invert_output, trigger_arm, trigger_disarm, matcher_timeout, verbose_console);

private:
    std::optional<PicoscopeWrapper<TPSImpl>> _picoscope;

    std::array<bool, TPSImpl::N_ANALOG_CHANNELS> signalInfoTagPublished{false};
    bool                                         _isArmed = false;                                     // for RapidBlock mode only
    std::vector<gr::property_map>                _currentTimingTags{};                                 // contains all tag maps for the currently running acquisition
    std::vector<gr::property_map>                _nextTimingTags{};                                    // contains all tag maps for the next acquisition (this is needed if a disarm+arm sequence interrupts a running acquisition)
    std::int16_t                                 _maxValue = std::numeric_limits<std::int16_t>::max(); // maximum ADC count used for ADC conversion // todo:: update from API call

    std::size_t unpublishedSamples = 0; // The number of unpublished samples already written to the output buffer but not published. Since streaming ports are single producer, we can publish these on the next iteration.

    TTagMatcher tagMatcher{.timeout = std::chrono::nanoseconds(matcher_timeout.value), .sampleRate = sample_rate.value};

public:
    ~Picoscope() { stop(); }
    using SuperT::SuperT; // inherit Block constructor

    template<gr::OutputSpanLike TOutSpan>
    requires(acquisitionMode == AcquisitionMode::Streaming)
    gr::work::Status processBulk(gr::InputSpanLike auto& timingInSpan, std::span<TOutSpan>& outputs, gr::OutputSpanLike auto& digitalOutSpan) {
        std::size_t       nSamples        = 0UZ;
        std::size_t       samplesDropped  = 0UZ;
        const std::size_t availableBuffer = std::min(std::ranges::min(outputs | std::views::transform(&TOutSpan::size)), digitalOutSpan.size());
        // Make acq time a member field and only update if there is new data?
        std::chrono::system_clock::time_point acquisitionTime = std::chrono::system_clock::now();
        const auto                            pollResult      = _picoscope->poll([&](std::span<std::span<const std::int16_t>> data, std::int16_t overflow) {
            nSamples = data[0].size();
            if (verbose_console) {
                const auto  thisAcquisitionTime = std::chrono::high_resolution_clock::now();
                static auto lastAcquisitionTime = thisAcquisitionTime;
                std::println("Streaming Update: {}; {}; {}; {}; {}", std::chrono::duration_cast<std::chrono::nanoseconds>(thisAcquisitionTime.time_since_epoch()).count(), nSamples, static_cast<float>(nSamples) * 1e9f / sample_rate.value, std::chrono::duration_cast<std::chrono::nanoseconds>(thisAcquisitionTime - lastAcquisitionTime).count(), std::this_thread::get_id());
                lastAcquisitionTime = thisAcquisitionTime;
            }
            for (const auto& [channelIdx, channelId, output] : std::views::zip(std::views::iota(0UZ), channel_ids.value, outputs)) {
                // copy and publish all analog channel data
                const float voltageMultiplier = channel_ranges[channelIdx] / static_cast<float>(_maxValue);
                if (nSamples + unpublishedSamples > availableBuffer) {
                    samplesDropped = nSamples + unpublishedSamples - availableBuffer;
                    nSamples       = availableBuffer - unpublishedSamples; // we don't want to publish more data than the output buffer can hold
                }
                const auto offset = getChannelSetting(std::span(channel_analog_offsets.value), channelIdx, 0.0f);
                const auto scale  = getChannelSetting(std::span(signal_scales.value), channelIdx, 1.0f);
                for (std::size_t i = 0; i < nSamples; ++i) {
                    assert(i + unpublishedSamples < availableBuffer);
                    if constexpr (std::is_same_v<T, float>) {
                        output[i + unpublishedSamples] = offset + scale * voltageMultiplier * static_cast<float>(data[channelIdx][i]);
                    } else if constexpr (std::is_same_v<T, gr::UncertainValue<float>>) {
                        output[i + unpublishedSamples] = gr::UncertainValue(offset + scale * voltageMultiplier * static_cast<float>(data[channelIdx][i]), TPSImpl::uncertainty());
                    } else if constexpr (std::is_same_v<T, int16_t>) {
                        output[i + unpublishedSamples] = data[channelIdx][i]; // std::ranges::copy(driverData, output.begin() + static_cast<std::ptrdiff_t>(channel.unpublished.size()));
                    }
                }
                if (overflow & (1 << channelIdx)) {                               // picoscope overrange
                    output.publishTag(gr::property_map{{"over-range", true}}, 0); // todo: correct tag
                }
            }
            if constexpr (TPSImpl::N_DIGITAL_CHANNELS > 0) {
                for (std::size_t i = 0; i < nSamples; ++i) {
                    assert(i + unpublishedSamples < digitalOutSpan.size());
                    digitalOutSpan[i + unpublishedSamples] = static_cast<std::uint16_t>(data.back()[i]);
                }
            }
        });
        const auto                            acqStartTime    = acquisitionTime - std::chrono::nanoseconds(static_cast<long>(1e9f / sample_rate * static_cast<float>(nSamples)));
        acquisitionTime                                       = acquisitionTime - std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::nanoseconds(static_cast<long>(1e9f / sample_rate * static_cast<float>(nSamples + unpublishedSamples))));
        if (!pollResult) {
            if (verbose_console) {
                std::println("Error polling: {}@{}L{}", pollResult.error().getDescription(), pollResult.error().location.file_name(), pollResult.error().location.line());
            }
            // TODO: forward error to scheduler and stop the block
        }
        if (nSamples + unpublishedSamples == 0) {
            for (auto& output : outputs) {
                output.publish(0);
            }
            digitalOutSpan.publish(0);
            return gr::work::Status::INSUFFICIENT_INPUT_ITEMS; // no new data to be processed
        }
        // find triggers and match
        auto tagsDriver         = timingInSpan.rawTags | std::views::transform([](const auto& t) { return t.map; }) | std::ranges::to<std::vector<gr::property_map>>();
        auto triggerEdgesDriver = [&]() -> std::vector<std::size_t> {
            if (trigger_source == "") { // no trigger configured
                return {};
            } else if (detail::isAnalogTrigger(trigger_source)) {
                for (const auto& channelIdx : std::views::iota(0UZ, channel_ids->size())) {
                    if (channel_ids[channelIdx] == trigger_source) {
                        assert(outputs[channelIdx].size() >= unpublishedSamples + nSamples);
                        auto result = findAnalogTriggers(channelIdx, std::span(outputs[channelIdx]).subspan(0UZ, unpublishedSamples + nSamples));
                        return result;
                    }
                }
                return {}; // the trigger is not one of the enabled channels
            } else if (const auto digitalTriggerBit = detail::parseDigitalTriggerSource(trigger_source); digitalTriggerBit && static_cast<std::size_t>(digitalTriggerBit.value()) < TPSImpl::N_DIGITAL_CHANNELS * 8) {
                if constexpr (TPSImpl::N_DIGITAL_CHANNELS > 0) {
                    const auto result = findDigitalTriggers(digitalTriggerBit.value(), std::span(digitalOutSpan).subspan(0UZ, unpublishedSamples + nSamples));
                    return result;
                }
                throw gr::exception(std::format("This picoscope model does not support digital triggers: {}", trigger_source.value));
            } else {
                throw gr::exception(std::format("Invalid Trigger Source configured: {}", trigger_source.value));
            }
        }();
        auto matchedTags = tagMatcher.match(tagsDriver, triggerEdgesDriver, unpublishedSamples + nSamples, acquisitionTime.time_since_epoch());
        if (samplesDropped > 0UZ) {
            tagMatcher.reset(); // reset the tag matcher whenever we drop samples
        }

        for (const auto& [channelIdx, output] : std::views::zip(std::views::iota(0UZ), outputs)) {
            if (!signalInfoTagPublished[channelIdx] && matchedTags.processedSamples > 0) {
                output.publishTag(channelToTagMap(channelIdx, sample_rate), 0);
                signalInfoTagPublished[channelIdx] = true;
            }
            bool chunkStartPublished = false;
            for (auto& [index, map] : matchedTags.tags) {
                if (verbose_console && !chunkStartPublished && index > unpublishedSamples && nSamples > 0 && matchedTags.processedSamples > unpublishedSamples) {
                    output.publishTag(gr::property_map{{"chunk-start-time", (std::chrono::duration_cast<std::chrono::nanoseconds>(acqStartTime.time_since_epoch()).count())}}, unpublishedSamples);
                    chunkStartPublished = true;
                }
                output.publishTag(map, index);
            }
            if (verbose_console && !chunkStartPublished && nSamples > 0 && matchedTags.processedSamples > unpublishedSamples) {
                output.publishTag(gr::property_map{{"chunk-start-time", (std::chrono::duration_cast<std::chrono::nanoseconds>(acqStartTime.time_since_epoch()).count())}}, unpublishedSamples);
            }
            if (samplesDropped > 0UZ) {
                output.publishTag(gr::property_map{{"droppedSamples", samplesDropped}}, unpublishedSamples + nSamples); // todo: correct tag
            }
            output.publish(matchedTags.processedSamples);
        }
        if (samplesDropped > 0UZ) {
            for (auto& [index, map] : matchedTags.tags) {
                digitalOutSpan.publishTag(map, index);
            }
            digitalOutSpan.publishTag(gr::property_map{{"droppedSamples", samplesDropped}}, unpublishedSamples + nSamples); // todo: correct tag
        }
        for (auto& [index, map] : matchedTags.tags) {
            digitalOutSpan.publishTag(map, index);
        }
        digitalOutSpan.publish(matchedTags.processedSamples);
        _nSamplesPublished += matchedTags.processedSamples;
        assert(unpublishedSamples + nSamples >= matchedTags.processedSamples);
        unpublishedSamples = unpublishedSamples + nSamples - matchedTags.processedSamples;

        // consume timing tags
        if (matchedTags.processedTags > 0) {
            auto lastTimingSampleIndex = timingInSpan.rawTags[matchedTags.processedTags - 1].index - timingInSpan.streamIndex + 1;
            timingInSpan.consumeTags(lastTimingSampleIndex);
            std::ignore = timingInSpan.consume(lastTimingSampleIndex);
        } else {
            timingInSpan.consumeTags(0);
            std::ignore = timingInSpan.consume(0);
        }
        // emit matcher messages
        for (const auto& msg : matchedTags.messages) {
            // this->emitErrorMessage(std::format("{}::TimingMatcher", this->name), msg); // if we emmit as an error, it will terminate the processing, which is probably not what we want in most cases
            if (verbose_console) {
                std::println("Problem in matcher: {}", msg); // TODO: correctly handle matching messages as soft metainformation instead of hard errors
            }
        }
        return gr::work::Status::OK;
    }

    template<gr::OutputSpanLike TOutSpan>
    requires(acquisitionMode == AcquisitionMode::RapidBlock)
    gr::work::Status processBulk(gr::InputSpanLike auto& timingInSpan, std::span<TOutSpan>& outputs, gr::OutputSpanLike auto& digitalOutSpan) {
        using TSample  = typename T::value_type;
        auto armResult = processTagsTriggered(timingInSpan);
        if (!_isArmed) {
            if (armResult.arm) {
                _picoscope->setPaused(false);
                _isArmed = true;
            }
            // todo: check if we really want to return here or if we need to continue to publish existing data
            for (auto& output : outputs) {
                output.publish(0);
            }
            digitalOutSpan.publish(0);
            return gr::work::Status::OK; // nothing to do here as the triggering is currently disabled
        }
        if (armResult.disarm) {
            _picoscope->setPaused(true);
            _isArmed = false; // todo move inside disarm?
        }
        std::size_t nCaptures  = 0;
        const auto  pollResult = _picoscope->poll([&](const std::span<std::span<const std::int16_t>> data, const std::int16_t overflow) {
            const std::chrono::system_clock::time_point acquisitionTime = std::chrono::system_clock::now();
            for (std::size_t channelIdx = 0; channelIdx < channel_ids.value.size(); channelIdx++) {
                const float voltageMultiplier  = getChannelSetting(std::span(channel_ranges.value), channelIdx, 5.0f) / static_cast<float>(_maxValue);
                const auto  driverData         = data[channelIdx];
                outputs[channelIdx][nCaptures] = createDataset(channelIdx, driverData.size());
                const auto offset              = getChannelSetting(std::span(signal_offsets.value), channelIdx, 0.0f);
                const auto scale               = getChannelSetting(std::span(signal_scales.value), channelIdx, 1.0f);
                for (std::size_t j = 0; j < driverData.size(); ++j) {
                    if constexpr (std::is_same_v<TSample, float>) {
                        outputs[channelIdx][nCaptures].signal_values[j] = offset + scale * voltageMultiplier * static_cast<float>(driverData[j]);
                    } else if constexpr (std::is_same_v<TSample, gr::UncertainValue<float>>) {
                        outputs[channelIdx][nCaptures].signal_values[j] = gr::UncertainValue(offset + scale * voltageMultiplier * static_cast<float>(driverData[j]), TPSImpl::uncertainty());
                    } else if constexpr (std::is_same_v<TSample, int16_t>) {
                        outputs[channelIdx][nCaptures].signal_values[j] = driverData[j]; // std::ranges::copy(driverData, output.begin() + static_cast<std::ptrdiff_t>(channel.unpublished.size()));
                    }
                }
                // add Tags
                if (overflow | (1 << channelIdx)) {                                                  // picoscope overrange
                    outputs[channelIdx][nCaptures].metaInformation(0UZ).insert({"Overrange", true}); // todo: use correct tag string
                }
                if constexpr (std::is_same_v<TSample, float> || std::is_same_v<TSample, std::int16_t>) {
                    gr::dataset::updateMinMax(outputs[channelIdx][nCaptures]);
                } else {
                    // TODO: fix UncertainValue, it requires changes in GR4
                }
            }
            if constexpr (TPSImpl::N_DIGITAL_CHANNELS > 0) {
                digitalOutSpan[nCaptures] = createDatasetDigital(data.back().size());
                for (std::size_t j = 0; j < data.back().size(); ++j) {
                    if (digital_port_invert_output) {
                        digitalOutSpan[nCaptures].signal_values[j] = static_cast<std::uint16_t>(~data.back()[j]);
                    } else {
                        digitalOutSpan[nCaptures].signal_values[j] = static_cast<std::uint16_t>(data.back()[j]);
                    }
                }
            }
            tagMatcher.reset();                                      // reset the tag matcher because for triggered acquisition there is always a gap in the data
            std::vector<std::size_t> triggerOffsets = {pre_samples}; // by default, only give the edge that has actually triggered this acquisition
            // if the trigger channel is also digitised, we can additionally add other edges within the acquisition window.
            if (detail::isDigitalTrigger(trigger_source)) {
                if constexpr (TPSImpl::N_DIGITAL_CHANNELS > 0) {
                    if (const std::expected<uint, gr::Error> digitalPin = detail::parseDigitalTriggerSource(trigger_source); digitalPin) {
                        triggerOffsets = findDigitalTriggers(digitalPin.value(), digitalOutSpan[nCaptures].signalValues(0UZ));
                    } else {
                        throw Error(std::format("Invalid Digital Trigger Source: {}", trigger_source.value));
                    }
                }
            } else {
                for (const auto& [i, channel] : std::views::zip(std::views::iota(0U), channel_ids.value)) {
                    if (channel == trigger_source) {
                        triggerOffsets = findAnalogTriggers(static_cast<std::size_t>(i), outputs[static_cast<std::size_t>(i)][nCaptures].signalValues(0UZ));
                    }
                }
            }
            auto triggerTags   = tagMatcher.match(_currentTimingTags, triggerOffsets, pre_samples + post_samples, acquisitionTime.time_since_epoch() - std::chrono::nanoseconds(static_cast<long>(1e9f * static_cast<float>(pre_samples + post_samples) / sample_rate)));
            _currentTimingTags = std::move(_nextTimingTags);
            _nextTimingTags.clear();
            if (!triggerTags.tags.empty()) {
                for (std::size_t channelIdx = 0; channelIdx < channel_ids.value.size(); ++channelIdx) {
                    auto& output = outputs[channelIdx][nCaptures];
                    for (auto& [index, map] : triggerTags.tags) {
                        if (static_cast<std::ptrdiff_t>(index) >= 0u) {
                            output.timing_events[0].push_back({static_cast<std::ptrdiff_t>(index), map});
                        }
                    }
                }
                if constexpr (TPSImpl::N_DIGITAL_CHANNELS > 0) {
                    for (auto& [index, map] : triggerTags.tags) {
                        if (static_cast<std::ptrdiff_t>(index) >= 0u) {
                            digitalOutSpan[nCaptures].timing_events[0].push_back({static_cast<std::ptrdiff_t>(index), map});
                        }
                    }
                }
                if (verbose_console) {
                    for (const auto& msg : triggerTags.messages) {
                        // this->emitErrorMessage(std::format("{}::TimingMatcher", this->name), msg);
                        std::println("Problem in matcher: {}", msg); // TODO: correctly handle matching messages as soft metainformation instead of hard errors
                    }
                }
            }
            nCaptures++;
        });
        if (!pollResult) {
            if (verbose_console) {
                std::println("Error polling: {}@{}L{}", pollResult.error().getDescription(), pollResult.error().location.file_name(), pollResult.error().location.line());
            }
            // TODO: forward error to scheduler and stop the block
        }

        if (nCaptures == 0) { // no new data to publish
            // todo: consume old timing tags while there are no updates happening
            for (auto& output : outputs) {
                output.publish(0);
            }
            digitalOutSpan.publish(0);
            return gr::work::Status::INSUFFICIENT_INPUT_ITEMS;
        }

        // publish
        for (std::size_t i = 0; i < outputs.size(); i++) {
            outputs[i].publish(nCaptures);
        }
        digitalOutSpan.publish(nCaptures);

        if (trigger_once) {
            _picoscope->stopAcquisition();
            return gr::work::Status::DONE;
        }
        return gr::work::Status::OK;
    }

    auto processTagsTriggered(gr::InputSpanLike auto& tagData) {
        struct tagProcessResult {
            bool arm    = false;
            bool disarm = false;
        } result;
        bool armed = _isArmed;
        for (const auto& [i, tag_map] : tagData.tags()) {
            if (!trigger_arm.value.empty() || !trigger_disarm.value.empty()) {
                if (detail::tagContainsTrigger(tag_map, _armTriggerNameAndCtx)) {
                    result.arm = true;
                    armed      = true;
                    _nextTimingTags.clear();
                }
                if (detail::tagContainsTrigger(tag_map, _disarmTriggerNameAndCtx)) {
                    result.disarm = true;
                    armed         = false;
                }
            }
            if (armed) {
                if (!result.disarm) {
                    _currentTimingTags.push_back(tag_map);
                } else {
                    _nextTimingTags.push_back(tag_map);
                }
            }
        }
        std::ignore = tagData.consume(tagData.size());
        std::ignore = tagData.rawTags.consume(tagData.rawTags.size()); // this should be the default, but there seem to be some cases where not all tags are consumed
        return result;
    }

    std::optional<std::size_t> findChannelSettingIndex(const ChannelName channelName) {
        if (const auto it = std::ranges::find(channel_ids.value, magic_enum::enum_name(channelName)); it == std::ranges::end(channel_ids.value)) {
            return std::nullopt;
        } else {
            return std::ranges::distance(std::ranges::begin(channel_ids.value), it);
        }
    }

    [[nodiscard]] gr::property_map channelToTagMap(std::size_t channelIdx, float sampleRate) {
        const float offset = getChannelSetting(std::span(signal_offsets.value), channelIdx, 0.0f);
        const float range  = getChannelSetting(std::span(channel_ranges.value), channelIdx, 0.0f);
        return {
            {std::string(gr::tag::SIGNAL_NAME.shortKey()), getChannelSetting(std::span(signal_names.value), channelIdx, ""s)},
            {std::string(gr::tag::SAMPLE_RATE.shortKey()), sampleRate},
            {std::string(gr::tag::SIGNAL_QUANTITY.shortKey()), getChannelSetting(std::span(signal_quantities.value), channelIdx, ""s)},
            {std::string(gr::tag::SIGNAL_UNIT.shortKey()), getChannelSetting(std::span(signal_units.value), channelIdx, ""s)},
            {std::string(gr::tag::SIGNAL_MIN.shortKey()), offset - range},
            {std::string(gr::tag::SIGNAL_MAX.shortKey()), offset + range},
        };
    }

    template<typename P>
    static P getChannelSetting(const std::span<P> settingsArray, const std::size_t i, const P& defaultValue) {
        if (settingsArray.size() > i) {
            return settingsArray[i];
        } else if (settingsArray.size() == 1UZ) {
            return settingsArray[0];
        } else {
            return defaultValue;
        }
    }

    void settingsChanged(const gr::property_map& oldSettings, const gr::property_map& newSettings) {
        tagMatcher.sampleRate = sample_rate;
        tagMatcher.timeout    = std::chrono::nanoseconds(matcher_timeout);
        if (!_picoscope || (oldSettings.contains("serial_number") && serial_number != std::get<std::string>(oldSettings.at("serial_number")))) {
            _picoscope.emplace(serial_number, verbose_console);
        }
        std::set<std::size_t> configuredSuccessfully{};
        for (const auto& [i, channelName] : std::views::zip(std::views::iota(0UZ), _picoscope->getChannelIds())) {
            ChannelConfig channelConfig = _picoscope->getChannelConfig(i);
            if (auto j = findChannelSettingIndex(channelName)) {
                configuredSuccessfully.insert(*j);
                channelConfig.enable   = true;
                channelConfig.range    = toAnalogChannelRange(getChannelSetting(std::span(channel_ranges.value), *j, 5.0f)).value_or(AnalogChannelRange::ps5V);
                channelConfig.offset   = getChannelSetting(std::span(channel_analog_offsets.value), *j, 0.0f);
                channelConfig.coupling = detail::convertToEnum<Coupling>(getChannelSetting(std::span(channel_couplings.value), *j, "DC"s));
            } else {
                channelConfig.enable = false;
            }
            _picoscope->configureChannel(i, channelConfig);
        }
        for (std::size_t i = 0; i < channel_ids.value.size(); i++) {
            if (!configuredSuccessfully.contains(i)) {
                throw Error(std::format("Invalid channel name: {}", channel_ids.value[i]));
            }
        }

        if (newSettings.contains("trigger_arm") || newSettings.contains("trigger_disarm")) {
            _armTriggerNameAndCtx    = detail::createTriggerNameAndCtx(trigger_arm);
            _disarmTriggerNameAndCtx = detail::createTriggerNameAndCtx(trigger_disarm);

            if (!trigger_arm.value.empty() && !trigger_disarm.value.empty() && trigger_arm == trigger_disarm) {
                this->emitErrorMessage(std::format("{}::settingsChanged()", this->name), gr::Error("Ill-formed settings: `trigger_arm` == `trigger_disarm`"));
            }
        }

        if (newSettings.contains("trigger_source")) {
            if (detail::isDigitalTrigger(trigger_source)) {
                if (const auto parseRes = detail::parseDigitalTriggerSource(trigger_source); parseRes.has_value()) {
                    TriggerConfig trigger{.source = parseRes.value(), .direction = trigger_direction, .threshold = static_cast<std::int16_t>(trigger_threshold * 5.0f * 32767.0f), .delay = 0U, .auto_trigger_ms = 0U};
                    if (verbose_console) {
                        std::println("set trigger source: {}, treshold={} ({})", trigger_source, trigger.threshold, trigger_threshold);
                    }
                    _picoscope->configureTrigger(trigger);
                } else {
                    this->emitErrorMessage(std::format("{}::settingsChanged()", this->name), parseRes.error());
                }
            } else if (detail::isAnalogTrigger(trigger_source)) {
                if (const auto parseRes = magic_enum::enum_cast<ChannelName>(trigger_source); parseRes.has_value()) {
                    TriggerConfig trigger{.source = parseRes.value(), .direction = trigger_direction, .threshold = static_cast<std::int16_t>(trigger_threshold * 5.0f * 32767.0f), .delay = 0U, .auto_trigger_ms = 0U};
                    if (verbose_console) {
                        std::println("set trigger source: {}, threshold={} ({})", trigger_source, trigger.threshold, trigger_threshold);
                    }
                    _picoscope->configureTrigger(trigger);
                } else {
                    this->emitErrorMessage(std::format("{}::settingsChanged()", this->name), gr::Error(std::format("Invalid trigger source: {}", trigger_source)));
                }
            } else if (trigger_source.value.empty()) {
                if (const auto parseRes = detail::parseDigitalTriggerSource(trigger_source); parseRes.has_value()) {
                    TriggerConfig trigger{.source = {}, .direction = trigger_direction, .threshold = static_cast<std::int16_t>(trigger_threshold * 5.0f * 32767.0f), .delay = 0U, .auto_trigger_ms = 0U};
                    if (verbose_console) {
                        std::println("clear trigger source");
                    }
                    _picoscope->configureTrigger(trigger);
                } else {
                    this->emitErrorMessage(std::format("{}::settingsChanged()", this->name), parseRes.error());
                }
            } else {
                this->emitErrorMessage(std::format("{}::settingsChanged()", this->name), gr::Error("Unsupported trigger name"));
            }
        }
    }

    void start() {
        if (acquisitionMode == AcquisitionMode::Streaming) { // Warn users if they set up streaming acquisition with small buffer sizes
            // since there is no internal buffering, if the picoscope driver provides bigger chunks than the output buffers, it will have to drop samples.
            for (const auto& [i, port] : std::views::zip(std::views::iota(0U), out)) {
                if (port.bufferSize() < 10000) {
                    this->emitErrorMessage(std::format("{}::start()", this->name), gr::Error(std::format("Buffer size seems very small for streaming acquisition: port out[{}], {} samples", i, port.bufferSize())));
                }
            }
            if (digitalOut.bufferSize() < 10000) {
                this->emitErrorMessage(std::format("{}::start()", this->name), gr::Error(std::format("Buffer size seems very small for streaming acquisition: port digitalOut, {} samples", digitalOut.bufferSize())));
            }
        }
        initialize();
        tagMatcher.reset();
        _picoscope->poll();
    }

    void stop() {
        _picoscope->stopAcquisition();
        _picoscope = std::nullopt;
    }

    void initialize() {
        if (!_picoscope) {
            _picoscope.emplace(serial_number, verbose_console);
        }
        if constexpr (acquisitionMode == AcquisitionMode::Streaming) {
            _picoscope->startStreamingAcquisition(sample_rate, digital_port_enable || detail::isDigitalTrigger(trigger_source));
        } else {
            _picoscope->startTriggeredAcquisition(
                sample_rate, pre_samples, post_samples, n_captures,
                [&]() {
                    if (verbose_console) {
                        std::println("Acquisition finished");
                    }
                    this->progress->incrementAndGet();
                },
                digital_port_enable || detail::isDigitalTrigger(trigger_source));
        }
        if (auto_arm) {
            _picoscope->setPaused(false);
            _isArmed = true;
        }
    }

    constexpr T createDataset(const std::size_t channelIdx, std::size_t nSamples)
    requires(acquisitionMode == AcquisitionMode::RapidBlock)
    {
        using TSample = typename T::value_type;
        T ds{};
        ds.timestamp = 0;

        ds.axis_names = {getChannelSetting(std::span(signal_names.value), channelIdx, ""s)};
        ds.axis_units = {getChannelSetting(std::span(signal_units.value), channelIdx, ""s)};

        ds.extents           = {static_cast<int32_t>(nSamples)};
        ds.layout            = gr::LayoutRight{};
        ds.signal_names      = {getChannelSetting(std::span(signal_names.value), channelIdx, ""s)};
        ds.signal_units      = {getChannelSetting(std::span(signal_units.value), channelIdx, ""s)};
        ds.signal_quantities = {getChannelSetting(std::span(signal_quantities.value), channelIdx, ""s)};

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
                return static_cast<std::int16_t>(i++ - pre);
                // return (i - pre) * static_cast<std::int16_t>(samplePeriod * 1e9); // alternatively give nanoseconds instead of index
            } else {
                static_assert(false, "unsupported sample type");
            }
        });

        ds.meta_information.resize(1);
        ds.meta_information[0] = channelToTagMap(channelIdx, sample_rate);
        return ds;
    }

    constexpr TDigitalOutput createDatasetDigital(std::size_t nSamples)
    requires(TPSImpl::N_DIGITAL_CHANNELS > 0 && acquisitionMode == AcquisitionMode::RapidBlock)
    {
        TDigitalOutput ds{};
        ds.extents      = {static_cast<int32_t>(nSamples)};
        ds.signal_names = {"DigitalOut"};
        ds.layout       = gr::LayoutRight{};
        ds.signal_values.resize(nSamples);
        ds.signal_ranges.resize(1);
        ds.timing_events.resize(1);
        // generate time axis
        int        i   = 0;
        const auto pre = static_cast<int>(pre_samples);
        ds.axis_values.resize(1);
        std::ranges::generate(ds.axis_values[0], [&i, pre]() { return static_cast<std::uint16_t>(i++ - pre); });
        return ds;
    }

    template<typename TSample>
    std::vector<std::size_t> findAnalogTriggers(const std::size_t triggerIdx, std::span<TSample> samples) {
        using enum TriggerDirection;
        if (samples.empty()) {
            return {};
        }

        std::vector<std::size_t> triggerOffsets; // relative offset of detected triggers
        const float              band = channel_ranges[triggerIdx] / 100.f;

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

        bool triggerState = toFloat(samples[0]) >= trigger_threshold;
        if (trigger_direction == Rising || trigger_direction == High) {
            for (std::size_t i = 0; i < samples.size(); i++) {
                if (const float value = toFloat(samples[i]); !triggerState && (value >= trigger_threshold)) {
                    triggerState = true;
                    triggerOffsets.push_back(i);
                } else if (triggerState && (value <= trigger_threshold - band)) {
                    triggerState = false;
                }
            }
        } else if (trigger_direction == Falling || trigger_direction == Low) {
            for (std::size_t i = 0; i < samples.size(); i++) {
                if (const float value = toFloat(samples[i]); triggerState && (value <= trigger_threshold)) {
                    triggerState = false;
                    triggerOffsets.push_back(i);
                } else if (!triggerState && (value >= trigger_threshold + band)) {
                    triggerState = true;
                }
            }
        }
        return triggerOffsets;
    }

    std::vector<std::size_t> findDigitalTriggers(const uint digitalChannelNumber, std::span<std::uint16_t> samples)
    requires(TPSImpl::N_DIGITAL_CHANNELS > 0)
    {
        using enum TriggerDirection;
        if (samples.empty()) {
            return {};
        }
        const auto mask = static_cast<uint16_t>(1U << digitalChannelNumber);

        std::vector<std::size_t> triggerOffsets;

        bool triggerState = (samples[0] & mask) > 0;
        if (trigger_direction == Rising || trigger_direction == High) {
            for (std::size_t i = 0; i < samples.size(); i++) {
                if (!triggerState && (samples[i] & mask)) {
                    triggerState = true;
                    triggerOffsets.push_back(i);
                } else if (triggerState && !(samples[i] & mask)) {
                    triggerState = false;
                }
            }
        } else if (trigger_direction == Falling || trigger_direction == Low) {
            for (std::size_t i = 0; i < samples.size(); i++) {
                if (triggerState && !(samples[i] & mask)) {
                    triggerState = false;
                    triggerOffsets.push_back(i);
                } else if (!triggerState && (samples[i] & mask)) {
                    triggerState = true;
                }
            }
        }
        return triggerOffsets;
    }
};

} // namespace fair::picoscope

#endif
