/**
 * A simple utility that uses ImChart terminal plotting utility from gr4 to visualise the values returned from the picoscope via the raw picoscope API.
 * At startup, it performs discovery of all connected Picoscope devices.
 * It is possible to change the parameters at runtime via a textbased interface.
 */

#include <chrono>
#include <cstdio>
#include <print>
#include <ranges>
#include <thread>
#include <vector>

#include <gnuradio-4.0/algorithm/ImChart.hpp>

#include <fair/picoscope/Picoscope3000a.hpp>
#include <fair/picoscope/Picoscope4000a.hpp>
#include <fair/picoscope/Picoscope5000a.hpp>
#include <fair/picoscope/Picoscope6000.hpp>
#include <fair/picoscope/SettingsEditor.hpp>

namespace fair::picoscope::cli {
static auto enumeratePicoscopes() {
    using std::views::split;
    using std::views::transform;
    struct DiscoveredPicoscope {
        std::string model;
        std::string serial;
    };
    std::vector<DiscoveredPicoscope> result;
    std::string                      serials;
    serials.resize(256);
    auto queryPicoscopeType = [&result, &serials]<typename TPSImpl>(const std::string& model) {
        int16_t count;
        auto    realSize = static_cast<int16_t>(serials.size());
        TPSImpl::enumerateUnits(&count, reinterpret_cast<int8_t*>(serials.data()), &realSize);
        std::ranges::copy(serials | std::views::take(realSize) | split(',') | transform([&model](auto&& r) -> DiscoveredPicoscope { return {std::move(model), to<std::string>(r)}; }), std::back_inserter(result));
    };
    queryPicoscopeType.operator()<Picoscope3000a>("PS3000a");
    queryPicoscopeType.operator()<Picoscope4000a>("PS4000a");
    queryPicoscopeType.operator()<Picoscope5000a>("PS5000a");
    queryPicoscopeType.operator()<Picoscope6000>("PS6000");
    return result;
}

struct ChannelData {
    std::vector<double>                            values{};
    std::size_t                                    lastUpdate = 0UZ;
    std::size_t                                    updates    = 0UZ;
    bool                                           overrange  = false;
    std::size_t                                    i          = 0;
    ChannelName                                    id;
    ChannelConfig                                  channelConfig{};
    std::chrono::high_resolution_clock::time_point lastUpdateTimestamp;
};

struct PicoscopeState {
    std::string                                  model;
    std::string                                  serial;
    bool                                         reinit = true;
    std::vector<ChannelData>                     data{};
    std::vector<std::uint16_t>                   digitalData{};
    std::unique_ptr<PicoscopeDynamicWrapperBase> scope{};
    float                                        sample_rate      = 1000.f;
    AcquisitionMode                              acquisition_mode = AcquisitionMode::Streaming;
    std::string                                  trigger_channel;
    int                                          data_type;
    std::string                                  active_channels;
    AnalogChannelRange                           channel_range = AnalogChannelRange::ps5V;
    std::string                                  picoscope_model;
    std::size_t                                  pre           = 100;
    std::size_t                                  post          = 900;
    std::size_t                                  nCaptures     = 10;
    bool                                         enableDigital = false;
};
} // namespace fair::picoscope::cli

int main() {
    using namespace fair::picoscope;
    using namespace fair::picoscope::cli;
    fair::ui_helpers::SettingsEditor editor{};
    constexpr int                    frameDelayMs = 96; // ~10 FPS
    constexpr std::size_t            nHistory     = 500;

    // Check for available scopes. Note: This takes quite some time
    auto availableScopes = enumeratePicoscopes();
    // create storage for all picoscopes to eventually be instantiated
    auto picoscopes = availableScopes | std::views::transform([](auto& discovered) -> PicoscopeState { return {.model = discovered.model, .serial = discovered.serial}; }) | std::ranges::to<std::vector>();

    std::println("starting picoscope-cli, scopes: {}", availableScopes | std::views::transform([](const auto& ps) { return std::format("{}({})", ps.model, ps.serial); }));
    auto start = std::chrono::high_resolution_clock::now();
    while (!editor.quit) { // main UI loop
        for (const auto& [i, picoscope] : std::views::zip(std::views::iota(0L), picoscopes)) {
            if (!picoscope.scope) {
                continue;
            }
            if (picoscope.reinit) {
                picoscope.reinit = false;
                // Initial channel configuration. Enable the first channel for each picoscope by default and start streaming acquisition.
                std::size_t channels = 0;
                picoscope.data       = std::views::zip(std::views::iota(0UZ), picoscope.scope->getChannelIds()) | std::views::transform([&picoscope, &channels](auto x) -> ChannelData {
                    auto& [_i, _id] = x;
                    return {.i{_i}, .id = _id, .channelConfig{channels++ == 0, picoscope.channel_range, 0.0f, Coupling::DC}};
                }) | std::ranges::to<std::vector>();
                for (const ChannelData& config : picoscope.data) {
                    picoscope.scope->configureChannel(config.i, config.channelConfig);
                }
                if (picoscope.acquisition_mode == AcquisitionMode::Streaming) {
                    picoscope.scope->startStreamingAcquisition(picoscope.sample_rate, picoscope.enableDigital);
                } else {
                    picoscope.scope->startTriggeredAcquisition(picoscope.sample_rate, picoscope.pre, picoscope.post, picoscope.nCaptures, []() {}, picoscope.enableDigital);
                }
            }
        }
        auto [termWidth, termHeight] = fair::ui_helpers::SettingsEditor::getTerminalSize();
        // Poll picoscopes: updates settings, fetches new data and copies to the processing buffers
        for (auto& picoscope : picoscopes) {
            if (!picoscope.scope) {
                continue;
            }
            for (const ChannelData& config : picoscope.data) { // update channel configuration
                picoscope.scope->configureChannel(config.i, config.channelConfig);
            }
            int16_t maxValue = std::numeric_limits<int16_t>::max();
            if (picoscope.acquisition_mode == AcquisitionMode::Streaming) {
                std::ignore = picoscope.scope->poll([&picoscope, &maxValue](std::span<std::span<const int16_t>> values, const int16_t overrange) {
                    std::size_t processed = 0;
                    if (!values.empty()) {
                        const std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
                        for (auto [driverData, result] : std::views::zip(values, picoscope.data | std::views::filter([](const ChannelData& x) { return x.channelConfig.enable; }))) {
                            std::ranges::copy(driverData | std::views::transform([maxValue, &result](auto x) { return static_cast<double>(x) * static_cast<double>(result.channelConfig.range) / static_cast<double>(maxValue); }), std::back_inserter(result.values));
                            result.lastUpdate = driverData.size();
                            result.updates += driverData.size();
                            result.overrange           = overrange & (1 << result.i);
                            result.lastUpdateTimestamp = now;
                            ++processed;
                        }
                    }
                    if (picoscope.enableDigital && values.size() > processed) {
                        std::ranges::copy(values[processed], std::back_inserter(picoscope.digitalData));
                    }
                });
            } else {
                bool first  = true;
                std::ignore = picoscope.scope->poll([&picoscope, &maxValue, &first](std::span<std::span<const int16_t>> values, const int16_t overrange) {
                    if (!values.empty()) {
                        const std::chrono::high_resolution_clock::time_point now       = std::chrono::high_resolution_clock::now();
                        std::size_t                                          processed = 0;
                        for (const auto& [driverData, result] : std::views::zip(values, picoscope.data | std::views::filter([](const ChannelData& x) { return x.channelConfig.enable; }))) {
                            if (first) {
                                result.values.clear();
                                const std::size_t samples = (picoscope.pre + picoscope.post) * picoscope.nCaptures;
                                result.values.reserve(samples);
                                // todo: obtain acquisition settings and preallocate vector size
                            }
                            std::ranges::copy(driverData | std::views::transform([maxValue, &result](auto x) { return static_cast<double>(x) * static_cast<double>(result.channelConfig.range) / static_cast<double>(maxValue); }), std::back_inserter(result.values));
                            result.lastUpdate = driverData.size();
                            result.updates += driverData.size();
                            result.overrange           = overrange & (1 << result.i);
                            result.lastUpdateTimestamp = now;
                            ++processed;
                        }
                        if (picoscope.enableDigital && values.size() > processed) {
                            std::ranges::copy(values[processed], std::back_inserter(picoscope.digitalData));
                        }
                        first = false;
                    }
                });
            }
        }
        // Cap framerate
        auto end = std::chrono::high_resolution_clock::now();
        if (auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(); elapsed < frameDelayMs) {
            continue;
        }
        start = std::chrono::high_resolution_clock::now();
        // Plot values
        gr::graphs::ImChart<std::dynamic_extent, std::dynamic_extent> chart{std::clamp(termWidth, 32UZ, 512UZ), termHeight > 35 ? termHeight - 35UZ : 10};
        for (auto& picoscope : picoscopes) {
            if (!picoscope.scope) {
                continue;
            }
            for (auto& result : picoscope.data | std::views::filter([](const ChannelData& x) { return x.channelConfig.enable; })) {
                if (picoscope.acquisition_mode == AcquisitionMode::Streaming) {
                    if (result.values.size() > 10UZ * nHistory) { // trim history size periodically
                        std::copy(result.values.end() - static_cast<long>(nHistory), result.values.end(), result.values.begin());
                        result.values.resize(nHistory);
                    }
                }
                std::size_t n       = result.values.size() > nHistory ? nHistory : result.values.size();
                auto        xValues = std::views::iota(0UZ, n) | std::views::transform([](auto x) { return static_cast<double>(x); }) | std::ranges::to<std::vector>();
                auto        yValues = std::span(result.values).subspan(result.values.size() - n) | std::ranges::to<std::vector>();
                if (n > 1) {
                    chart.draw(xValues, yValues, std::format("{}({})-{}", picoscope.scope->getDeviceInfo().model, picoscope.scope->getDeviceInfo().serial, result.id));
                }
            }
            if (picoscope.digitalData.size() > 10UZ * nHistory) { // trim history size periodically
                std::copy(picoscope.digitalData.end() - static_cast<long>(nHistory), picoscope.digitalData.end(), picoscope.digitalData.begin());
                picoscope.digitalData.resize(nHistory);
            }
        }
        editor.processInput();
        // Show settings for channels and other things
        std::puts("\033[?2026h"); // start a new frame https://github.com/contour-terminal/vt-extensions/blob/master/synchronized-output.md
        gr::graphs::resetView();
        if (chart._n_datasets > 0) { // there is a bug in ImChart that crashes the program whenever there is no data present in the chart.
            chart.draw();
            for (auto& picoscope : picoscopes | std::views::filter(&PicoscopeState::enableDigital) | std::views::filter([](auto& ps) -> bool { return ps.scope != nullptr; })) {
                for (std::size_t i = 0; i < 16UZ; ++i) {
                    std::println("D{:02}: {}", i,
                        picoscope.digitalData                                                                                        // plot digital port data
                            | std::views::drop(picoscope.digitalData.size() - std::min(termWidth - 5, picoscope.digitalData.size())) // drop all older entries that don't fit the screen
                            | std::views::transform([i](auto v) -> char { return v & (1 << i) ? 'O' : '.'; })                        // show 0 as a dot and 1 as a circle
                            | std::ranges::to<std::string>());
                }
            }
        }
        int row = 0;
        for (auto& picoscope : picoscopes) {
            if (bool enabled = picoscope.scope != nullptr; !enabled) {
                editor.renderSetting(std::pair{0, row}, 1, enabled, std::format("Scope {} ({})", picoscope.model, picoscope.serial));
                std::println(" model: {}, serial: {}", picoscope.model, picoscope.serial);
                row++;
                if (enabled) {
                    if (picoscope.model == "PS3000a") {
                        picoscope.scope = std::make_unique<PicoscopeDynamicWrapper<Picoscope3000a>>(picoscope.serial, false);
                    } else if (picoscope.model == "PS4000a") {
                        picoscope.scope = std::make_unique<PicoscopeDynamicWrapper<Picoscope4000a>>(picoscope.serial, false);
                    } else if (picoscope.model == "PS5000a") {
                        picoscope.scope = std::make_unique<PicoscopeDynamicWrapper<Picoscope5000a>>(picoscope.serial, false);
                    } else if (picoscope.model == "PS6000") {
                        picoscope.scope = std::make_unique<PicoscopeDynamicWrapper<Picoscope6000>>(picoscope.serial, false);
                    } else {
                        throw std::runtime_error("Unknown picoscope model");
                    }
                    picoscope.reinit = true;
                }
            } else {
                int ps_col = 0;
                editor.renderSetting(std::pair{ps_col++, row}, 1, enabled, std::format("Scope {} ({})", picoscope.model, picoscope.serial));
                if (!enabled) {
                    row++;
                    picoscope.scope = nullptr;
                    continue;
                }
                std::print("opened device: model: {}, serial: {}, hardware version: {}, ", picoscope.scope->getDeviceInfo().model, picoscope.scope->getDeviceInfo().serial, picoscope.scope->getDeviceInfo().hardwareVersion);
                if (auto& lastError = picoscope.scope->getLastError(); lastError) {
                    std::println("last error: {}: {}\nat: {}:L{}", lastError.value().getError(), lastError.value().getDescription(), lastError.value().location.file_name(), lastError.value().location.line());
                } else {
                    std::println("no errors encountered");
                }
                row++;
                ps_col = 0;
                // acquisitionSettings
                picoscope.reinit |= editor.renderSetting(std::pair{ps_col++, row}, 1, picoscope.acquisition_mode, "AcquisitionMode");
                picoscope.reinit |= editor.renderSetting(std::pair{ps_col++, row}, 1, picoscope.sample_rate, "set sample rate");
                picoscope.reinit |= editor.renderSetting(std::pair{ps_col++, row}, 1, picoscope.enableDigital, "enable digital");
                picoscope.reinit |= editor.renderSetting(std::pair{ps_col++, row}, 1, picoscope.pre, "samples to capture before trigger");
                picoscope.reinit |= editor.renderSetting(std::pair{ps_col++, row}, 1, picoscope.post, "samples to capture after trigger");
                picoscope.reinit |= editor.renderSetting(std::pair{ps_col++, row}, 1, picoscope.nCaptures, "number of captures");
                // trigger settings
                TriggerConfig triggerConfig        = picoscope.scope->getTriggerConfig();
                bool          triggerConfigChanged = false;
                if (editor.renderSetting(std::pair{ps_col++, row}, 1, picoscope.trigger_channel, "set channel id for trigger")) {
                    if (picoscope.trigger_channel.starts_with("DI")) {
                        triggerConfig.source      = static_cast<unsigned int>(strtoul(picoscope.trigger_channel.substr(2).data(), nullptr, 10));
                        picoscope.trigger_channel = std::format("DI{}", std::get<unsigned int>(triggerConfig.source));
                    } else if (std::optional<ChannelName> analogChannel = magic_enum::enum_cast<ChannelName>(picoscope.trigger_channel); analogChannel.has_value()) {
                        triggerConfig.source      = analogChannel.value();
                        picoscope.trigger_channel = magic_enum::enum_name(std::get<ChannelName>(triggerConfig.source));
                    } else {
                        triggerConfig.source      = {};
                        picoscope.trigger_channel = "";
                    }
                    triggerConfigChanged = true;
                }
                triggerConfigChanged |= editor.renderSetting(std::pair{ps_col++, row}, 1, triggerConfig.threshold, "trigger threshold");
                triggerConfigChanged |= editor.renderSetting(std::pair{ps_col++, row}, 1, triggerConfig.direction, "trigger direction");
                triggerConfigChanged |= editor.renderSetting(std::pair{ps_col++, row}, 1, triggerConfig.delay, "trigger delay");
                if (triggerConfigChanged) {
                    picoscope.scope->configureTrigger(triggerConfig);
                }
                std::println();
                row++;
                // channel settings
                for (auto& channel : picoscope.data) {
                    int col = 0;
                    std::print("  channel {:15}: ", channel.id);
                    editor.renderSetting(std::pair{col++, row}, 1, channel.channelConfig.enable, std::format("enable/disable channel {}", channel.id));
                    editor.renderSetting(std::pair{col++, row}, 1, channel.channelConfig.range, std::format("set range for channel {}", channel.id));
                    editor.renderSetting(std::pair{col++, row}, 1, channel.channelConfig.offset, std::format("set offset for channel {}", channel.id));
                    editor.renderSetting(std::pair{col++, row}, 1, channel.channelConfig.coupling, std::format("channel coupling mode for channel {}", channel.id));
                    std::print("   - lastUpdateTimestamp: {}, total samples: {} lastUpdate: {}", channel.lastUpdateTimestamp.time_since_epoch(), channel.updates, channel.lastUpdate);
                    std::println();
                    row++;
                }
            }
        }
        editor.renderEditor();
        std::println();
        std::puts("\033[?2026l"); // end frame, render contents
    }
    return 0;
}
