#include <format>
#include <thread>
#include <iostream>
#include <cstdio>

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

#include <fmt/chrono.h>
#include <fmt/ranges.h>

#include "timing.hpp"
#include "ps4000a.hpp"

static auto tai_ns_to_utc(auto input) {
    return std::chrono::utc_clock::to_sys(std::chrono::tai_clock::to_utc(std::chrono::tai_clock::time_point{} + std::chrono::nanoseconds(input)));
}

void showTimingEventTable(gr::BufferReader auto &event_reader) {
    if (ImGui::Button("clear")) {
        event_reader.consume(event_reader.available());
    }
    if (ImGui::CollapsingHeader("Received Timing Events", ImGuiTreeNodeFlags_DefaultOpen)) {
        static int freeze_cols = 1;
        static int freeze_rows = 1;

        const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
        const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
        ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 20);
        static ImGuiTableFlags flags =
                ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable |
                ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
        if (ImGui::BeginTable("received_events", 19, flags, outer_size)) {
            ImGui::TableSetupScrollFreeze(freeze_cols, freeze_rows);
            ImGui::TableSetupColumn("timestamp", ImGuiTableColumnFlags_NoHide); // Make the first column not hideable to match our use of TableSetupScrollFreeze()
            ImGui::TableSetupColumn("executed at", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("gid");
            ImGui::TableSetupColumn("sid");
            ImGui::TableSetupColumn("bpid");
            ImGui::TableSetupColumn("eventno");
            ImGui::TableSetupColumn("beam-in");
            ImGui::TableSetupColumn("bpc-start");
            ImGui::TableSetupColumn("req no beam");
            ImGui::TableSetupColumn("virt acc");
            ImGui::TableSetupColumn("bpcid");
            ImGui::TableSetupColumn("bpcts");
            ImGui::TableSetupColumn("fid");
            ImGui::TableSetupColumn("id");
            ImGui::TableSetupColumn("param");
            ImGui::TableSetupColumn("reserved1", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("reserved2", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("reserved", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("flags");

            ImGui::TableHeadersRow();
            auto data = event_reader.get();

            for (auto &evt : data) {
                ImGui::TableNextRow();
                if (ImGui::TableSetColumnIndex(0)) { // time
                    ImGui::Text("%s", fmt::format("{}", tai_ns_to_utc(evt.time)).c_str());
                }
                if (ImGui::TableSetColumnIndex(1)) { // executed time
                    ImGui::Text("%s", fmt::format("{}", tai_ns_to_utc(evt.executed)).c_str());
                }
                if (ImGui::TableSetColumnIndex(6)) {
                    ImGui::Text("%s", fmt::format("{}", evt.gid).c_str());
                }
                if (ImGui::TableSetColumnIndex(12)) {
                    ImGui::Text("%s", fmt::format("{}", evt.sid).c_str());
                }
                if (ImGui::TableSetColumnIndex(13)) {
                    ImGui::Text("%s", fmt::format("{}", evt.bpid).c_str());
                }
                if (ImGui::TableSetColumnIndex(7)) {
                    ImGui::Text("%s", fmt::format("{}", evt.eventno).c_str());
                }
                if (ImGui::TableSetColumnIndex(8)) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, evt.flag_beamin ? ImGui::GetColorU32({0,255,0,0}) : ImGui::GetColorU32({255,0,0,0}));
                    ImGui::Text("%s", fmt::format("{}", evt.flag_beamin ? "y" : "n").c_str());
                }
                if (ImGui::TableSetColumnIndex(9)) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, evt.flag_bpc_start ? ImGui::GetColorU32({0,255,0,0}) : ImGui::GetColorU32({255,0,0,0}));
                    ImGui::Text("%s", fmt::format("{}", evt.flag_bpc_start ? "y" : "n").c_str());
                }
                if (ImGui::TableSetColumnIndex(15)) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, evt.reqNoBeam ? ImGui::GetColorU32({0,255,0,0}) : ImGui::GetColorU32({255,0,0,0}));
                    ImGui::Text("%s", fmt::format("{}", evt.reqNoBeam ? "y" : "n").c_str());
                }
                if (ImGui::TableSetColumnIndex(16)) {
                    ImGui::Text("%s", fmt::format("{}", evt.virtAcc).c_str());
                }
                if (ImGui::TableSetColumnIndex(17)) {
                    ImGui::Text("%s", fmt::format("{}", evt.bpcid).c_str());
                }
                if (ImGui::TableSetColumnIndex(18)) {
                    ImGui::Text("%s", fmt::format("{}", evt.bpcts).c_str());
                }
                if (ImGui::TableSetColumnIndex(5)) { // fid
                    ImGui::Text("%s", fmt::format("{}", evt.fid).c_str());
                }
                if (ImGui::TableSetColumnIndex(2)) { // id
                    ImGui::Text("%s", fmt::format("{:#x}", evt.id()).c_str());
                }
                if (ImGui::TableSetColumnIndex(3)) { // param
                    ImGui::Text("%s", fmt::format("{:#x}", evt.param()).c_str());
                }
                if (ImGui::TableSetColumnIndex(10)) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, evt.flag_reserved1 ? ImGui::GetColorU32({0,255,0,0}) : ImGui::GetColorU32({255,0,0,0}));
                    ImGui::Text("%s", fmt::format("{}", evt.flag_reserved1 ? "y" : "n").c_str());
                }
                if (ImGui::TableSetColumnIndex(11)) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, evt.flag_reserved2 ? ImGui::GetColorU32({0,255,0,0}) : ImGui::GetColorU32({255,0,0,0}));
                    ImGui::Text("%s", fmt::format("{}", evt.flag_reserved2 ? "y" : "n").c_str());
                }
                if (ImGui::TableSetColumnIndex(14)) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, evt.reserved ? ImGui::GetColorU32({0,255,0,0}) : ImGui::GetColorU32({255,0,0,0}));
                    ImGui::Text("%s", fmt::format("{}", evt.reserved ? "y" : "n").c_str());
                }
                if (ImGui::TableSetColumnIndex(4)) { // flags
                    // print flags
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, evt.flags ? ImGui::GetColorU32({0,255,0,0}) : ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_TableRowBg]));
                    auto delay = evt.executed - evt.time;
                    if (evt.flags & 1) {
                        ImGui::Text("%s", fmt::format(" !late (by {} ns)", delay).c_str());
                    } else if (evt.flags & 2) {
                        ImGui::Text("%s", fmt::format(" !early (by {} ns)", delay).c_str());
                    } else if (evt.flags & 4) {
                        ImGui::Text("%s", fmt::format(" !conflict (delayed by {} ns)", delay).c_str());
                    } else if (evt.flags & 8) {
                        ImGui::Text("%s", fmt::format(" !delayed (by {} ns)", delay).c_str());
                    }
                }
            }
            ImGui::EndTable();
            if (data.size() > event_reader.buffer().size() / 2) {
                std::ignore = event_reader.consume(data.size() - event_reader.buffer().size() / 2);
            }
        }
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
    static std::vector<Timing::event> events{};
    static enum class InjectState { STOPPED, RUNNING, ONCE, SINGLE } injectState = InjectState::STOPPED;
    if (ImGui::CollapsingHeader("Schedule to inject", ImGuiTreeNodeFlags_DefaultOpen)) {
        static uint64_t default_offset = 100000000; // 100ms
        ImGui::DragScalar("default event offset", ImGuiDataType_U64, &default_offset, 1.0f, &min_uint64, &max_uint64, "%d", ImGuiSliderFlags_None);
        ImGui::SameLine();
        if (ImGui::Button("+")) {
            events.emplace_back(default_offset + (events.empty() ? 0ul : events.end()->time));
        }
        ImGui::SameLine();
        if (ImGui::Button("start")) {
            current = 0;
            time_offset = timing.receiver->CurrentTime().getTAI();
            injectState = InjectState::RUNNING;
        }
        ImGui::SameLine();
        if (ImGui::Button("once")) {
            current = 0;
            time_offset = timing.receiver->CurrentTime().getTAI();
            injectState = InjectState::ONCE;
        }
        ImGui::SameLine();
        if (ImGui::Button("stop")) {
            injectState = InjectState::STOPPED;
        }
        // schedule table
        static int freeze_cols = 1;
        static int freeze_rows = 1;
        const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
        const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
        ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 15);
        static ImGuiTableFlags flags =
                ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable |
                ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
        if (ImGui::BeginTable("event schedule", 17, flags, outer_size)) {
            ImGui::TableSetupScrollFreeze(freeze_cols, freeze_rows);
            ImGui::TableSetupColumn("time", ImGuiTableColumnFlags_NoHide); // Make the first column not hideable to match our use of TableSetupScrollFreeze()
            ImGui::TableSetupColumn("id");
            ImGui::TableSetupColumn("param");
            ImGui::TableSetupColumn("fid");
            ImGui::TableSetupColumn("gid");
            ImGui::TableSetupColumn("eventno");
            ImGui::TableSetupColumn("beam-in");
            ImGui::TableSetupColumn("bpc-start");
            ImGui::TableSetupColumn("reserved1");
            ImGui::TableSetupColumn("reserved2");
            ImGui::TableSetupColumn("sid");
            ImGui::TableSetupColumn("bpid");
            ImGui::TableSetupColumn("reserved");
            ImGui::TableSetupColumn("req no beam");
            ImGui::TableSetupColumn("virt acc");
            ImGui::TableSetupColumn("bpcid");
            ImGui::TableSetupColumn("bpcts");

            ImGui::TableHeadersRow();

            for (auto &ev: events) {
                ImGui::PushID(&ev);
                ImGui::TableNextRow();
                if (ImGui::TableSetColumnIndex(0)) { // time
                    ImGui::DragScalar("##time", ImGuiDataType_U64, &ev.time, 1.0f, &min_uint64, &max_uint64, "%d", ImGuiSliderFlags_None);
                }
                if (ImGui::TableSetColumnIndex(1)) {
                    ImGui::Text("%s", fmt::format("{:#x}", ev.id()).c_str());
                }
                if (ImGui::TableSetColumnIndex(2)) {
                    ImGui::Text("%s", fmt::format("{:#x}", ev.param()).c_str());
                }
                if (ImGui::TableSetColumnIndex(3)) {
                    uint64_t fid = ev.fid;
                    ImGui::DragScalar("##fid", ImGuiDataType_U64, &fid, 1.0f, &min_uint64, &max_uint4, "%d",
                                      ImGuiSliderFlags_None);
                    ev.fid = fid & max_uint4;
                }
                if (ImGui::TableSetColumnIndex(4)) {
                    uint64_t gid = ev.gid;
                    ImGui::DragScalar("##gid", ImGuiDataType_U64, &gid, 1.0f, &min_uint64, &max_uint12, "%d",
                                      ImGuiSliderFlags_None);
                    ev.gid = gid & max_uint12;
                }
                if (ImGui::TableSetColumnIndex(5)) {
                    uint64_t eventno = ev.eventno;
                    ImGui::DragScalar("##eventno", ImGuiDataType_U64, &eventno, 1.0f, &min_uint64, &max_uint12, "%d",
                                      ImGuiSliderFlags_None);
                    ev.eventno = eventno & max_uint12;
                }
                if (ImGui::TableSetColumnIndex(6)) {
                    bool flag_beamin = ev.flag_beamin;
                    ImGui::Checkbox("##beamin", &flag_beamin);
                    ev.flag_beamin = flag_beamin;
                }
                if (ImGui::TableSetColumnIndex(7)) {
                    bool flag_bpc_start = ev.flag_bpc_start;
                    ImGui::Checkbox("##bpcstart", &flag_bpc_start);
                    ev.flag_bpc_start = flag_bpc_start;
                }
                if (ImGui::TableSetColumnIndex(8)) {
                    bool flag_reserved1 = ev.flag_reserved1;
                    ImGui::Checkbox("##reserved1", &flag_reserved1);
                    ev.flag_reserved1 = flag_reserved1;
                }
                if (ImGui::TableSetColumnIndex(9)) {
                    bool flag_reserved2 = ev.flag_reserved2;
                    ImGui::Checkbox("##reserved2", &flag_reserved2);
                    ev.flag_reserved2 = flag_reserved2;
                }
                if (ImGui::TableSetColumnIndex(10)) {
                    uint64_t sid = ev.sid;
                    ImGui::DragScalar("##sid", ImGuiDataType_U64, &sid, 1.0f, &min_uint64, &max_uint12, "%d",
                                      ImGuiSliderFlags_None);
                    ev.sid = sid & max_uint12;
                }
                if (ImGui::TableSetColumnIndex(11)) {
                    uint64_t bpid = ev.bpid;
                    ImGui::DragScalar("##pbid", ImGuiDataType_U64, &bpid, 1.0f, &min_uint64, &max_uint14, "%d",
                                      ImGuiSliderFlags_None);
                    ev.bpid = bpid & max_uint14;
                }
                if (ImGui::TableSetColumnIndex(12)) {
                    bool reserved = ev.reserved;
                    ImGui::Checkbox("##reserved", &reserved);
                    ev.reserved = reserved;
                }
                if (ImGui::TableSetColumnIndex(13)) {
                    bool reqNoBeam = ev.reqNoBeam;
                    ImGui::Checkbox("##reqNoBeam", &reqNoBeam);
                    ev.reqNoBeam = reqNoBeam;
                }
                if (ImGui::TableSetColumnIndex(14)) {
                    uint64_t virtAcc = ev.virtAcc;
                    ImGui::DragScalar("##virtAcc", ImGuiDataType_U64, &virtAcc, 1.0f, &min_uint64, &max_uint4, "%d",
                                      ImGuiSliderFlags_None);
                    ev.virtAcc = virtAcc & max_uint4;
                }
                if (ImGui::TableSetColumnIndex(15)) {
                    uint64_t bpcid = ev.bpcid;
                    ImGui::DragScalar("##bpcid", ImGuiDataType_U64, &bpcid, 1.0f, &min_uint64, &max_uint22, "%d",
                                      ImGuiSliderFlags_None);
                    ev.bpcid = bpcid & max_uint22;
                }
                if (ImGui::TableSetColumnIndex(16)) {
                    uint64_t bpcts = ev.bpcts;
                    ImGui::DragScalar("##bpcts", ImGuiDataType_U64, &bpcts, 1.0f, &min_uint64, &max_uint42, "%d",
                                      ImGuiSliderFlags_None);
                    ev.bpcts = bpcts & max_uint42;
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }
    // if running, schedule events up to 100ms ahead
    if (injectState == InjectState::RUNNING || injectState == InjectState::ONCE || injectState == InjectState::SINGLE) {
        while (events[current].time + time_offset < timing.receiver->CurrentTime().getTAI() + 500000000) {
            timing.receiver->InjectEvent(events[current].id(), events[current].param(), saftlib::makeTimeTAI(events[current].time + time_offset));
            if (injectState == InjectState::SINGLE) {
                injectState = InjectState::STOPPED;
                break;
            }
            if (current + 1 >= events.size()) {
                if (injectState == InjectState::ONCE) {
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

void showTRConfig(Timing &timing) {
    ImGui::SetNextItemOpen(false);
    if (ImGui::CollapsingHeader("Timing Receiver IO configuration", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (!timing.initialized) {
            ImGui::TextUnformatted("No timing receiver found");
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
        const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
        const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
        ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 24);
        static ImGuiTableFlags flags =
                ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable |
                ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
        if (ImGui::BeginTable("ioConfiguration", 8, flags, outer_size)) {
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

            for (auto &out :outputs) {
                ImGui::PushID(&out);
                ImGui::TableNextRow();
                auto port_proxy = saftlib::Output_Proxy::create(out.second);
                auto conditions = port_proxy->getAllConditions();
                if (ImGui::TableSetColumnIndex(0)) {
                    ImGui::Text("%s", fmt::format("{}", out.first).c_str());
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        // Table of ECA Conditions
        static int freeze_cols_conds = 1;
        static int freeze_rows_conds = 1;
        ImVec2 outer_size_conds = ImVec2(0.0f, TEXT_BASE_HEIGHT * 24);
        static ImGuiTableFlags flags_conds =
                ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable |
                ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
        if (ImGui::BeginTable("ioConfiguration", 8, flags_conds, outer_size_conds)) {
            ImGui::TableSetupScrollFreeze(freeze_cols_conds, freeze_rows_conds);
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

            for (auto &out :outputs) {
                ImGui::PushID(&out);
                ImGui::TableNextRow();
                auto port_proxy = saftlib::Output_Proxy::create(out.second);
                auto conditions = port_proxy->getAllConditions();
                if (ImGui::TableSetColumnIndex(0)) {
                    ImGui::Text("%s", fmt::format("{}", out.first).c_str());
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        // Table: EVENT | MASK | OFFSET | FLAGS (late1/early2/conflict4/delayed8) | on/off
    }
}

void showEBConsole(WBConsole &console) {
    ImGui::SetNextItemOpen(false);
    if (ImGui::CollapsingHeader("EtherBone Console", ImGuiTreeNodeFlags_CollapsingHeader)) {
    }
}

void showTimePlot(gr::BufferReader auto &picoscope_reader, gr::BufferReader auto &event_reader) {
    auto data = picoscope_reader.get();
    if (ImGui::CollapsingHeader("Plot", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImPlot::BeginPlot("timing markers", ImVec2(-1,0), ImPlotFlags_CanvasOnly)) {
            ImPlot::SetupAxes("x","y");
            ImPlot::PlotLine("IO1", data.data(), data.size());
            ImPlot::EndPlot();
        }
    }
    // remove as many items from the buffer s.t. it only half full so that new samples can be added
    if (data.size() > picoscope_reader.buffer().size() / 2) {
        std::ignore = picoscope_reader.consume(data.size() - picoscope_reader.buffer().size() / 2);
    }
}

int interactive(Ps4000a &digitizer, Timing &timing, WBConsole &console) {
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

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    gr::BufferReader auto event_reader = timing.snooped.new_reader();
    gr::BufferReader auto digitizer_reader = digitizer.buffer().new_reader();
    //gr::HistoryBuffer<Timing::event, 1000> snoop_history{};

    // Main loop
    bool done = false;
    while (!done) {
        digitizer.process();
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
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        static ImGuiWindowFlags imGuiWindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos);
        ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize);
        if (ImGui::Begin("Example: Fullscreen window", nullptr, imGuiWindowFlags)) {
            // TODO: include FAIR header
            showTimingEventTable(event_reader);
            showTimingSchedule(timing);
            showTRConfig(timing);
            showEBConsole(console);
            showTimePlot(digitizer_reader, event_reader);
        }
        ImGui::End();

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
    Ps4000a picoscope; // a picoscope to examine generated timing signals
    Timing timing; // an interface to the timing card allowing condition & io configuration and event injection & snooping
    WBConsole console; // a whishbone console to send raw commands to the timing card and switch it beteween master and slave mode

    CLI::App app{"timing receiver saftbus example"};
    bool scope = false;
    app.add_flag("--scope", scope, "enter interactive scope mode showing the timing input connected to a picoscope and the generated and received timing msgs");
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
        return interactive(picoscope, timing, console);
    }

    return 0;
}
