#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <implot.h>
/**
 * A small wrapper to automatically wrap ImGui Begin/End pairs to prevent mismatched calls.
 * Taken from: https://github.com/ocornut/imgui/issues/2096#issuecomment-1463837461
 * TODO:
 * - implot support
 * - BeginChild needs a more manual wrapper because of varargs
 * - small issue: this wrapper breaks default arguments, they have to be always specified
 */
namespace ImScoped {
namespace detail {
template <auto Begin, auto End, bool UnconditionalEnd = false> class Widget {
    bool shown;
public:
    explicit Widget (auto&&... a)
        : shown{
            [] (auto&&... aa) {
                return Begin(std::forward<decltype(aa)>(aa)...);
            } (std::forward<decltype(a)>(a)...)
        } {}
    ~Widget () { if (UnconditionalEnd || shown) End(); }
    explicit operator bool () const& { return shown; }
    explicit operator bool () && = delete; // force to store a reference to the widget in the if condition to extend the lifetime
};
template <auto Begin, auto End> class WidgetVoid {
public:
    [[nodiscard]] explicit WidgetVoid (auto&&... a) {
        Begin(std::forward<decltype(a)>(a)...);
    }
    ~WidgetVoid () { End(); }
};
} // namespace detail

using Window = detail::Widget<ImGui::Begin, ImGui::End, true>;
using TabBar = detail::Widget<ImGui::BeginTabBar, ImGui::EndTabBar>;
using TabItem = detail::Widget<ImGui::BeginTabItem, ImGui::EndTabItem>;
using Table = detail::Widget<ImGui::BeginTable, ImGui::EndTable>;
using Menu = detail::Widget<ImGui::BeginMenu, ImGui::EndMenu>;
using ListBox= detail::Widget<ImGui::BeginListBox, ImGui::EndListBox>;
using Disabled = detail::WidgetVoid<ImGui::BeginDisabled, ImGui::EndDisabled>;
using Tooltip = detail::WidgetVoid<ImGui::BeginTooltip, ImGui::EndTooltip>;
} // namespace ImScoped