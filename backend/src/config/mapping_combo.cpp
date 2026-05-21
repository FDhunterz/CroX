#include "inputflow/config/mapping_combo.hpp"
#include <algorithm>
#include <unordered_set>

namespace inputflow {

void normalizeMappingButtons(JoystickMapping& m) {
    if (m.buttons.empty()) {
        m.buttons = {m.button};
    } else {
        m.button = m.buttons.front();
    }
}

std::vector<JoystickButton> mappingButtons(const JoystickMapping& m) {
    if (!m.buttons.empty()) return m.buttons;
    return {m.button};
}

std::string comboSignature(const std::vector<JoystickButton>& buttons) {
    std::vector<int> ids;
    ids.reserve(buttons.size());
    for (auto b : buttons) ids.push_back(static_cast<int>(b));
    std::sort(ids.begin(), ids.end());
    std::string sig;
    for (size_t i = 0; i < ids.size(); ++i) {
        if (i) sig += '+';
        sig += std::to_string(ids[i]);
    }
    return sig;
}

bool allComboPressed(const JoystickMapping& m, const JoystickState& state) {
    for (auto b : mappingButtons(m)) {
        if (!state.pressed.test(static_cast<size_t>(b))) return false;
    }
    return true;
}

void dedupeProfileMappings(std::vector<JoystickMapping>& mappings) {
    std::unordered_set<std::string> seen;
    std::vector<JoystickMapping> out;
    out.reserve(mappings.size());
    for (auto& m : mappings) {
        normalizeMappingButtons(m);
        if (m.buttons.empty()) {
            out.push_back(std::move(m));
            continue;
        }
        const auto sig = comboSignature(m.buttons);
        if (seen.count(sig)) continue;
        seen.insert(sig);
        out.push_back(std::move(m));
    }
    mappings = std::move(out);
}

}  // namespace inputflow
