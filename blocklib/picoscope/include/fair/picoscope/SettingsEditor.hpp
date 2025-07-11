#ifndef SETTINGSEDITOR_HPP
#define SETTINGSEDITOR_HPP

#include <chrono>
#include <cstdio>
#include <fcntl.h>
#include <print>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <utility>

namespace fair::ui_helpers {

/**
 * Small helper class to allow editing settings interactively without heavy dependencies.
 * Intended to be run as part of a (terminal) UI loop in an ImGui like fashion.
 * Settings are laid out in a fixed-width grid and can be navigated with vim-style movement keys
 */
class SettingsEditor {
    bool                editing = false;
    std::pair<int, int> position{0, 0};
    std::pair<int, int> movement{0, 0};    // direction of requested movement
    int                 columnWidth  = 10; // size of the columns for editing parameters
    char                currentInput = 0;  // the currently pressed input
    std::string         editor;
    std::string         currentDocstring;
    // store old terminal settings to restore on quit
    termios oldt{};

public:
    bool quit = false;

    SettingsEditor() {
        std::puts("\x1b[?1049h"); // xterm alternate buffer (allows keeping regular scrollback when exiting fullscreen tui)
        termios newt{};
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
        newt.c_cc[VERASE] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    }

    ~SettingsEditor() {
        std::puts("\x1b[?1049l"); // xterm exit alternate buffer mode
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }

    static char getNonBlockingInput() {
        int oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
        const auto ch = static_cast<char>(getchar());
        fcntl(STDIN_FILENO, F_SETFL, oldf);
        return ch;
    }

    void processInput(const char input = getNonBlockingInput()) {
        currentInput = 0;
        if (input == 033) {                     // control characters
            if (getNonBlockingInput() == '[') { // arrow keys
                switch (getNonBlockingInput()) {
                case 'A': movement = {0, -1}; break;
                case 'B': movement = {0, 1}; break;
                case 'C': movement = {1, 0}; break;
                case 'D': movement = {-1, 0}; break;
                case 'H': position = {0, 0}; break;
                }
            }
            return;
        }
        if (editing) { // in editing mode all inputs are processed by the value editor
            currentInput = input;
            return;
        }
        switch (input) {
        case 'h': movement = {-1, 0}; break;
        case 'j': movement = {0, 1}; break;
        case 'k': movement = {0, -1}; break;
        case 'l': movement = {1, 0}; break;
        case '#': position = {0, 0}; break;
        case 'q': quit = true; break;
        default: currentInput = input;
        }
    }

    template<typename T>
    bool renderSetting(std::pair<int, int> pos, int span, T& value, std::string_view docstring = "") {
        if (pos.first <= position.first && pos.first + span > position.first && pos.second == position.second) {
            if (movement != std::pair{0, 0}) {
                if (movement.first == 1) {
                    position.first = pos.first + span;
                    movement       = std::pair{0, 0};
                } else if (movement.first == -1) {
                    position.first = std::max(position.first - 1, 0);
                    movement       = std::pair{0, 0};
                } else if (movement.second == 1) {
                    position.second += 1;
                    movement = std::pair{0, 0};
                } else if (movement.second == -1) {
                    position.second = std::max(position.second - 1, 0);
                    movement        = std::pair{0, 0};
                } else {
                    std::println("illegal movement");
                    std::exit(0);
                }
            } else {
                currentDocstring = docstring;
                std::print("*{:^{}}*", value, columnWidth - 2);
                if constexpr (magic_enum::is_scoped_enum_v<T>) {
                    currentDocstring.append(" Select option: ");
                    bool changed = false;
                    magic_enum::enum_for_each<T>([this, &value, &changed](const auto& enumElement) {
                        currentDocstring.append(std::format("{}: {}, ", magic_enum::enum_index(enumElement()).value_or(0), magic_enum::enum_name(enumElement())));
                        if (currentInput >= '0' && currentInput <= '9' && magic_enum::enum_index(enumElement()) == currentInput - '0') {
                            value   = enumElement;
                            changed = true;
                        }
                    });
                    return changed;
                } else {
                    if (currentInput == '\n') {
                        if constexpr (std::same_as<T, bool>) { // bool just gets toggled
                            value = !value;
                        } else {            // all other values get put into the editor field
                            if (!editing) { // start editing
                                editing = true;
                                editor  = std::format("{}", value);
                            } else { // stop editing
                                editing  = false;
                                T oldval = value;
                                if constexpr (std::same_as<T, unsigned int>) {
                                    value = static_cast<unsigned int>(std::stoul(editor));
                                } else if constexpr (std::same_as<T, int>) {
                                    value = std::stoi(editor);
                                } else if constexpr (std::same_as<T, short int>) {
                                    value = static_cast<short int>(std::stoi(editor));
                                } else if constexpr (std::same_as<T, unsigned short int>) {
                                    value = static_cast<unsigned short int>(std::stoul(editor));
                                } else if constexpr (std::same_as<T, long>) {
                                    value = std::stol(editor);
                                } else if constexpr (std::same_as<T, long long>) {
                                    value = std::stoll(editor);
                                } else if constexpr (std::same_as<T, unsigned long>) {
                                    value = std::stoul(editor);
                                } else if constexpr (std::same_as<T, unsigned long long>) {
                                    value = std::stoull(editor);
                                } else if constexpr (std::same_as<T, float>) {
                                    value = std::stof(editor);
                                } else if constexpr (std::same_as<T, double>) {
                                    value = std::stod(editor);
                                } else if constexpr (std::same_as<T, std::string>) {
                                    value = editor;
                                } else {
                                    static_assert(false && "Editing this value is not implemented" && std::is_trivially_copyable_v<T>);
                                }
                                editor = "";
                                return oldval != value;
                            }
                        }
                    }
                }
            }
            return false;
        }
        std::print("{:^{}}", value, columnWidth);
        return false;
    }

    void renderEditor() {
        if (editing) {
            if ((currentInput == '\b' || currentInput == '+') && !editor.empty()) {
                editor.resize(editor.size() - 1);
            } else if (currentInput >= 32) { // only allow non-control-code characters
                editor.push_back(currentInput);
            }
            std::print("{} <enter> to commit): {}", currentDocstring, editor);
        } else { // no value selected for editing - print docstring and navigation help
            std::print("{} - use hjkl/arrows to move selection, <enter> to edit/toggle, # to return to first item, q to quit", currentDocstring);
        }
    }

    static std::pair<std::size_t, std::size_t> getTerminalSize() {
        std::pair size{150UZ, 40UZ};
#ifdef TIOCGSIZE
        struct ttysize ts {};
        ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
        size.first  = ts.ts_cols;
        size.second = ts.ts_lines;
#elif defined(TIOCGWINSZ)
        struct winsize ts {};
        ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
        size.first  = ts.ws_col;
        size.second = ts.ws_row;
#endif /* TIOCGSIZE */
        return size;
    }
};
} // namespace fair::ui_helpers

#endif // SETTINGSEDITOR_HPP
