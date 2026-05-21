#include "inputflow/config/mapping_validator.hpp"
#include "inputflow/config/json_mapping_parser.hpp"
#include "inputflow/config/mapping_combo.hpp"
#include "inputflow/util/key_code_table.hpp"
#include <unordered_set>
#include <sstream>

namespace inputflow {

std::string MappingValidator::outputSignature(bool hasKey, KeyCode key, Modifier mods) {
    std::ostringstream oss;
    oss << (hasKey ? 1 : 0) << ":" << key << ":" << static_cast<unsigned>(mods);
    return oss.str();
}

std::vector<ValidationError> MappingValidator::validate(
    const std::vector<JoystickMapping>& mappings) {
    std::vector<ValidationError> errors;
    std::unordered_set<int> seenButtons;
    std::unordered_set<std::string> seenCombos;
    std::unordered_set<std::string> seenOutputs;

    for (auto m : mappings) {
        if (!m.enabled) continue;
        normalizeMappingButtons(m);

        if (m.buttons.empty()) {
            errors.push_back({
                "buttons",
                "Pemetaan \"" + m.id + "\" harus punya minimal satu tombol joystick"
            });
            continue;
        }

        const auto comboSig = comboSignature(m.buttons);
        if (seenCombos.count(comboSig)) {
            errors.push_back({
                "buttons",
                "Kombinasi tombol sudah dipetakan (tidak boleh duplikat)"
            });
        }
        seenCombos.insert(comboSig);

        for (auto b : m.buttons) {
            const int btn = static_cast<int>(b);
            if (seenButtons.count(btn)) {
                errors.push_back({
                    "buttons",
                    "Tombol \"" + JsonMappingParser::buttonToString(b) +
                        "\" sudah dipakai di pemetaan lain"
                });
            }
            seenButtons.insert(btn);
        }

        const auto sig = outputSignature(m.hasKey, m.key, m.modifiers);
        if (!m.hasKey && m.modifiers == Modifier::None) {
            errors.push_back({
                "output",
                "Pemetaan \"" + m.id + "\" harus punya minimal satu modifier atau tombol keyboard"
            });
            continue;
        }
        if (seenOutputs.count(sig)) {
            std::string out = m.hasKey ? keyCodeToName(m.key) : "(modifier)";
            errors.push_back({
                "output",
                "Output keyboard \"" + out + "\" sudah digunakan (tidak boleh sama)"
            });
        }
        seenOutputs.insert(sig);
    }
    return errors;
}

}  // namespace inputflow
