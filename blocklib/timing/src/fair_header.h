#ifndef IMPLOT_VISUALIZATION_FAIR_HEADER_H
#define IMPLOT_VISUALIZATION_FAIR_HEADER_H
#include <imgui.h>
#include <chrono>
#include <cmrc/cmrc.hpp>
#include <fmt/chrono.h>
#include <string_view>

CMRC_DECLARE(ui_assets);

namespace app_header {
namespace detail {
    void TextCentered(const std::string_view text) {
        auto windowWidth = ImGui::GetWindowSize().x;
        auto textWidth = ImGui::CalcTextSize(text.data(), text.data() + text.size()).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::Text("%s", text.data());
    }

    void TextRight(const std::string_view text) {
        auto windowWidth = ImGui::GetWindowSize().x;
        auto textWidth = ImGui::CalcTextSize(text.data(), text.data() + text.size()).x;
        ImGui::SetCursorPosX(windowWidth - textWidth - ImGui::GetStyle().ItemSpacing.x);
        ImGui::Text("%s", text.data());
    }
}
ImVec2 logoSize{ 0.f, 0.f };

ImFont* loadHeaderFont(float size) {
    ImFontConfig config;
    config.OversampleH          = 1;
    config.OversampleV          = 1;
    config.PixelSnapH           = true;
    config.FontDataOwnedByAtlas = false;
    ImGuiIO   &io       = ImGui::GetIO();
    auto primaryFont = cmrc::ui_assets::get_filesystem().open("Roboto-Medium.ttf");
    return io.Fonts->AddFontFromMemoryTTF(const_cast<char *>(primaryFont.begin()), primaryFont.size(), size, &config);
}

void draw_header_bar(std::string_view title, ImFont *headerFont) {
    using namespace detail;
    // localtime
    const auto clock         = std::chrono::system_clock::now();
    const auto utcClock      = fmt::format("{:%Y-%m-%d %H:%M:%S (LOC)}", std::chrono::round<std::chrono::seconds>(clock));
    const auto utcStringSize = ImGui::CalcTextSize(utcClock.c_str());

    const auto topLeft       = ImGui::GetCursorPos();
    // draw title
    ImGui::PushFont(headerFont);
    const auto titleSize     = ImGui::CalcTextSize(title.data());
    // suppress title if it doesn't fit or is likely to overlap
    if (0.5f * ImGui::GetIO().DisplaySize.x > (0.5f * titleSize.x + utcStringSize.x)) {
        TextCentered(title);
    }
    ImGui::PopFont();

    ImGui::SameLine();
    auto pos = ImGui::GetCursorPos();
    TextRight(utcClock); // %Z should print abbreviated timezone but doesnt
    // utc (using c-style timedate functions because of missing stdlib support)
    // const auto utc_clock = std::chrono::utc_clock::now(); // c++20 timezone is not implemented in gcc or clang yet
    // date + tz library unfortunately doesn't play too well with emscripten/fetchcontent
    // const auto localtime = fmt::format("{:%H:%M:%S (%Z)}", date::make_zoned("utc", clock).get_sys_time());
    std::string utctime; // assume maximum of 32 characters for datetime length
    utctime.resize(32);
    pos.y += ImGui::GetTextLineHeightWithSpacing();
    ImGui::SetCursorPos(pos);
    const auto utc = std::chrono::system_clock::to_time_t(clock);
    const auto len = strftime(utctime.data(), utctime.size(), "%H:%M:%S (UTC)", gmtime(&utc));
    TextRight(std::string_view(utctime.data(), len));
    auto posBeneathClock = ImGui::GetCursorPos();

    // draw fair logo -> do not draw logo for simple debug app to not require external image loading dependencies and assets
    ImGui::SetCursorPos(topLeft);
    ImGui::PushFont(headerFont);
    ImGui::TextUnformatted("FAIR");
    ImGui::PopFont();

    posBeneathClock.x = 0.f;
    ImGui::SetCursorPos(posBeneathClock); // set to end of image
}

} // namespace app_header
#endif // IMPLOT_VISUALIZATION_FAIR_HEADER_H
