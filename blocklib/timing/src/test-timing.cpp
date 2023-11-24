#include <format>
#include <thread>
#include <iostream>
#include <cstdio>
#include <unordered_set>
#include <ranges>
#include <string>

// CLI - interface
#include <CLI/CLI.hpp>

// UI
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include "ImScoped.hpp"

#include <fmt/chrono.h>
#include <fmt/ranges.h>
#include <OutputCondition_Proxy.hpp>

#include "timing.hpp"
#include "fair_header.h"
#include "plot.hpp"
#include "event_definitions.hpp"


static auto tai_ns_to_utc(auto input) {
    return std::chrono::utc_clock::to_sys(std::chrono::tai_clock::to_utc(std::chrono::tai_clock::time_point{} + std::chrono::nanoseconds(input)));
}

template <typename... T>
void tableColumnString(fmt::format_string<T...> &&fmt, T&&... args) {
    if (ImGui::TableNextColumn()) {
        ImGui::TextUnformatted(fmt::vformat(fmt, fmt::make_format_args(args...)).c_str());
    }
}
template <typename... T>
void tableColumnStringColor(ImColor color, fmt::format_string<T...> &&fmt, T&&... args) {
if (ImGui::TableNextColumn()) {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, color);
        ImGui::TextUnformatted(fmt::vformat(fmt, fmt::make_format_args(args...)).c_str());
    }
}
void tableColumnBool(bool state, ImColor trueColor, ImColor falseColor) {
    if (ImGui::TableNextColumn()) {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, state ? trueColor : falseColor);
        ImGui::TextUnformatted(fmt::format("{}", state ? "y" : "n").c_str());
    }
}
template <typename T>
void tableColumnSlider(const std::string &id, T &field, const uint64_t &max, float width) {
    static constexpr uint64_t min_uint64 = 0;
    if (ImGui::TableNextColumn()) {
        ImGui::SetNextItemWidth(width);
        uint64_t tmp = field;
        ImGui::DragScalar(id.c_str(), ImGuiDataType_U64, &tmp, 1.0f, &min_uint64, &max, "%d", ImGuiSliderFlags_None);
        field = tmp & max;
    }
}
void tableColumnCheckbox(const std::string &id, bool &field) {
    if (ImGui::TableNextColumn()) {
        bool flag_beamin = field;
        ImGui::Checkbox(id.c_str(), &flag_beamin);
        field = flag_beamin;
    }
}

void showTimingEventTable(gr::BufferReader auto &event_reader) {
    if (ImGui::Button("clear")) {
        std::ignore = event_reader.consume(event_reader.available());
    }
    if (ImGui::CollapsingHeader("Received Timing Events", ImGuiTreeNodeFlags_DefaultOpen)) {
        static int freeze_cols = 1;
        static int freeze_rows = 1;

        static std::map<uint16_t, ImColor> colors;

        const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
        const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
        ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 20);
        static ImGuiTableFlags flags =
                ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
                ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
        if (auto _ = ImScoped::Table("received_events", 19, flags, outer_size, 0.f)) {
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

            ImGui::TableHeadersRow();
            auto data = event_reader.get();

            for (auto &evt : std::ranges::reverse_view{data}) {
                ImGui::TableNextRow();
                tableColumnString("{}", tai_ns_to_utc(evt.time));
                tableColumnString("{}", tai_ns_to_utc(evt.executed));
                tableColumnString("{}", evt.bpcid);
                tableColumnString("{}", evt.sid);
                tableColumnStringColor(getStableColor(evt.bpid, colors, bpcidColors, ImGui::GetStyle().Colors[ImGuiCol_TableRowBg]),"{}", evt.bpid);
                tableColumnString("{1}({0})", evt.gid, timingGroupTable.contains(evt.gid) ? timingGroupTable.at(evt.gid).first : "UNKNOWN");
                tableColumnString("{1}({0})", evt.eventno, eventNrTable.contains(evt.eventno) ? eventNrTable.at(evt.eventno).first : "UNKNOWN");
                tableColumnBool(evt.flag_beamin, ImGui::GetColorU32({0,1.0,0,0.4f}), ImGui::GetColorU32({1.0,0,0,0.4f}));
                tableColumnBool(evt.flag_bpc_start, ImGui::GetColorU32({0,1.0,0,0.4f}), ImGui::GetColorU32({1.0,0,0,0.4f}));
                tableColumnBool(evt.reqNoBeam, ImGui::GetColorU32({0,1.0,0,0.4f}), ImGui::GetColorU32({1.0,0,0,0.4f}));
                tableColumnString("{}", evt.virtAcc);
                tableColumnString("{}", evt.bpcts);
                tableColumnString("{}", evt.fid);
                tableColumnString("{:#08x}", evt.id());
                tableColumnString("{:#08x}", evt.param());
                tableColumnBool(evt.flag_reserved1, ImGui::GetColorU32({0,1.0,0,0.4f}), ImGui::GetColorU32({1.0,0,0,0.4f}));
                tableColumnBool(evt.flag_reserved2, ImGui::GetColorU32({0,1.0,0,0.4f}), ImGui::GetColorU32({1.0,0,0,0.4f}));
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
            }
            if (data.size() > event_reader.buffer().size() / 2) {
                std::ignore = event_reader.consume(data.size() - event_reader.buffer().size() / 2);
            }
        }
    }
}

template<typename T>
ImColor getStableColor(T id, std::map<T, ImColor> &colors, std::span<const ImColor> colormap, ImColor defaultColor) {
    if (colors.contains(id)) {
        return colors[id];
    } else {
        if (colormap.size() > colors.size()) {
            colors.insert({id, ImColor{colormap[colors.size()]}});
            return colors[id];
        }
    }
    return defaultColor;
}

void loadEventsFromString(std::vector<Timing::event> &events, std::string_view string) {
    events.clear();
    using std::operator""sv;
    try {
        for (auto line: std::views::split(string, "\n"sv)) {
            std::string_view line_sv(line);
            if (line_sv.empty()) continue;
            std::array<uint64_t, 3> numbers{};
            for (auto [i, number]: std::views::split(line_sv, " "sv) | std::views::enumerate) {
                std::string_view number_sv(number);
                numbers[i] = std::stoul(std::string(number_sv), nullptr, 0);
            }
            events.emplace_back(numbers[2], numbers[0], numbers[1]);
        }
    } catch (std::exception &e) {
        events.clear();
        fmt::print("Error parsing clipboard data: {}", string);
    }

}

void showTimingSchedule(Timing &timing) {
    static constexpr uint64_t min_uint64 = 0;
    static constexpr uint64_t max_uint64 = std::numeric_limits<uint64_t>::max();
    static constexpr uint64_t max_uint42 = (1ul << 42) - 1;
    static constexpr uint64_t max_uint22 = (1ul << 22) - 1;
    static constexpr uint64_t max_uint14 = (1ul << 14) - 1;
    static constexpr uint64_t max_uint12 = (1ul << 12) - 1;
    static constexpr uint64_t max_uint4 = (1ul << 4) - 1;

    static std::size_t current = 0;
    static uint64_t time_offset = 0;
    static std::vector<Timing::event> events = {};
    static enum class InjectState { STOPPED, RUNNING, SINGLE } injectState = InjectState::STOPPED;
    if (ImGui::CollapsingHeader("Schedule to inject", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SetNextItemWidth(80.f);
        static uint64_t default_offset = 100000000ul; // 100ms
        ImGui::DragScalar("default event offset", ImGuiDataType_U64, &default_offset, 1.0f, &min_uint64, &max_uint64, "%d", ImGuiSliderFlags_None);
        ImGui::SameLine();
        if (ImGui::Button("+")) {
            events.emplace_back(default_offset + (events.empty() ? 0ul : events.back().time));
        }
        ImGui::SameLine(0.f, 10.f);
        if (ImGui::Button("Clear##schedule")) {
            events.clear();
        }
        ImGui::SameLine();
        ImGui::Button("Load");
        if (ImGui::BeginPopupContextItem("load schedule popup", ImGuiPopupFlags_MouseButtonLeft)) {
            if (ImGui::Button("from Clipboard")) {
                loadEventsFromString(events, ImGui::GetClipboardText());
            }
            for (const auto & [scheduleName, schedule] : demoSchedules) {
                if (ImGui::Button(scheduleName.c_str())) {
                    loadEventsFromString(events, schedule);
                }
            }
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save to Clipboard")) {
            std::string string;
            for (auto &ev: events) {
                string.append(fmt::format("{:#x} {:#x} {}\n", ev.id(), ev.param(), ev.time));
            }
            ImGui::SetClipboardText(string.c_str());
        }
        // set state
        ImGui::SameLine(0.f, 10.f);
        if (injectState == InjectState::RUNNING || injectState == InjectState::SINGLE) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.4f, 0.4f, 1.0f,1.0f});
            if (ImGui::Button("Stop###StartStop")) {
                current = 0;
                time_offset = timing.receiver->CurrentTime().getTAI();
                injectState = InjectState::STOPPED;
            }
            ImGui::PopStyleColor();
        } else {
            if (ImGui::Button("Start###StartStop")) {
                current = 0;
                time_offset = timing.receiver->CurrentTime().getTAI();
                injectState = InjectState::RUNNING;
            }
        }
        ImGui::SameLine();
        {
            auto _ = ImScoped::Disabled(injectState != InjectState::STOPPED);
            if (ImGui::Button("Single")) {
                current = 0;
                time_offset = timing.getTAI();
                injectState = InjectState::SINGLE;
            }
        }
        // schedule table
        static int freeze_cols = 1;
        static int freeze_rows = 1;
        const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
        ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 15);
        static ImGuiTableFlags flags =
                ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
                ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
        {
            auto _ = ImScoped::Disabled(injectState != InjectState::STOPPED);
            if (auto _ = ImScoped::Table("event schedule", 22, flags, outer_size, 0.f)) {
                ImGui::TableSetupScrollFreeze(freeze_cols, freeze_rows);
                ImGui::TableSetupColumn("time",
                                        ImGuiTableColumnFlags_NoHide); // Make the first column not hideable to match our use of TableSetupScrollFreeze()
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
                ImGui::TableSetupColumn("Trigger Outputs", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Trigger Delay", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Trigger Flat-Top", ImGuiTableColumnFlags_NoHide);

                ImGui::TableHeadersRow();

                events.erase(std::remove_if(events.begin(), events.end(),
                                            [&timing, default_offset = default_offset](
                                                    auto &ev) {
                                                bool to_remove = false;
                                                ImGui::PushID(&ev);
                                                ImGui::TableNextRow();
                                                tableColumnSlider("##time", ev.time, max_uint64, 80.f);
                                                tableColumnSlider("##bpcid", ev.bpcid, max_uint22, 40.f);
                                                tableColumnSlider("##sid", ev.sid, max_uint12, 40.f);
                                                tableColumnSlider("##pbid", ev.bpid, max_uint14, 40.f);
                                                if (ImGui::TableNextColumn()) {
                                                    ImGui::SetNextItemWidth(80.f);
                                                    std::size_t textlen;
                                                    uint64_t tmp = ev.gid;
                                                    ImGui::DragScalar("##gid", ImGuiDataType_U64, &tmp, 1.0f,
                                                                      &min_uint64, &max_uint12, "%d",
                                                                      ImGuiSliderFlags_None);
                                                    // trying to make selection of group-ids and events more user-friendly
                                                    //std::array<char, 32> buf{};
                                                    //ImGui::InputText("##gidtxt", buf.data(), buf.size(), 0, [](ImGuiInputTextCallbackData *edit) {
                                                    //    fmt::print("{}\n", edit->Buf[0]);
                                                    //    *((std::size_t*) edit->UserData) = edit->BufTextLen;
                                                    //    return 0;
                                                    //}, &textlen);
                                                    //if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
                                                    //    auto _ = ImScoped::Tooltip();
                                                    //    std::size_t parsed;
                                                    //    long number = std::stol(std::string{buf.data(), buf.size()}, &parsed);
                                                    //    std::vector<uint16_t> available;
                                                    //    if (parsed == textlen) { // is a number
                                                    //        available.push_back(number);
                                                    //    } else { // search for the string in enum
                                                    //        for (auto & [id, pair] : timingGroupTable) {
                                                    //            auto & [name, _] = pair;
                                                    //            if (name.contains(std::string(buf.data(), std::min(textlen, buf.size())))) {
                                                    //                available.push_back(number);
                                                    //            }
                                                    //        }
                                                    //    }
                                                    //    int listCurrent = 0;
                                                    //    if (auto _ = ImScoped::ListBox("##gidList", ImVec2{0.f,0.f})) {
                                                    //        for
                                                    //        ImGuiListClipper clipper;
                                                    //        clipper.Begin(items_count, GetTextLineHeightWithSpacing()); // We know exactly our line height here so we pass it as a minor optimization, but generally you don't need to.
                                                    //        while (clipper.Step())
                                                    //            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                                                    //            {
                                                    //                const char* item_text;
                                                    //                if (!items_getter(data, i, &item_text))
                                                    //                    item_text = "*Unknown item*";

                                                    //                PushID(i);
                                                    //                const bool item_selected = (i == *current_item);
                                                    //                if (Selectable(item_text, item_selected))
                                                    //                {
                                                    //                    *current_item = i;
                                                    //                    value_changed = true;
                                                    //                }
                                                    //                if (item_selected)
                                                    //                    SetItemDefaultFocus();
                                                    //                PopID();
                                                    //            }
                                                    //    }
                                                    //}
                                                    ev.gid = tmp & max_uint12;
                                                }
                                                tableColumnSlider("##eventno", ev.eventno, max_uint12, 40.f);
                                                tableColumnCheckbox("##beamin", ev.flag_beamin);
                                                tableColumnCheckbox("##bpcstart", ev.flag_bpc_start);
                                                tableColumnCheckbox("##reqNoBeam", ev.reqNoBeam);
                                                tableColumnSlider("##virtAcc", ev.virtAcc, max_uint4, 40.f);
                                                tableColumnSlider("##bpcts", ev.bpcts, max_uint42, 80.f);
                                                tableColumnSlider("##fid", ev.fid, max_uint4, 40.f);
                                                tableColumnString("{:#08x}", ev.id());
                                                tableColumnString("{:#08x}", ev.param());
                                                tableColumnCheckbox("##reserved1", ev.flag_reserved1);
                                                tableColumnCheckbox("##reserved2", ev.flag_reserved2);
                                                tableColumnCheckbox("##reserved", ev.reserved);
                                                // interactive settings
                                                ImGui::TableNextColumn();
                                                if (ImGui::Button("remove")) {
                                                    to_remove = true;
                                                }
                                                ImGui::TableNextColumn();
                                                if (ImGui::Button("inject")) {
                                                    timing.injectEvent(ev, timing.getTAI() + default_offset);
                                                }
                                                auto trigger = timing.triggers.contains(ev.id()) ? std::optional<Timing::Trigger>{timing.triggers[ev.id()]} : std::optional<Timing::Trigger>{};
                                                ImGui::TableNextColumn();
                                                for (auto & [i, name, _] : timing.outputs) {
                                                    if (trigger) {
                                                        ImGui::Checkbox(fmt::format("##{}", name).c_str(), &trigger->outputs[i]);
                                                        ImGui::SameLine();
                                                        // TODO:: add tooltip
                                                    } else {
                                                        bool output_selected = false;
                                                        ImGui::Checkbox(fmt::format("##{}", name).c_str(), &output_selected);
                                                        ImGui::SameLine();
                                                        // TODO:: add tooltip
                                                        if (output_selected) {
                                                            trigger = Timing::Trigger{};
                                                        }
                                                    }
                                                }
                                                ImGui::TableNextColumn();
                                                if (trigger) {
                                                    ImGui::SetNextItemWidth(80);
                                                    ImGui::DragScalar("delay", ImGuiDataType_U64, &trigger->delay, 1.0f, &min_uint64, &max_uint64, "%d", ImGuiSliderFlags_None);
                                                }
                                                ImGui::TableNextColumn();
                                                if (trigger) {
                                                    ImGui::SetNextItemWidth(80);
                                                    ImGui::DragScalar("flattop", ImGuiDataType_U64, &trigger->flattop, 1.0f, &min_uint64, &max_uint64, "%d", ImGuiSliderFlags_None);
                                                }
                                                if (trigger) {
                                                    timing.updateTrigger(*trigger);
                                                }
                                                ImGui::PopID();
                                                return to_remove;
                                            }), events.end());
            }
        }
    }
    // if running, schedule events up to 500ms ahead
    if (injectState == InjectState::RUNNING || injectState == InjectState::SINGLE) {
        while (events[current].time + time_offset < timing.getTAI() + 500000000ul) {
            auto ev = events[current];
            timing.injectEvent(ev, time_offset);
            if (injectState == InjectState::SINGLE) {
                injectState = InjectState::STOPPED;
                break;
            }
            if (current + 1 >= events.size()) {
                if (injectState == InjectState::SINGLE) {
                    injectState = InjectState::STOPPED;
                    break;
                } else {
                    time_offset += events[current].time;
                    current = 0;
                }
            } else {
                ++current;
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
        auto trTime = timing.receiver->CurrentTime().getTAI();
        ImGui::TextUnformatted(fmt::format("{} -- ({} ns)\nTemperature: {}°C,\nGateware: {},\n(\"version\", \"{}\")",
                                           trTime, tai_ns_to_utc(trTime),
                                           timing.receiver->CurrentTemperature(),
                                           fmt::join(timing.receiver->getGatewareInfo(), ",\n"),
                                           timing.receiver->getGatewareVersion()).c_str());
        // print table of io info
        static int freeze_cols = 1;
        static int freeze_rows = 1;
        const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
        ImVec2 outer_size = ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, TEXT_BASE_HEIGHT * 10);
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

            for (auto & [name, port]:outputs) {
                ImGui::PushID(&name);
                ImGui::TableNextRow();
                auto port_proxy = saftlib::Output_Proxy::create(port);
                auto input_name = port_proxy->getInput();
                auto input_proxy = input_name.empty() ? std::shared_ptr<saftlib::Input_Proxy>{} : saftlib::Input_Proxy::create(input_name);
                tableColumnString("{}", name);
                tableColumnString(""); //{}", port_proxy->direction);
                tableColumnString("{}", port_proxy->getOutputEnable());
                tableColumnString("{}", input_proxy ? input_proxy->getInputTermination() : false);
                tableColumnString("{}", input_proxy ? port_proxy->getSpecialPurposeOut() : false);
                tableColumnString("{}", input_proxy ? input_proxy->getSpecialPurposeIn() : false);
                tableColumnString("{}", input_proxy ? input_proxy->getResolution() : false);
                tableColumnString("{}", port_proxy->getLogicLevelOut());
                ImGui::PopID();
            }
        }
        // Table of ECA Conditions
        ImGui::SameLine();
        static int freeze_cols_conds = 1;
        static int freeze_rows_conds = 1;
        ImVec2 outer_size_conds = ImVec2(0.0f, TEXT_BASE_HEIGHT * 10);
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

            for (auto &[name, port] : timing.receiver->getOutputs()) {
                auto port_proxy = saftlib::Output_Proxy::create(port);
                for (auto & condition : port_proxy->getAllConditions()) {
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
}

template<gr::Buffer BufferT>
class TimePlot {
public:
    enum class Mode {TRIGGERED, STREAMING, SNAPSHOT} mode = Mode::STREAMING;
    std::size_t duration = 5000000000000ul; // default duration to show in streaming/snapshot mode
    std::vector<uint16_t> contextStartEvents{}; // Event types that should trigger an update of the context
    std::vector<uint16_t> singleEventFilter{}; // event types that should be displayed as individual labeled lines
    bool show = true; // controls if the plot is expanded or collapsed
    using Reader = decltype(std::declval<BufferT>().new_reader());
private:
    gr::HistoryBuffer<Timing::event, 10000> contextEvents{};
    gr::HistoryBuffer<Timing::event, 10000> freestandingEvents{};
    Reader snoopReader;
    // buffer reader
public:
    explicit TimePlot(BufferT &events) : snoopReader{events.new_reader()} { }

    void clear() {
        // it seems like the history buffer does not support clearing its state (or assigning an empty buffer)...
    }

    void updateTriggered() {

    }

    void updateStreaming() {
        std::optional<Timing::event> last = contextEvents.size() == 0 ? std::optional<Timing::event>{} : contextEvents[contextEvents.size()-1];
        auto data = snoopReader.get();
    }

    void computeAndDisplay(Timing &timing) {
        auto d = snoopReader.get();
        double plot_depth = 20; // [ms] = 5 s
        double time = timing.getTAI() * 1e-9;
        if (ImGui::CollapsingHeader("Plot", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImPlot::BeginPlot("timing markers", ImVec2(-1,0), ImPlotFlags_CanvasOnly)) {
                ImPlot::SetupAxes("x","y");
                ImPlot::SetupAxisLimits(ImAxis_X1, time, time - plot_depth, ImGuiCond_Always);

                ImPlot::PushStyleVar(ImPlotStyleVar_DigitalBitHeight, 16.0f);
                ImPlot::PushStyleColor(ImPlotCol_Line, {1.0,0,0,0.6f});
                ImPlot::PushStyleColor(ImPlotCol_Fill, {0,1.0,0,0.6f});
                ImPlot::PlotStatusBarG("beamin", [](int i, void* data) {auto ev = ((Timing::event*) data)[i];return ImPlotPoint{ev.time * 1e-9, ev.flag_beamin * 1.0};}, ((void *) d.data()), d.size(), ImPlotStatusBarFlags_Bool);
                ImPlot::PopStyleColor(2);

                ImPlot::PlotStatusBarG("pbcid", [](int i, void* data) {auto ev = ((Timing::event*) data)[i];return ImPlotPoint{ev.time * 1e-9, ev.bpcid * 1.0};}, ((void *) d.data()), d.size(), ImPlotStatusBarFlags_Discrete);

                ImPlot::PushStyleColor(ImPlotCol_Line, {0.7f,0.2f,0,0.6f});
                ImPlot::PushStyleColor(ImPlotCol_Fill, {0.2f,0.1f,0,0.6f});
                ImPlot::PlotStatusBarG("pbcid", [](int i, void* data) {auto ev = ((Timing::event*) data)[i];return ImPlotPoint{ev.time * 1e-9, ev.sid * 1.0};}, ((void *) d.data()), d.size(), ImPlotStatusBarFlags_Alternate);
                ImPlot::PopStyleColor(2);

                ImPlot::PushStyleColor(ImPlotCol_Line, {0,0.7f,0.2f,0.6f});
                ImPlot::PushStyleColor(ImPlotCol_Fill, {0,0.2f,0.7f,0.6f});
                ImPlot::PlotStatusBarG("pbid", [](int i, void* data) {auto ev = ((Timing::event*) data)[i];return ImPlotPoint{ev.time * 1e-9, ev.bpid * 1.0};}, ((void *) d.data()), d.size(), ImPlotStatusBarFlags_Alternate);
                ImPlot::PopStyleColor(2);
                ImPlot::PopStyleVar();

                // plot non-beam process events

                ImPlot::EndPlot();
            }
        }
    }
};

int interactive(Timing &timing) {
    // Initialize UI
    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
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
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImGui::StyleColorsLight();

    auto defaultFont = app_header::loadHeaderFont(13.f);
    auto headerFont = app_header::loadHeaderFont(32.f);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    gr::BufferReader auto event_reader = timing.snooped.new_reader();

    bool imGuiDemo = false;
    bool imPlotDemo = false;

    // Main loop
    bool done = false;
    while (!done) {
        //console.process();
        timing.process();

        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                done = true;
            } else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window)) {
                done = true;
            }
        }
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        static ImGuiWindowFlags imGuiWindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos);
        ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize);
        if (auto _ = ImScoped::Window("Example: Fullscreen window", nullptr, imGuiWindowFlags)) {
            // TODO: include FAIR header
            app_header::draw_header_bar("Digitizer Timing Debug", headerFont);
            showTimingEventTable(event_reader);
            showTimingSchedule(timing);
            static TimePlot plot{timing.snooped};
            plot.computeAndDisplay(timing);
            showTRConfig(timing, imGuiDemo, imPlotDemo);
        }
        if (imGuiDemo){
            ImGui::ShowDemoWindow(&imGuiDemo);
        }
        if (imPlotDemo) {
            ImPlot::ShowDemoWindow(&imPlotDemo);
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void draw_plot() {
}

int main(int argc, char** argv) {
    Timing timing; // an interface to the timing card allowing condition & io configuration and event injection & snooping

    CLI::App app{"timing receiver saftbus example"};
    bool scope = false;
    app.add_flag("--scope", scope, "enter interactive scope mode showing the timing input connected to a picoscope and the generated and received timing msgs");
    app.add_flag("--simulate", timing.simulate, "mock the timing card to test the gui by directly inserting the scheduled events into the snooped events");
    app.add_flag("--pps", timing.ppsAlign, "add time to next pps pulse");
    app.add_flag("--abs", timing.absoluteTime, "time is an absolute time instead of an offset");
    app.add_flag("--utc", timing.UTC, "absolute time is in utc, default is tai");
    app.add_flag("--leap", timing.UTCleap, "utc calculation leap second flag");

    bool snoop = false;
    app.add_flag("-s", snoop, "snoop");
    app.add_option("-f", timing.snoopID, "id filter");
    app.add_option("-m", timing.snoopMask, "snoop mask");
    app.add_option("-o", timing.snoopOffset, "snoop offset");

    CLI11_PARSE(app, argc, argv);

    if (scope) {
        fmt::print("entering interactive timing scope mode\n");
        return interactive(timing);
    }

    return 0;
}
