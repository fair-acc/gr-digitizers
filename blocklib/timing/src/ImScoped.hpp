#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <implot.h>

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
    explicit operator bool () && = delete;
};
} // namespace detail

using Window = detail::Widget<ImGui::Begin, ImGui::End, true>;
using TabBar = detail::Widget<ImGui::BeginTabBar, ImGui::EndTabBar>;
using TabItem = detail::Widget<ImGui::BeginTabItem, ImGui::EndTabItem>;
using Table = detail::Widget<ImGui::BeginTable, ImGui::EndTable>;
using Disabled = detail::Widget<ImGui::BeginDisabled, ImGui::EndDisabled>;
// etc.
} // namespace ImScoped