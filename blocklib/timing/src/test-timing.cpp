#include <format>
#include <thread>
#include <cstdio>
#include <unordered_set>
#include <ranges>
#include <string>

// UI
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include "ImScoped.hpp"

#include <fmt/chrono.h>
#include <fmt/ranges.h>
#include <OutputCondition_Proxy.hpp>

#include <timing.hpp>
#include <experimental/simd>
#include "fair_header.h"
#include "fairPlot.hpp"
#include "event_definitions.hpp"

enum class InjectState { STOPPED, RUNNING, SINGLE };
static constexpr uint64_t max_uint64 = std::numeric_limits<uint64_t>::max();
static constexpr uint64_t max_uint42 = (1UL << 42) - 1;
static constexpr uint64_t max_uint22 = (1UL << 22) - 1;
static constexpr uint64_t max_uint14 = (1UL << 14) - 1;
static constexpr uint64_t max_uint12 = (1UL << 12) - 1;
static constexpr uint64_t max_uint4 = (1UL << 4) - 1;
static constexpr double minDouble = 0;
static constexpr double maxDouble = std::numeric_limits<double>::max();

template <typename... T>
void tableColumnString(const fmt::format_string<T...> &fmt, T&&... args) {
    if (ImGui::TableNextColumn()) {
        ImGui::TextUnformatted(fmt::format(fmt, std::forward<T>(args)...).c_str());
    }
}
template <typename... T>
void tableColumnStringColor(ImColor color, const fmt::format_string<T...> &fmt, T&&... args) {
if (ImGui::TableNextColumn()) {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, color);
        ImGui::TextUnformatted(fmt::format(fmt, std::forward<T>(args)...).c_str());
    }
}
void tableColumnBool(bool state, ImColor trueColor, ImColor falseColor) {
    if (ImGui::TableNextColumn()) {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, state ? trueColor : falseColor);
        ImGui::TextUnformatted(fmt::format("{}", state ? "y" : "n").c_str());
    }
}
template <typename T, T max>
void tableColumnSlider(const std::string &id, T &field, float width) {
    static constexpr T min_T = 0;
    static constexpr T max_T = max;
    if (ImGui::TableNextColumn()) {
        ImGui::SetNextItemWidth(width);
        uint64_t tmp = field;
        ImGui::DragScalar(id.c_str(), ImGuiDataType_U64, &tmp, 1.0f, &min_T, &max_T, "%d", ImGuiSliderFlags_None);
        field = tmp & max;
    }
}
template <typename T, T max, T scale>
void sliderScaled(const std::string &id, T &field) {
    static constexpr T min_T = 0;
    static constexpr T max_T = max;
    uint64_t tmp = field / scale;
    ImGui::DragScalar(id.c_str(), ImGuiDataType_U64, &tmp, 1.0f, &min_T, &max_T, "%d", ImGuiSliderFlags_None);
    field = (tmp * scale) & max;
}
template <typename T, T max, T scale>
void tableColumnSliderScaled(const std::string &id, T &field, float width) {
    if (ImGui::TableNextColumn()) {
        ImGui::SetNextItemWidth(width);
        sliderScaled<T, max, scale>(id, field);
    }
}
void tableColumnCheckbox(const std::string &id, bool &field) {
    if (ImGui::TableNextColumn()) {
        bool flag_beamin = field;
        ImGui::Checkbox(id.c_str(), &flag_beamin);
        field = flag_beamin;
    }
}

template<typename T>
std::size_t getStableColorIndex(T id, std::map<T, std::size_t> &colors, std::size_t colormapSize) {
    if (colors.contains(id)) {
        return colors[id];
    } else {
        if (colormapSize - 1 > colors.size()) {
            colors.insert({id, colors.size()});
            return colors[id];
        }
    }
    return colormapSize - 1;
}

template<typename T>
std::size_t getStableBPCIDColorIndex(T id) {
    static std::map<T, std::size_t> colors{};
    return getStableColorIndex(id, colors, bpcidColors.size());
}

template<typename T>
ImColor getStableBPCIDColor(T id) {
    return ImColor{bpcidColors[getStableBPCIDColorIndex(static_cast<std::size_t>(id))]};
}

std::pair<uint64_t, uint64_t> TimingGroupFilterDropdown() {
    static int current = 0;
    static const uint64_t mask = ((1ULL << 16) - 1 ) << (64-16);
    auto getTimingMask = [&mask_ = mask](uint64_t gid){
        uint64_t id = ((gid & ((1UL << 12) - 1)) << 48)
                      + ((1   & ((1UL <<  4) - 1)) << 60);
        return std::pair<uint64_t, uint64_t> {id, mask_};
    };
    static int radioButtonState = 0;
    ImGui::RadioButton("All Timing Groups", &radioButtonState, 0); ImGui::SameLine();
    ImGui::RadioButton("SIS18", &radioButtonState, 1); ImGui::SameLine();
    ImGui::RadioButton("SIS100", &radioButtonState, 2); ImGui::SameLine();
    ImGui::RadioButton("ESR", &radioButtonState, 3); ImGui::SameLine();
    ImGui::RadioButton("CRYRING", &radioButtonState, 4); ImGui::SameLine();
    ImGui::RadioButton("Other:", &radioButtonState, 5); ImGui::SameLine();
    static std::vector<const char*> items{};
    static std::vector<std::string> displayStrings{timingGroupTable.size() - 4};
    static std::vector<uint64_t> result = [&itms = items, &dispStrings = displayStrings]() {
       std::vector<uint64_t> res{};
       for (const auto & [gid, strings] : timingGroupTable) {
           if (gid == 300 /*SIS18*/ || gid == 310 /*SIS100*/ || gid == 210 /*CRYRING*/ || gid == 340 /*ESR*/) {
               continue;
           }
           auto & [enumName, description] = strings;
           dispStrings.push_back(fmt::format("{} ({})", enumName, gid));
           itms.push_back(dispStrings.back().c_str());
           uint64_t id = ((gid & ((1UL << 12) - 1)) << 48)
                       + ((1   & ((1UL <<  4) - 1)) << 60);
           res.emplace_back(id);
       }
       return res;
    }();
    ImGui::SetNextItemWidth(200);
    {
        auto _ = ImScoped::Disabled(radioButtonState != 5);
        ImGui::Combo("##TimingGroup", &current, items.data(), static_cast<int>(items.size()));
    }
    switch (radioButtonState) {
        case 0:
            return {0,0};
        case 1:
            return getTimingMask(300);
        case 2:
            return getTimingMask(310);
        case 3:
            return getTimingMask(340);
        case 4:
            return getTimingMask(210);
        default:
            return {result[static_cast<std::size_t>(current)], mask};
    }
}

void drawSnoopedEventTableRow(const Timing::Event &evt, Timing &timing) {
    ImGui::TableNextRow();
    ImGui::PushID(&evt);
    tableColumnString("{}", taiNsToUtc(evt.time));
    tableColumnString("{}", taiNsToUtc(evt.executed));
    tableColumnStringColor(getStableBPCIDColor(evt.bpcid),"{}", evt.bpcid);
    tableColumnString("{}", evt.sid);
    tableColumnString("{}", evt.bpid);
    tableColumnString("{1}({0})", evt.gid, timingGroupTable.contains(evt.gid) ? timingGroupTable.at(evt.gid).first : "UNKNOWN");
    tableColumnString("{1}({0})", evt.eventNo, eventNrTable.contains(evt.eventNo) ? eventNrTable.at(evt.eventNo).first : "UNKNOWN");
    tableColumnBool(evt.flagBeamin, ImGui::GetColorU32({0, 1.0, 0, 0.4f}), ImGui::GetColorU32({1.0, 0, 0, 0.4f}));
    tableColumnBool(evt.flagBpcStart, ImGui::GetColorU32({0, 1.0, 0, 0.4f}), ImGui::GetColorU32({1.0, 0, 0, 0.4f}));
    tableColumnBool(evt.reqNoBeam, ImGui::GetColorU32({0,1.0,0,0.4f}), ImGui::GetColorU32({1.0,0,0,0.4f}));
    tableColumnString("{}", evt.virtAcc);
    tableColumnString("{}", evt.bpcts);
    tableColumnString("{}", evt.fid);
    tableColumnString("{:#08x}", evt.id());
    tableColumnString("{:#08x}", evt.param());
    tableColumnBool(evt.flagReserved1, ImGui::GetColorU32({0, 1.0, 0, 0.4f}), ImGui::GetColorU32({1.0, 0, 0, 0.4f}));
    tableColumnBool(evt.flagReserved2, ImGui::GetColorU32({0, 1.0, 0, 0.4f}), ImGui::GetColorU32({1.0, 0, 0, 0.4f}));
    tableColumnBool(evt.reserved, ImGui::GetColorU32({0,1.0,0,0.4f}), ImGui::GetColorU32({1.0,0,0,0.4f}));
    if (ImGui::TableNextColumn()) { // flags
        // print flags
        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, evt.flags ? ImGui::GetColorU32({1.0,0,0,0.4f}) : ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_TableRowBg]));
        auto delay = (static_cast<double>(evt.executed - evt.time)) * 1e-6;
        if (evt.flags & 1) {
            ImGui::Text("%s", fmt::format(" !late (by {} ms)", delay).c_str());
        } else if (evt.flags & 2) {
            ImGui::Text("%s", fmt::format(" !early (by {} ms)", delay).c_str());
        } else if (evt.flags & 4) {
            ImGui::Text("%s", fmt::format(" !conflict (delayed by {} ms)", delay).c_str());
        } else if (evt.flags & 8) {
            ImGui::Text("%s", fmt::format(" !delayed (by {} ms)", delay).c_str());
        }
    }
    ImGui::TableNextColumn();
    if (ImGui::Button("add to Schedule##addSchedule")) {
        timing.events.emplace_back(100000000ULL + (timing.events.empty() ? 0UL : timing.events.back().time), evt.id(), evt.param());
    }
    ImGui::PopID();
}

void showTimingEventTable(Timing &timing) {
    static gr::BufferReader auto event_reader = timing.snooped.new_reader();
    if (ImGui::Button("clear")) {
        std::ignore = event_reader.consume(event_reader.available());
    }
    ImGui::SameLine(); ImGui::Dummy({50,5});ImGui::SameLine();
    auto [id_filter, mask] = TimingGroupFilterDropdown();
    if (id_filter != timing.snoopID || mask != timing.snoopMask) {
        timing.snoopID = id_filter;
        timing.snoopMask = mask;
        timing.updateSnoopFilter();
    }
    if (ImGui::CollapsingHeader("Received Timing Events", ImGuiTreeNodeFlags_DefaultOpen)) {
        static int freeze_cols = 1;
        static int freeze_rows = 1;

        const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
        ImVec2 outer_size{0.0f, TEXT_BASE_HEIGHT * 20};
        static ImGuiTableFlags flags =
                ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
                ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
        if (auto _ = ImScoped::Table("received_events", 20, flags, outer_size, 0.f)) {
            ImGui::TableSetupScrollFreeze(freeze_cols, freeze_rows);
            ImGui::TableSetupColumn("timestamp", ImGuiTableColumnFlags_NoHide); // Make the first column not hideable to match our use of TableSetupScrollFreeze()
            ImGui::TableSetupColumn("executed at", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("bpcid");
            ImGui::TableSetupColumn("sid");
            ImGui::TableSetupColumn("bpid");
            ImGui::TableSetupColumn("gid");
            ImGui::TableSetupColumn("eventno");
            ImGui::TableSetupColumn("beam-in");
            ImGui::TableSetupColumn("bpc-start");
            ImGui::TableSetupColumn("req no beam");
            ImGui::TableSetupColumn("virt acc");
            ImGui::TableSetupColumn("bpcts");
            ImGui::TableSetupColumn("fid", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("id", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("param", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("reserved1", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("reserved2", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("reserved", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("flags");
            ImGui::TableSetupColumn("##addToSchedule");

            ImGui::TableHeadersRow();
            auto data = event_reader.get();

            for (const auto &evt : std::ranges::reverse_view{data}) {
                drawSnoopedEventTableRow(evt, timing);
            }
            if (data.size() > event_reader.buffer().size() / 2) {
                std::ignore = event_reader.consume(data.size() - event_reader.buffer().size() / 2);
            }
        }
    }
}


bool drawScheduledEventTableRow(Timing::Event &ev, Timing &timing, uint64_t default_offset) {
    bool to_remove = false;
    ImGui::PushID(&ev);
    ImGui::TableNextRow();
    tableColumnSliderScaled<uint64_t, max_uint64, 1000000>("[ms]###time", ev.time, 80.f);
    tableColumnSlider<uint32_t, max_uint22>("##bpcid", ev.bpcid, 40.f);
    tableColumnSlider<uint16_t, max_uint12>("##sid", ev.sid, 40.f);
    tableColumnSlider<uint16_t, max_uint14>("##pbid", ev.bpid, 40.f);
    tableColumnSlider<uint16_t, max_uint12>("##gid", ev.gid, 40.f);
    ImGui::SameLine();
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", fmt::format("{}", timingGroupTable.contains(ev.gid) ? timingGroupTable.at(ev.gid).first : "UNKNOWN").c_str());
    }
    tableColumnSlider<uint16_t, max_uint12>("##eventno", ev.eventNo, 40.f);
    ImGui::SameLine();
    ImGui::TextUnformatted(fmt::format("{}", eventNrTable.contains(ev.eventNo) ? eventNrTable.at(ev.eventNo).first : "UNKNOWN").c_str());
    tableColumnCheckbox("##beamin", ev.flagBeamin);
    tableColumnCheckbox("##bpcstart", ev.flagBpcStart);
    tableColumnCheckbox("##reqNoBeam", ev.reqNoBeam);
    tableColumnSlider<uint8_t, max_uint4>("##virtAcc", ev.virtAcc, 40.f);
    tableColumnSlider<uint64_t, max_uint42>("##bpcts", ev.bpcts, 80.f);
    tableColumnSlider<uint8_t, max_uint4>("##fid", ev.fid, 40.f);
    tableColumnString("{:#08x}", ev.id());
    tableColumnString("{:#08x}", ev.param());
    tableColumnCheckbox("##reserved1", ev.flagReserved1);
    tableColumnCheckbox("##reserved2", ev.flagReserved2);
    tableColumnCheckbox("##reserved", ev.reserved);
    // interactive settings
    ImGui::TableNextColumn();
    if (ImGui::Button("remove")) {
        to_remove = true;
    }
    ImGui::TableNextColumn();
    if (ImGui::Button("inject")) {
        timing.injectEvent(ev,
                           timing.currentTimeTAI() + default_offset);
    }
    auto trigger = timing.triggers.contains(ev.id()) ? std::optional<Timing::Trigger>{timing.triggers[ev.id()]} : std::optional<Timing::Trigger>{};
    for (const auto & [i, outputName, _2] : timing.outputs) {
        ImGui::TableNextColumn();
        auto [name, _3] = outputConfig.at(outputName);
        if (trigger) {
            ImGui::Checkbox(fmt::format("##{}", name).c_str(), &trigger->outputs[i]);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", fmt::format("Output: {}", outputName).c_str());
            }
        } else {
            bool output_selected = false;
            ImGui::Checkbox(fmt::format("##{}", name).c_str(), &output_selected);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", fmt::format("Output: {}", outputName).c_str());
            }
            if (output_selected) {
                trigger = Timing::Trigger{};
                trigger->id = ev.id();
                trigger->outputs[i] = true;
                trigger->delay = 0.0;
                trigger->flattop = 0.003;
            }
        }
    }
    ImGui::TableNextColumn();
    if (trigger) {
        ImGui::SetNextItemWidth(80);
        ImGui::DragScalar("[ms]###delay", ImGuiDataType_Double, &trigger->delay, 1.0f, &minDouble, &maxDouble, "%f", ImGuiSliderFlags_None);
    }
    ImGui::TableNextColumn();
    if (trigger) {
        ImGui::SetNextItemWidth(80);
        ImGui::DragScalar("[ms]###flattop", ImGuiDataType_Double, &trigger->flattop, 1.0f, &minDouble, &maxDouble, "%f", ImGuiSliderFlags_None);
    }
    if (trigger) {
        timing.updateTrigger(*trigger);
    }
    ImGui::PopID();
    return to_remove;
}

void scheduleUpcomingEvents(Timing &timing, size_t &current, uint64_t &time_offset, InjectState &injectState) {
    using enum InjectState;
    if (injectState == RUNNING || injectState == SINGLE) {
        if (current >= timing.events.size()) {
            injectState = STOPPED;
        } else {
            while (timing.events[current].time + time_offset < timing.currentTimeTAI() + 500000000UL) {
                auto ev = timing.events[current];
                timing.injectEvent(ev, time_offset);
                if (current + 1 >= timing.events.size()) {
                    if (injectState == SINGLE) {
                        injectState = STOPPED;
                        break;
                    } else {
                        time_offset += timing.events[current].time;
                        current = 0;
                    }
                } else {
                    ++current;
                }
            }
        }
    }
}

void startStopButtons(Timing &timing, size_t &current, uint64_t &time_offset, InjectState &injectState) {
    using enum InjectState;
    ImGui::SameLine();
    if (injectState == RUNNING) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.4f, 0.4f, 1.0f,1.0f});
        if (ImGui::Button("Stop###StartStop")) {
            injectState = STOPPED;
        }
        ImGui::PopStyleColor();
    } else {
        if (ImGui::Button("Start###StartStop")) {
            current = 0;
            time_offset = timing.currentTimeTAI();
            injectState = RUNNING;
        }
    }
    ImGui::SameLine();
    if (injectState == SINGLE) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.4f, 0.4f, 1.0f,1.0f});
        if (ImGui::Button("Stop###SingleStop")) {
            injectState = STOPPED;
        }
        ImGui::PopStyleColor();
    } else {
        auto _ = ImScoped::Disabled(injectState != STOPPED);
        if (ImGui::Button("Single###SingleStop")) {
            current = 0;
            time_offset = timing.currentTimeTAI();
            injectState = SINGLE;
        }
    }
}

void saveLoadEvents(Timing &timing) {
    ImGui::Button("Load");
    if (ImGui::BeginPopupContextItem("load schedule popup", ImGuiPopupFlags_MouseButtonLeft)) {
        if (ImGui::Button("from Clipboard")) {
            Timing::Event::loadEventsFromString(timing.events, ImGui::GetClipboardText());
        }
        for (const auto & [scheduleName, schedule] : demoSchedules) {
            if (ImGui::Button(scheduleName.c_str())) {
                Timing::Event::loadEventsFromString(timing.events, schedule);
            }
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Save to Clipboard")) {
        std::string string;
        for (const auto &ev: timing.events) {
            string.append(fmt::format("{:#x} {:#x} {}\n", ev.id(), ev.param(), ev.time));
        }
        ImGui::SetClipboardText(string.c_str());
    }
}

void showTimingSchedule(Timing &timing) {
    using enum InjectState;
    static std::size_t current = 0;
    static uint64_t time_offset = 0;
    static InjectState injectState = STOPPED;
    if (ImGui::CollapsingHeader("SchedULe to inject", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SetNextItemWidth(80.f);
        static uint64_t default_offset = 100000000UL; // 100ms
        sliderScaled<uint64_t, max_uint64, 1000000>("[ms] default event offset", default_offset);
        ImGui::SameLine();
        if (ImGui::Button("+")) {
            timing.events.emplace_back(default_offset + (timing.events.empty() ? 0UL : timing.events.back().time));
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear##schedule")) {
            timing.events.clear();
        }
        ImGui::SameLine();
        saveLoadEvents(timing);
        ImGui::SameLine();
        ImGui::Dummy({50, 5});
        startStopButtons(timing, current, time_offset, injectState);
        // schedule table
        static int freeze_cols = 1;
        static int freeze_rows = 1;
        const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
        ImVec2 outer_size{0.0f, TEXT_BASE_HEIGHT * 15};
        static ImGuiTableFlags flags =
                ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
                ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
        {
            auto _ = ImScoped::Disabled(injectState != STOPPED);
            if (auto _1 = ImScoped::Table("event schedule", static_cast<int>(21 + timing.outputs.size()), flags, outer_size, 0.f)) {
                ImGui::TableSetupScrollFreeze(freeze_cols, freeze_rows);
                ImGui::TableSetupColumn("time", ImGuiTableColumnFlags_NoHide); // Make the first column not hideable to match our use of TableSetupScrollFreeze()
                ImGui::TableSetupColumn("bpcid");
                ImGui::TableSetupColumn("sid");
                ImGui::TableSetupColumn("bpid");
                ImGui::TableSetupColumn("gid");
                ImGui::TableSetupColumn("eventno");
                ImGui::TableSetupColumn("beam-in");
                ImGui::TableSetupColumn("bpc-start");
                ImGui::TableSetupColumn("req no beam");
                ImGui::TableSetupColumn("virt acc");
                ImGui::TableSetupColumn("bpcts");
                ImGui::TableSetupColumn("fid");
                ImGui::TableSetupColumn("id", ImGuiTableColumnFlags_DefaultHide);
                ImGui::TableSetupColumn("param", ImGuiTableColumnFlags_DefaultHide);
                ImGui::TableSetupColumn("reserved1", ImGuiTableColumnFlags_DefaultHide);
                ImGui::TableSetupColumn("reserved2", ImGuiTableColumnFlags_DefaultHide);
                ImGui::TableSetupColumn("reserved", ImGuiTableColumnFlags_DefaultHide);
                ImGui::TableSetupColumn("##inject", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("##remove", ImGuiTableColumnFlags_NoHide);
                for (const auto & [_2, outputName, _3] : timing.outputs) {
                    auto [name, show] = outputConfig.contains(outputName) ? outputConfig.at(outputName) : std::pair{outputName, false};
                    ImGui::TableSetupColumn(name.c_str(), show ? ImGuiTableColumnFlags_None : ImGuiTableColumnFlags_DefaultHide);
                }
                ImGui::TableSetupColumn("Trigger Delay", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Trigger Flat-Top", ImGuiTableColumnFlags_NoHide);

                ImGui::TableHeadersRow();

                timing.events.erase(std::remove_if(timing.events.begin(), timing.events.end(),
                        [&timing, &default_offset_ = default_offset](auto &ev) { return drawScheduledEventTableRow(ev, timing, default_offset_); }), timing.events.end());
            }
        }
    }
    scheduleUpcomingEvents(timing, current, time_offset, injectState);
}

void outputsTable(const Timing &timing, float TEXT_BASE_HEIGHT) {
    static int freeze_cols = 1;
    static int freeze_rows = 1;
    ImVec2 outer_size{(ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x) * 0.5f, TEXT_BASE_HEIGHT * 10};
    static ImGuiTableFlags flags =
            ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
            ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
    if (auto _ = ImScoped::Table("ioConfiguration", 8, flags, outer_size, 0.f)) {
        ImGui::TableSetupScrollFreeze(freeze_cols, freeze_rows);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
        ImGui::TableSetupColumn("Direction");
        ImGui::TableSetupColumn("Output Enable");
        ImGui::TableSetupColumn("Input Termination");
        ImGui::TableSetupColumn("Special Out");
        ImGui::TableSetupColumn("Special In");
        ImGui::TableSetupColumn("Resolution");
        ImGui::TableSetupColumn("Level");
        ImGui::TableHeadersRow();

        auto outputs = timing.receiver->getOutputs();

        for (const auto & [name, port]:outputs) {
            ImGui::PushID(&name);
            ImGui::TableNextRow();
            auto port_proxy = saftlib::Output_Proxy::create(port);
            auto input_name = port_proxy->getInput();
            auto input_proxy = input_name.empty() ? std::shared_ptr<saftlib::Input_Proxy>{} : saftlib::Input_Proxy::create(input_name);
            tableColumnString("{}", name);
            tableColumnString(""); //{}", port_proxy->direction);
            tableColumnString("{}", port_proxy->getOutputEnable());
            tableColumnString("{}", input_proxy && input_proxy->getInputTermination());
            tableColumnString("{}", input_proxy && port_proxy->getSpecialPurposeOut());
            tableColumnString("{}", input_proxy && input_proxy->getSpecialPurposeIn());
            tableColumnString("{}", input_proxy && input_proxy->getResolution());
            tableColumnString("{}", port_proxy->getLogicLevelOut());
            ImGui::PopID();
        }
    }
}

void conditionsTable(const Timing &timing, const float TEXT_BASE_HEIGHT) {
    static int freeze_cols_conds = 1;
    static int freeze_rows_conds = 1;
    ImVec2 outer_size_conds{0.0f, TEXT_BASE_HEIGHT * 10};
    static ImGuiTableFlags flags_conds =
            ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
            ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
    if (auto _ = ImScoped::Table("ioConditions", 10, flags_conds, outer_size_conds, 0.f)) {
        ImGui::TableSetupScrollFreeze(freeze_cols_conds, freeze_rows_conds);
        ImGui::TableSetupColumn("IO", ImGuiTableColumnFlags_NoHide);
        ImGui::TableSetupColumn("Offset");
        ImGui::TableSetupColumn("ID Filter");
        ImGui::TableSetupColumn("Mask");
        ImGui::TableSetupColumn("Accept Confligt");
        ImGui::TableSetupColumn("Accept Early");
        ImGui::TableSetupColumn("Accept Delayed");
        ImGui::TableSetupColumn("Accept Late");
        ImGui::TableSetupColumn("Active");
        ImGui::TableSetupColumn("OutputState");
        ImGui::TableHeadersRow();

        for (const auto &[name, port] : timing.receiver->getOutputs()) {
            auto port_proxy = saftlib::Output_Proxy::create(port);
            for (const auto & condition : port_proxy->getAllConditions()) {
                auto condition_proxy = saftlib::OutputCondition_Proxy::create(condition);
                ImGui::PushID(&condition);
                ImGui::TableNextRow();
                tableColumnString("{}", name);
                tableColumnString("{}", condition_proxy->getOffset());
                tableColumnString("{:#016x}", condition_proxy->getID());
                tableColumnString("{:#016x}", condition_proxy->getMask());
                tableColumnString("{}", condition_proxy->getAcceptConflict());
                tableColumnString("{}", condition_proxy->getAcceptEarly());
                tableColumnString("{}", condition_proxy->getAcceptDelayed());
                tableColumnString("{}", condition_proxy->getAcceptLate());
                tableColumnString("{}", condition_proxy->getActive());
                tableColumnString("{}", condition_proxy->getOn());
                ImGui::PopID();
            }
        }
    }
}

void showTRConfig(Timing &timing, bool &imGuiDemo, bool &imPlotDemo) {
    ImGui::SetNextItemOpen(false, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Timing Receiver IO configuration", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("ShowImGuiDemo", &imGuiDemo);
        ImGui::SameLine();
        ImGui::Checkbox("ShowImPlotDemo", &imPlotDemo);

        if (!timing.initialized) {
            ImGui::TextUnformatted("No timing receiver found");
            return;
        }
        if (timing.simulate) {
            ImGui::TextUnformatted("Mocking the timing card by directly forwarding injected events");
            return;
        }
        uint64_t trTime = timing.currentTimeTAI();
        ImGui::TextUnformatted(fmt::format("{} -- ({} ns)\nTemperature: {}°C,\nGateware: {},\n(\"version\", \"{}\")",
                                           trTime, taiNsToUtc(trTime),
                                           timing.receiver->CurrentTemperature(),
                                           fmt::join(timing.receiver->getGatewareInfo(), ",\n"),
                                           timing.receiver->getGatewareVersion()).c_str());
        const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
        outputsTable(timing, TEXT_BASE_HEIGHT);
        ImGui::SameLine();
        conditionsTable(timing, TEXT_BASE_HEIGHT);
    }
}

template<gr::Buffer BufferT>
class TimePlot {
public:
    static constexpr int bufferSize = 5000;
    using Reader = decltype(std::declval<BufferT>().new_reader());
private:
    uint64_t startTime = 0;
    FairPlot::ScrollingBuffer<bufferSize> beamin{};
    FairPlot::ScrollingBuffer<bufferSize> bpcids{};
    FairPlot::ScrollingBuffer<bufferSize> sids{};
    FairPlot::ScrollingBuffer<bufferSize> bpids{};
    FairPlot::ScrollingBuffer<bufferSize> events{};
    Reader snoopReader;
    int boolColormap = FairPlot::boolColormap();
    int bpcidColormap = FairPlot::bpcidColormap();
    int bpidColormap = FairPlot::bpidColormap();
    int sidColormap = FairPlot::sidColormap();
    std::optional<Timing::Event> previousContextEvent{};
    bool previousBPToggle = false;
    bool previousSIDToggle = false;
public:
    explicit TimePlot(BufferT &_events) : snoopReader{_events.new_reader()} { }

    void updateStreaming() {
        auto newEvents = snoopReader.get();
        if (startTime == 0 && !newEvents.empty()) {
            startTime = newEvents[0].time;
        }
        for (const auto &event: newEvents) {
            double eventtime = (static_cast<double>(event.time - startTime)) * 1e-9;
            // filter out starts of new contexts
            if (!previousContextEvent || (event.eventNo == 256 && (event.bpcid != previousContextEvent->bpcid || event.sid != previousContextEvent->sid || event.bpid != previousContextEvent->bpid))) {
                previousSIDToggle = (previousContextEvent->sid != event.sid) ? !previousSIDToggle : previousSIDToggle;
                previousBPToggle = (previousContextEvent->bpid != event.bpid) ? !previousBPToggle : previousBPToggle;
                beamin.pushBack({eventtime, static_cast<double>(event.flagBeamin)});
                bpcids.pushBack({eventtime, static_cast<double>(getStableBPCIDColorIndex(static_cast<uint16_t>(event.bpcid)))});
                sids.pushBack({eventtime, static_cast<double>(previousSIDToggle)});
                bpids.pushBack({eventtime, static_cast<double>(previousBPToggle)});
                previousContextEvent = event;
            }
            if (event.eventNo != 256) { // filter out non-BP_START events
                events.pushBack({eventtime, static_cast<double>(event.eventNo)});
            }
        }
        std::ignore = snoopReader.consume(newEvents.size()); // consume processed events
    }

    void display(Timing &timing) const {
        double plot_depth = 10; // [s]
        auto currentTime = timing.currentTimeTAI();
        double time = (static_cast<double>(currentTime - startTime)) * 1e-9;
        if (ImGui::CollapsingHeader("Plot", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImPlot::BeginPlot("timing markers", ImVec2(-1,0), ImPlotFlags_CanvasOnly)) {
                ImPlot::SetupAxes(fmt::format("t [s] + {}", taiNsToUtc(currentTime)).c_str(), nullptr, 0, ImPlotAxisFlags_NoDecorations);
                ImPlot::SetupAxisLimits(ImAxis_X1, -plot_depth, 0, ImGuiCond_Always);

                // plot freestanding events
                if (!events.empty()) {
                    FairPlot::plotNamedEvents("Events", events, static_cast<ImPlotInfLinesFlags_>(0), -time);
                }

                ImPlot::PushStyleVar(ImPlotStyleVar_DigitalBitHeight, 16.0f);

                if (!beamin.empty()) {
                    ImPlot::PushColormap(boolColormap);
                    FairPlot::plotStatusBar("beamin_plot", beamin, -time);
                    ImPlot::PopColormap();
                }

                if (!bpcids.empty()) {
                    ImPlot::PushColormap(bpcidColormap);
                    FairPlot::plotStatusBar("bpcid_plot", bpcids, -time);
                    ImPlot::PopColormap();
                }
                if (!sids.empty()) {
                    ImPlot::PushColormap(sidColormap);
                    FairPlot::plotStatusBar("sid_plot", sids, -time);
                    ImPlot::PopColormap();
                }
                if (!bpids.empty()) {
                    ImPlot::PushColormap(bpidColormap);
                    FairPlot::plotStatusBar("bpid_plot", bpids, -time);
                    ImPlot::PopColormap();
                }
                ImPlot::PopStyleVar();
                ImPlot::EndPlot();
            }
        }
    }
};

std::pair<SDL_Window*, SDL_GLContext> openSDLWindow() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return {};
    }
    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    auto window_flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    return {window, gl_context};
}

void processSDLEvents(SDL_Window * window, bool &done) {
    for(SDL_Event event; SDL_PollEvent(&event); ) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                                       event.window.windowID == SDL_GetWindowID(window))) {
            done = true;
        }
    }
}

void startImGuiFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void RenderToSDL(const ImGuiIO& io, SDL_Window *window) {
    ImGui::Render();
    static ImVec4 clear_color{0.45f, 0.55f, 0.60f, 1.00f};
    glViewport(0, 0, static_cast<int>(io.DisplaySize.x), static_cast<int>(io.DisplaySize.y));
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
}

void shutdownSDL(SDL_Window *window, SDL_GLContext gl_context) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int showUI(Timing &timing) {
    timing.initialize();
    auto [window, gl_context] = openSDLWindow();
    if (!window) return 200;

    const ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsLight(); // set light color scheme, alt: ImGui::StyleColorsDark() ImGui::StyleColorsClassic();
    app_header::loadHeaderFont(13.f); // default font for the application
    auto headerFont = app_header::loadHeaderFont(32.f);

    bool imGuiDemo = false;
    bool imPlotDemo = false;
    bool done = false;
    while (!done) { // main ui loop
        timing.process();
        processSDLEvents(window, done);
        startImGuiFrame();
        // create imgui "window" filling the whole platform window in the background s.t. other imgui windows may be put in front of it
        static ImGuiWindowFlags imGuiWindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos);
        ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize);
        if (auto _ = ImScoped::Window("Example: Fullscreen window", nullptr, imGuiWindowFlags)) {
            app_header::draw_header_bar("Digitizer Timing Debug", headerFont);
            showTimingEventTable(timing);
            showTimingSchedule(timing);
            static TimePlot plot{timing.snooped};
            plot.updateStreaming();
            plot.display(timing);
            showTRConfig(timing, imGuiDemo, imPlotDemo);
        }
        if (imGuiDemo){
            ImGui::ShowDemoWindow(&imGuiDemo);
        }
        if (imPlotDemo) {
            ImPlot::ShowDemoWindow(&imPlotDemo);
        }
        RenderToSDL(io, window);
    } // main ui loop
    shutdownSDL(window, gl_context);
    return 0;
}

int main() {
    Timing timing; // an interface to the timing card allowing condition & io configuration and event injection & snooping
    return showUI(timing);
}
