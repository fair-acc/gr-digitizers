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
#include "fair_header.h"

static auto tai_ns_to_utc(auto input) {
    return std::chrono::utc_clock::to_sys(std::chrono::tai_clock::to_utc(std::chrono::tai_clock::time_point{} + std::chrono::nanoseconds(input)));
}

template <typename... T>
void tableColumnString(fmt::format_string<T...> &&fmt, T&&... args) {
    if (ImGui::TableNextColumn()) {
        ImGui::Text("%s", fmt::vformat(fmt, fmt::make_format_args(args...)).c_str());
    }
}
void tableColumnBool(bool state, ImColor trueColor, ImColor falseColor) {
    if (ImGui::TableNextColumn()) {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, state ? trueColor : falseColor);
        ImGui::Text("%s", fmt::format("{}", state ? "y" : "n").c_str());
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
    // colormap for timing ids
    static std::array<ImColor, 20> colorlist { // gemerated with http://medialab.github.io/iwanthue/
            ImColor{78,172,215,120}, ImColor{200,76,41,120}, ImColor{85,199,102,120}, ImColor{186,84,191,120},
            ImColor{101,173,51,120}, ImColor{117,98,204,120}, ImColor{169,180,56,120}, ImColor{211,76,146,120},
            ImColor{71,136,57,120}, ImColor{209,69,88,120}, ImColor{86,192,158,120}, ImColor{217,132,45,120},
            ImColor{102,125,198,120}, ImColor{201,160,65,120}, ImColor{199,135,198,120}, ImColor{156,176,104,120},
            ImColor{168,85,112,120}, ImColor{55,132,95,120}, ImColor{203,126,93,120}, ImColor{118,109,41,120},
    };

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
        if (ImGui::BeginTable("received_events", 19, flags, outer_size)) {
            ImGui::TableSetupScrollFreeze(freeze_cols, freeze_rows);
            ImGui::TableSetupColumn("timestamp", ImGuiTableColumnFlags_NoHide); // Make the first column not hideable to match our use of TableSetupScrollFreeze()
            ImGui::TableSetupColumn("executed at", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("bpcid");
            ImGui::TableSetupColumn("gid");
            ImGui::TableSetupColumn("sid");
            ImGui::TableSetupColumn("bpid");
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
                tableColumnString("{}", evt.gid);
                tableColumnString("{}", evt.bpcid);
                tableColumnString("{}", evt.sid);
                if (ImGui::TableNextColumn()) {
                    if (colors.contains(evt.bpid)) {
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, colors[evt.bpid]);
                    } else {
                        if (colorlist.size() > colors.size()) {
                            colors.insert({evt.bpid, ImColor{colorlist[colors.size()]}});
                            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, colors[evt.bpid]);
                        }
                    }
                    ImGui::Text("%s", fmt::format("{}", evt.bpid).c_str());
                }
                tableColumnString("{}", evt.eventno);
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
        ImGui::SetNextItemWidth(80.f);
        static uint64_t default_offset = 100000000ul; // 100ms
        ImGui::DragScalar("default event offset", ImGuiDataType_U64, &default_offset, 1.0f, &min_uint64, &max_uint64, "%d", ImGuiSliderFlags_None);
        ImGui::SameLine();
        if (ImGui::Button("+")) {
            events.emplace_back(default_offset + (events.empty() ? 0ul : events.back().time));
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
        const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
        ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 15);
        static ImGuiTableFlags flags =
                ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
                ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
        ImGui::BeginDisabled(injectState != InjectState::STOPPED);
        if (ImGui::BeginTable("event schedule", 17, flags, outer_size)) {
            ImGui::TableSetupScrollFreeze(freeze_cols, freeze_rows);
            ImGui::TableSetupColumn("time", ImGuiTableColumnFlags_NoHide); // Make the first column not hideable to match our use of TableSetupScrollFreeze()
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
            ImGui::TableSetupColumn("id", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("param", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("reserved1", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("reserved2", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("reserved", ImGuiTableColumnFlags_DefaultHide);

            ImGui::TableHeadersRow();

            for (auto &ev: events) {
                ImGui::PushID(&ev);
                ImGui::TableNextRow();
                tableColumnSlider("##time", ev.time, max_uint64, 80.f);
                tableColumnSlider("##gid", ev.gid, max_uint12,20.f);
                tableColumnSlider("##sid", ev.sid, max_uint12, 20.f);
                tableColumnSlider("##pbid", ev.bpid, max_uint14, 20.f);
                tableColumnSlider("##eventno", ev.eventno, max_uint12, 20.f);
                tableColumnCheckbox("##beamin", ev.flag_beamin);
                tableColumnCheckbox("##bpcstart",ev.flag_bpc_start);
                tableColumnCheckbox("##reqNoBeam",ev.reqNoBeam);
                tableColumnSlider("##virtAcc", ev.virtAcc, max_uint4, 20.f);
                tableColumnSlider("##bpcid",ev.bpcid, max_uint22, 40.f);
                tableColumnSlider("##bpcts",ev.bpcts, max_uint42, 40.f);
                tableColumnSlider("##fid",ev.fid, max_uint4, 20.f);
                tableColumnString("{:#08x}", ev.id());
                tableColumnString("{:#08x}", ev.param());
                tableColumnCheckbox("##reserved1",ev.flag_reserved1);
                tableColumnCheckbox("##reserved2",ev.flag_reserved2);
                tableColumnCheckbox("##reserved",ev.reserved);
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::EndDisabled();
    }
    // if running, schedule events up to 100ms ahead
    if (injectState == InjectState::RUNNING || injectState == InjectState::ONCE || injectState == InjectState::SINGLE) {
        while (events[current].time + time_offset < timing.receiver->CurrentTime().getTAI() + 500000000ul) {
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
    ImGui::SetNextItemOpen(false, ImGuiCond_Once);
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
        const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
        ImVec2 outer_size = ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, TEXT_BASE_HEIGHT * 24);
        static ImGuiTableFlags flags =
                ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
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
        ImGui::SameLine();
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
    ImGui::SetNextItemOpen(false, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("EtherBone Console", ImGuiTreeNodeFlags_CollapsingHeader)) {
    }
}

void showTimePlot(gr::BufferReader auto &picoscope_reader, Timing &timing, gr::BufferReader auto &event_reader) {
    auto data = picoscope_reader.get();
    double plot_depth = 20; // [ms] = 5 s
    double time = timing.receiver->CurrentTime().getTAI() * 1e-9;
    if (ImGui::CollapsingHeader("Plot", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImPlot::BeginPlot("timing markers", ImVec2(-1,0), ImPlotFlags_CanvasOnly)) {
            ImPlot::SetupAxes("x","y");
            ImPlot::SetupAxisLimits(ImAxis_X1, time, time - plot_depth, ImGuiCond_Always);
            static std::vector<double> lines;
            lines.clear();
            for (auto &ev: event_reader.get()) {
                double evTime = ev.time * 1e-9;
                if (evTime < time && evTime > time - plot_depth) {
                    lines.push_back(evTime);
                }
            }
            ImPlot::PlotInfLines("Vertical", lines.data(), lines.size());
            // TODO: reenable picoscope data
            //ImPlot::PlotLine("IO1", data.data(), data.size());
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
    ImGui::StyleColorsLight();

    auto defaultFont = app_header::loadHeaderFont(13.f);
    auto headerFont = app_header::loadHeaderFont(32.f);

    // Our state
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
        static ImGuiWindowFlags imGuiWindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos);
        ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize);
        if (ImGui::Begin("Example: Fullscreen window", nullptr, imGuiWindowFlags)) {
            // TODO: include FAIR header
            app_header::draw_header_bar("Digitizer Timing Debug", headerFont);
            showTimingEventTable(event_reader);
            showTimingSchedule(timing);
            showTRConfig(timing);
            showTimePlot(digitizer_reader, timing, event_reader);
            showEBConsole(console);
        }
        ImGui::End();

        ImGui::ShowDemoWindow();
        ImPlot::ShowDemoWindow();

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
