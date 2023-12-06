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
template <auto Begin, auto End, bool UnconditionalEnd = false>
class Widget {
    bool shown = false;
public:
    template <typename... A>
    explicit Widget (A&&... a)
    requires (!std::same_as<Widget, std::remove_cvref_t<A>> && ...)
        : shown{
            []<typename... AA> (AA&&... aa) {
                return Begin(std::forward<AA>(aa)...);
            } (std::forward<A>(a)...)
        } {}
    ~Widget () { if (UnconditionalEnd || shown) End(); }
    Widget(Widget&) = delete;
    Widget(Widget&&) = delete;
    Widget& operator=(const Widget&) = delete;
    Widget& operator=(Widget&&) = delete;
    explicit operator bool () const& { return shown; }
    explicit operator bool () && = delete; // force to store a reference to the widget in the if condition to extend the lifetime
};
template <auto Begin, auto End> class WidgetVoid {
public:
    template <typename... A>
    requires (!std::same_as<WidgetVoid, std::remove_cvref_t<A>> && ...)
    [[nodiscard]] explicit WidgetVoid (A&&... a) {
        Begin(std::forward<A>(a)...);
    }
    ~WidgetVoid () { End(); }
    WidgetVoid(WidgetVoid&) = delete;
    WidgetVoid(WidgetVoid&&) = delete;
    WidgetVoid& operator=(const WidgetVoid&) = delete;
    WidgetVoid& operator=(WidgetVoid&&) = delete;
};
} // namespace detail

using Window = detail::Widget<&ImGui::Begin, &ImGui::End, true>;
using TabBar = detail::Widget<&ImGui::BeginTabBar, &ImGui::EndTabBar>;
using TabItem = detail::Widget<&ImGui::BeginTabItem, &ImGui::EndTabItem>;
using Table = detail::Widget<&ImGui::BeginTable, &ImGui::EndTable>;
using Menu = detail::Widget<&ImGui::BeginMenu, &ImGui::EndMenu>;
using ListBox= detail::Widget<&ImGui::BeginListBox, &ImGui::EndListBox>;
using Disabled = detail::WidgetVoid<&ImGui::BeginDisabled, &ImGui::EndDisabled>;
using Tooltip = detail::WidgetVoid<&ImGui::BeginTooltip, &ImGui::EndTooltip>;
} // namespace ImScoped
