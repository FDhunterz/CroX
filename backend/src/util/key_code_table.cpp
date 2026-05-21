#include "inputflow/util/key_code_table.hpp"
#include "inputflow/util/special_keys.hpp"
#include <unordered_map>
#include <algorithm>
#include <cctype>

#if defined(INPUTFLOW_PLATFORM_MACOS)
#include <Carbon/Carbon.h>
#endif

namespace inputflow {
namespace {

std::string upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}

#if defined(INPUTFLOW_PLATFORM_MACOS)

const std::unordered_map<std::string, KeyCode>& nameTable() {
    static const std::unordered_map<std::string, KeyCode> table = {
        {"A", kVK_ANSI_A}, {"B", kVK_ANSI_B}, {"C", kVK_ANSI_C},
        {"D", kVK_ANSI_D}, {"E", kVK_ANSI_E}, {"F", kVK_ANSI_F},
        {"G", kVK_ANSI_G}, {"H", kVK_ANSI_H}, {"I", kVK_ANSI_I},
        {"J", kVK_ANSI_J}, {"K", kVK_ANSI_K}, {"L", kVK_ANSI_L},
        {"M", kVK_ANSI_M}, {"N", kVK_ANSI_N}, {"O", kVK_ANSI_O},
        {"P", kVK_ANSI_P}, {"Q", kVK_ANSI_Q}, {"R", kVK_ANSI_R},
        {"S", kVK_ANSI_S}, {"T", kVK_ANSI_T}, {"U", kVK_ANSI_U},
        {"V", kVK_ANSI_V}, {"W", kVK_ANSI_W}, {"X", kVK_ANSI_X},
        {"Y", kVK_ANSI_Y}, {"Z", kVK_ANSI_Z},
        {"0", kVK_ANSI_0}, {"1", kVK_ANSI_1}, {"2", kVK_ANSI_2},
        {"3", kVK_ANSI_3}, {"4", kVK_ANSI_4}, {"5", kVK_ANSI_5},
        {"6", kVK_ANSI_6}, {"7", kVK_ANSI_7}, {"8", kVK_ANSI_8},
        {"9", kVK_ANSI_9},
        {"SPACE", kVK_Space}, {"ENTER", kVK_Return}, {"RETURN", kVK_Return},
        {"TAB", kVK_Tab}, {"ESC", kVK_Escape}, {"ESCAPE", kVK_Escape},
        {"F1", kVK_F1}, {"F2", kVK_F2}, {"F3", kVK_F3}, {"F4", kVK_F4},
        {"F5", kVK_F5}, {"F6", kVK_F6}, {"F7", kVK_F7}, {"F8", kVK_F8},
        {"F9", kVK_F9}, {"F10", kVK_F10}, {"F11", kVK_F11}, {"F12", kVK_F12},
        {"MOUSE_LEFT", kKeyMouseLeft}, {"MOUSE_RIGHT", kKeyMouseRight},
    };
    return table;
}

KeyCode letterFallback(char c) {
    switch (c) {
        case 'A': return kVK_ANSI_A;
        case 'B': return kVK_ANSI_B;
        case 'C': return kVK_ANSI_C;
        case 'D': return kVK_ANSI_D;
        case 'E': return kVK_ANSI_E;
        case 'F': return kVK_ANSI_F;
        case 'G': return kVK_ANSI_G;
        case 'H': return kVK_ANSI_H;
        case 'I': return kVK_ANSI_I;
        case 'J': return kVK_ANSI_J;
        case 'K': return kVK_ANSI_K;
        case 'L': return kVK_ANSI_L;
        case 'M': return kVK_ANSI_M;
        case 'N': return kVK_ANSI_N;
        case 'O': return kVK_ANSI_O;
        case 'P': return kVK_ANSI_P;
        case 'Q': return kVK_ANSI_Q;
        case 'R': return kVK_ANSI_R;
        case 'S': return kVK_ANSI_S;
        case 'T': return kVK_ANSI_T;
        case 'U': return kVK_ANSI_U;
        case 'V': return kVK_ANSI_V;
        case 'W': return kVK_ANSI_W;
        case 'X': return kVK_ANSI_X;
        case 'Y': return kVK_ANSI_Y;
        case 'Z': return kVK_ANSI_Z;
        default: return 0;
    }
}

std::string codeToLetter(KeyCode code) {
    struct Pair { KeyCode code; char letter; };
    static const Pair pairs[] = {
        {kVK_ANSI_A,'A'},{kVK_ANSI_B,'B'},{kVK_ANSI_C,'C'},{kVK_ANSI_D,'D'},
        {kVK_ANSI_E,'E'},{kVK_ANSI_F,'F'},{kVK_ANSI_G,'G'},{kVK_ANSI_H,'H'},
        {kVK_ANSI_I,'I'},{kVK_ANSI_J,'J'},{kVK_ANSI_K,'K'},{kVK_ANSI_L,'L'},
        {kVK_ANSI_M,'M'},{kVK_ANSI_N,'N'},{kVK_ANSI_O,'O'},{kVK_ANSI_P,'P'},
        {kVK_ANSI_Q,'Q'},{kVK_ANSI_R,'R'},{kVK_ANSI_S,'S'},{kVK_ANSI_T,'T'},
        {kVK_ANSI_U,'U'},{kVK_ANSI_V,'V'},{kVK_ANSI_W,'W'},{kVK_ANSI_X,'X'},
        {kVK_ANSI_Y,'Y'},{kVK_ANSI_Z,'Z'},
    };
    for (const auto& p : pairs) {
        if (p.code == code) return std::string(1, p.letter);
    }
    return "";
}

#else

const std::unordered_map<std::string, KeyCode>& nameTable() {
    static const std::unordered_map<std::string, KeyCode> table = {
        {"A", 0x41}, {"B", 0x42}, {"C", 0x43}, {"D", 0x44}, {"E", 0x45},
        {"F", 0x46}, {"G", 0x47}, {"H", 0x48}, {"I", 0x49}, {"J", 0x4A},
        {"K", 0x4B}, {"L", 0x4C}, {"M", 0x4D}, {"N", 0x4E}, {"O", 0x4F},
        {"P", 0x50}, {"Q", 0x51}, {"R", 0x52}, {"S", 0x53}, {"T", 0x54},
        {"U", 0x55}, {"V", 0x56}, {"W", 0x57}, {"X", 0x58}, {"Y", 0x59},
        {"Z", 0x5A},
        {"0", 0x30}, {"1", 0x31}, {"2", 0x32}, {"3", 0x33}, {"4", 0x34},
        {"5", 0x35}, {"6", 0x36}, {"7", 0x37}, {"8", 0x38}, {"9", 0x39},
        {"SPACE", 0x20}, {"ENTER", 0x0D}, {"TAB", 0x09}, {"ESC", 0x1B},
        {"ESCAPE", 0x1B},
        {"F1", 0x70}, {"F2", 0x71}, {"F3", 0x72}, {"F4", 0x73},
        {"F5", 0x74}, {"F6", 0x75}, {"F7", 0x76}, {"F8", 0x77},
        {"F9", 0x78}, {"F10", 0x79}, {"F11", 0x7A}, {"F12", 0x7B},
    };
    return table;
}

KeyCode letterFallback(char c) {
    if (c >= 'A' && c <= 'Z') return static_cast<KeyCode>(c);
    return 0;
}

std::string codeToLetter(KeyCode code) {
    if (code >= 0x41 && code <= 0x5A) return std::string(1, static_cast<char>(code));
    return "";
}

#endif

}  // namespace

KeyCode keyCodeFromName(const std::string& name) {
    const auto key = upper(name);
    auto it = nameTable().find(key);
    if (it != nameTable().end()) return it->second;
    if (key.size() == 1) return letterFallback(key[0]);
    return 0;
}

std::string keyCodeToName(KeyCode code) {
    if (code == kKeyMouseLeft) return "MOUSE_LEFT";
    if (code == kKeyMouseRight) return "MOUSE_RIGHT";
    for (const auto& [name, val] : nameTable()) {
        if (val == code && name.size() <= 2) return name;
    }
    const auto letter = codeToLetter(code);
    if (!letter.empty()) return letter;
    return "UNKNOWN";
}

Modifier modifiersFromNames(const std::vector<std::string>& names) {
    Modifier m = Modifier::None;
    for (auto n : names) {
        n = upper(n);
        if (n == "CTRL" || n == "CONTROL") m = m | Modifier::Ctrl;
        else if (n == "SHIFT") m = m | Modifier::Shift;
        else if (n == "ALT" || n == "OPTION") m = m | Modifier::Alt;
        else if (n == "META" || n == "CMD" || n == "WIN" || n == "SUPER") m = m | Modifier::Meta;
    }
    return m;
}

std::vector<std::string> modifiersToNames(Modifier mods) {
    std::vector<std::string> out;
    if (hasModifier(mods, Modifier::Ctrl)) out.push_back("Ctrl");
    if (hasModifier(mods, Modifier::Shift)) out.push_back("Shift");
    if (hasModifier(mods, Modifier::Alt)) out.push_back("Alt");
    if (hasModifier(mods, Modifier::Meta)) out.push_back("Meta");
    return out;
}

}  // namespace inputflow
