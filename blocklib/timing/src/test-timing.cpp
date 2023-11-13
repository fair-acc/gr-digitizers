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
    if (ImGui::CollapsingHeader("Received Timing Events", ImGuiTreeNodeFlags_DefaultOpen)) {
        static int freeze_cols = 1;
        static int freeze_rows = 1;

        const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
        const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
        ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 24);
        static ImGuiTableFlags flags =
                ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable |
                ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
        if (ImGui::BeginTable("received_events", 13, flags, outer_size)) {
            ImGui::TableSetupScrollFreeze(freeze_cols, freeze_rows);
            ImGui::TableSetupColumn("time", ImGuiTableColumnFlags_NoHide); // Make the first column not hideable to match our use of TableSetupScrollFreeze()
            ImGui::TableSetupColumn("executed");
            ImGui::TableSetupColumn("id");
            ImGui::TableSetupColumn("param");
            ImGui::TableSetupColumn("flags");
            ImGui::TableSetupColumn("eventno");
            ImGui::TableSetupColumn("fid");
            ImGui::TableSetupColumn("gid");
            ImGui::TableSetupColumn("bpc");
            ImGui::TableSetupColumn("sid");
            ImGui::TableSetupColumn("bpcid");
            ImGui::TableSetupColumn("res");
            ImGui::TableSetupColumn("flgs");

            ImGui::TableHeadersRow();
            auto data = event_reader.get();

            for (auto &evt : data) {
                ImGui::TableNextRow();
                auto fid   = ((evt.id >> 60) & 0xf);   // 1
                if (ImGui::TableSetColumnIndex(0)) { // time
                    ImGui::Text("%s", fmt::format("{:%F-%T}", std::chrono::system_clock::time_point{} + std::chrono::nanoseconds(evt.time)).c_str());
                }
                if (ImGui::TableSetColumnIndex(1)) { // executed time
                    ImGui::Text("%s", fmt::format("{:%F-%T}", std::chrono::system_clock::time_point{} + std::chrono::nanoseconds(evt.executed)).c_str());
                }
                if (ImGui::TableSetColumnIndex(2)) { // id
                    ImGui::Text("%s", fmt::format("{:#x}", evt.id).c_str());
                }
                if (ImGui::TableSetColumnIndex(3)) { // param
                    ImGui::Text("%s", fmt::format("{:#x}", evt.param).c_str());
                }
                if (ImGui::TableSetColumnIndex(4)) { // flags
                    // print flags
                    auto delay = evt.executed - evt.time;
                    if (flags & 1) {
                        ImGui::Text("%s", fmt::format(" !late (by {} ns)", delay).c_str());
                    } else if (flags & 2) {
                        ImGui::Text("%s", fmt::format(" !early (by {} ns)", delay).c_str());
                    } else if (flags & 4) {
                        ImGui::Text("%s", fmt::format(" !conflict (delayed by {} ns)", delay).c_str());
                    } else if (flags & 8) {
                        ImGui::Text("%s", fmt::format(" !delayed (by {} ns)", delay).c_str());
                    }
                }
                if (ImGui::TableSetColumnIndex(5)) { // eventno
                    auto evtno = ((evt.id >> 36) & 0xfff); // 4
                    ImGui::Text("%s", fmt::format("{}", evtno).c_str());
                }
                if (ImGui::TableSetColumnIndex(6)) { // fid
                    ImGui::Text("%s", fmt::format("{}", fid).c_str());
                }
                if (ImGui::TableSetColumnIndex(7)) { // gid
                    auto gid   = ((evt.id >> 48) & 0xfff); // 4
                    ImGui::Text("%s", fmt::format("{}", gid).c_str());
                }
                if (fid == 0) { //full << " FLAGS: N/A";
                    if (ImGui::TableSetColumnIndex(9)) { // sid
                        auto sid   = ((evt.id >> 24) & 0xfff);  // 4
                        ImGui::Text("%s", fmt::format("{}", sid).c_str());
                    }
                    if (ImGui::TableSetColumnIndex(10)) { // pbcid
                        auto bpcid  = ((evt.id >> 10) & 0x3fff); // 5
                        ImGui::Text("%s", fmt::format("{}", bpcid).c_str());
                    }
                    if (ImGui::TableSetColumnIndex(11)) { // res
                        auto res   = (evt.id & 0x3ff);          // 4
                        ImGui::Text("%s", fmt::format("{}", res).c_str());
                    }
                    if (ImGui::TableSetColumnIndex(12)) { // flgs
                        ImGui::Text("%s", fmt::format("{}", 0).c_str());
                    }
                } else if (fid == 1) {
                    if (ImGui::TableSetColumnIndex(8)) { //bpc
                        auto bpc   = ((evt.id >> 34) & 0x1);   // 1
                        ImGui::Text("%s", fmt::format("{}", bpc).c_str());
                    }
                    if (ImGui::TableSetColumnIndex(9)) { // sid
                        auto sid   = ((evt.id >> 20) & 0xfff); // 4
                        ImGui::Text("%s", fmt::format("{}", sid).c_str());
                    }
                    if (ImGui::TableSetColumnIndex(10)) { // pbcid
                        auto bpcid  = ((evt.id >> 6) & 0x3fff); // 5
                        ImGui::Text("%s", fmt::format("{}", bpcid).c_str());
                    }
                    if (ImGui::TableSetColumnIndex(11)) { // res
                        auto res   = (evt.id & 0x3f);          // 4
                        ImGui::Text("%s", fmt::format("{}", res).c_str());
                    }
                    if (ImGui::TableSetColumnIndex(12)) { // flgs
                        auto flgs = ((evt.id >> 32) & 0xf);   // 1
                        ImGui::Text("%s", fmt::format("{}", flgs).c_str());
                    }
                } else {
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
    if (ImGui::CollapsingHeader("Schedule to inject", ImGuiTreeNodeFlags_DefaultOpen)) {
        static enum class InjectState { STOPPED, RUNNING, ONCE, SINGLE } injectState = InjectState::STOPPED;
        static int current = 0;
        static std::vector<Timing::event> events{};
        if (ImGui::Button("+")) {
            events.emplace_back();
        }
        ImGui::SameLine();
        if (ImGui::Button("start")) {
            current = 0;
            injectState = InjectState::RUNNING;
        }
        ImGui::SameLine();
        if (ImGui::Button("once")) {
            current = 0;
            injectState = InjectState::ONCE;
        }
        ImGui::SameLine();
        if (ImGui::Button("stop")) {
            injectState = InjectState::STOPPED;
        }
        ImGui::SameLine();
        static bool showDetails;
        ImGui::Checkbox("show details", &showDetails);
        ImGui::SameLine();
        static std::array<char*, 3> format{"raw", "fid = 1", "fid = 2"};
        static int current_format = 1;
        ImGui::Combo("timing format", &current_format, format.data(), format.size(), format.size());
        // schedule table
        static int freeze_cols = 1;
        static int freeze_rows = 1;
        const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
        ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 24);
        static ImGuiTableFlags flags =
                ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable |
                ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
        int nColumns = current_format == 1 ? 7 : (current_format == 2 ? 8 : 3);
        if (ImGui::BeginTable("schedule", nColumns, flags, outer_size)) {
            ImGui::TableSetupScrollFreeze(freeze_cols, freeze_rows);
            ImGui::TableSetupColumn("time offset", ImGuiTableColumnFlags_NoHide); // Make the first column not hideable to match our use of TableSetupScrollFreeze()
            switch (current_format) {
                case 1:
                    ImGui::TableSetupColumn("eventno");
                    ImGui::TableSetupColumn("gid");
                    ImGui::TableSetupColumn("sid");
                    ImGui::TableSetupColumn("bpcid");
                    ImGui::TableSetupColumn("res");
                    ImGui::TableSetupColumn("flgs");
                    break;
                case 2:
                    ImGui::TableSetupColumn("eventno");
                    ImGui::TableSetupColumn("gid");
                    ImGui::TableSetupColumn("bpc");
                    ImGui::TableSetupColumn("sid");
                    ImGui::TableSetupColumn("bpcid");
                    ImGui::TableSetupColumn("res");
                    ImGui::TableSetupColumn("flgs");
                    break;
                default:
                case 0:
                    ImGui::TableSetupColumn("id");
                    ImGui::TableSetupColumn("param");
                    break;
            }
            ImGui::TableHeadersRow();
            static constexpr uint64_t min_uint64 = 0, max_uint64 = std::numeric_limits<uint64_t>::max();
            static constexpr uint64_t max_uint40 = (1ul << 40) - 1; // 5
            static constexpr uint64_t max_uint32 = (1ul << 32) - 1; // 4
            static constexpr uint64_t max_uint24 = (1ul << 24) - 1; // 3
            static constexpr uint64_t max_uint16 = (1ul << 16) - 1; // 2
            static constexpr uint64_t max_uint8 = (1ul << 8) - 1; // 1
            for (auto &ev : events) {
                ImGui::PushID(&ev);
                if (ImGui::TableNextColumn()) {
                    ImGui::DragScalar("##time", ImGuiDataType_U64, &ev.time, 1.0f, &min_uint64, &max_uint64, "%d", ImGuiSliderFlags_None);
                }
                switch (current_format) {
                    case 1:
                        if (ImGui::TableNextColumn()) {
                            uint64_t eventno = ((ev.id >> 36) & 0xfff);
                            ImGui::DragScalar("##eventno", ImGuiDataType_U64, &eventno, 1.0f, &min_uint64, &max_uint32, "%d", ImGuiSliderFlags_None);
                            ev.id = (eventno << 36) | (ev.id & ~(max_uint32 << 36));
                        } // ("eventno");
                        if (ImGui::TableNextColumn()) {
                            uint64_t gid = ((ev.id >> 48) & 0xfff);
                            ImGui::DragScalar("##gid", ImGuiDataType_U64, &gid, 1.0f, &min_uint64, &max_uint32, "%d", ImGuiSliderFlags_None);
                            ev.id = (gid << 48) | (ev.id & ~(max_uint32 << 48));
                        } // ("gid");
                        if (ImGui::TableNextColumn()) {
                            auto sid   = ((ev.id >> 20) & 0xfff); // 4
                            ImGui::DragScalar("##sid", ImGuiDataType_U64, &sid, 1.0f, &min_uint64, &max_uint32, "%d", ImGuiSliderFlags_None);
                            ev.id = (sid << 20) | (ev.id & ~(max_uint32 << 20));
                        } // ("sid");
                        if (ImGui::TableNextColumn()) {
                            auto bpcid  = ((ev.id >> 6) & 0x3fff); // 5
                            ImGui::DragScalar("##bpcid", ImGuiDataType_U64, &bpcid, 1.0f, &min_uint64, &max_uint40, "%d", ImGuiSliderFlags_None);
                            ev.id = (bpcid << 6) | (ev.id & ~(max_uint40 << 6));
                        } // ("bpcid");
                        if (ImGui::TableNextColumn()) {
                            auto res   = (ev.id & 0x3f);          // 4
                            ImGui::DragScalar("##res", ImGuiDataType_U64, &res, 1.0f, &min_uint64, &max_uint32, "%d", ImGuiSliderFlags_None);
                            ev.id = (res << 36) | (ev.id & ~(max_uint32 << 36));
                        } // ("res");
                        if (ImGui::TableNextColumn()) {
                            auto flgs = ((ev.id >> 32) & 0xf);   // 1
                            ImGui::DragScalar("##flgs", ImGuiDataType_U64, &flgs, 1.0f, &min_uint64, &max_uint8, "%d", ImGuiSliderFlags_None);
                            ev.id = (flgs << 32) | (ev.id & ~(max_uint8 << 32));
                        } // ("flgs");
                        break;
                    case 2:
                        if (ImGui::TableNextColumn()) { } // ("eventno"); //   auto bpc   = ((evt.id >> 34) & 0x1);   // 1
                        if (ImGui::TableNextColumn()) { } // ("gid");
                        if (ImGui::TableNextColumn()) { } // ("bpc");
                        if (ImGui::TableNextColumn()) { } // ("sid");
                        if (ImGui::TableNextColumn()) { } // ("bpcid");
                        if (ImGui::TableNextColumn()) { } // ("res");
                        if (ImGui::TableNextColumn()) { } // ("flgs");
                        break;
                    default:
                    case 0:
                        if (ImGui::TableNextColumn()) {  // ("id");
                            std::string id = fmt::format("{}", ev.id);
                            id.reserve(32);
                            ImGui::InputText("##id", id.data(), 32);

                            // ImGui::DragScalar("##id", ImGuiDataType_U64, &ev.id, 1.0f, &min_uint64, &max_uint64, "%d", ImGuiSliderFlags_None);
                            // ev.id =
                        }
                        if (ImGui::TableNextColumn()) {
                            ImGui::DragScalar("##param", ImGuiDataType_U64, &ev.param, 1.0f, &min_uint64, &max_uint64, "%d", ImGuiSliderFlags_None);
                        }
                        break;
                }
                ImGui::PopID();
            }
        }
        ImGui::EndTable();
        // if running, schedule events up to 100ms ahead
    }
}

void showTRConfig(Timing &timing) {
    if (ImGui::CollapsingHeader("Timing Receiver IO configuration", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (!timing.initialized) {
            ImGui::TextUnformatted("No timing receiver found");
            return;
        }
        auto trTime = timing.receiver->CurrentTime().getTAI();
        ImGui::TextUnformatted(fmt::format("{}\n {}\n {}°C,\nGateware: {},\n(\"version\", \"{}\")",
                                           trTime, tai_ns_to_utc(trTime),
                                           timing.receiver->CurrentTemperature(),
                                           fmt::join(timing.receiver->getGatewareInfo(), ",\n"),
                                           timing.receiver->getGatewareVersion()).c_str());
        // print tr info
        // table of ios
        //saft-io-ctl pexaria -i
        //Name           Direction  OutputEnable  InputTermination  SpecialOut  SpecialIn  Resolution  Logic Level
        //--------------------------------------------------------------------------------------------------------
        //        IO1            Out        Yes           No                No          No         1ns (LVDS)  LVTTL
        //IO2            Out        Yes           No                No          No         1ns (LVDS)  LVTTL
        //IO3            Out        Yes           No                No          No         1ns (LVDS)  LVTTL
        //LED1_ADD_R     Out        No            No                No          No         8ns (GPIO)  TTL
        // conditions (dropdown to select IO}
        // Table: EVENT | MASK | OFFSET | FLAGS (late1/early2/conflict4/delayed8) | on/off
    }
}

void showEBConsole(WBConsole &console) {
    if (ImGui::CollapsingHeader("EtherBone Console")) {
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
