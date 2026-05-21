#include "inputflow/config/json_mapping_parser.hpp"
#include "inputflow/config/mapping_combo.hpp"
#include "inputflow/util/key_code_table.hpp"
#include <fstream>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace inputflow {

JoystickButton JsonMappingParser::buttonFromString(const std::string& s) {
    static const std::unordered_map<std::string, JoystickButton> map = {
        {"A", JoystickButton::A}, {"B", JoystickButton::B},
        {"X", JoystickButton::X}, {"Y", JoystickButton::Y},
        {"LB", JoystickButton::LB}, {"RB", JoystickButton::RB},
        {"LT", JoystickButton::LT}, {"RT", JoystickButton::RT},
        {"DPAD_UP", JoystickButton::DpadUp},
        {"DPAD_DOWN", JoystickButton::DpadDown},
        {"DPAD_LEFT", JoystickButton::DpadLeft},
        {"DPAD_RIGHT", JoystickButton::DpadRight},
        {"START", JoystickButton::Start},
        {"BACK", JoystickButton::Back},
        {"L3", JoystickButton::L3}, {"R3", JoystickButton::R3},
    };
    auto it = map.find(s);
    return it != map.end() ? it->second : JoystickButton::A;
}

std::string JsonMappingParser::buttonToString(JoystickButton b) {
    switch (b) {
        case JoystickButton::A: return "A";
        case JoystickButton::B: return "B";
        case JoystickButton::X: return "X";
        case JoystickButton::Y: return "Y";
        case JoystickButton::LB: return "LB";
        case JoystickButton::RB: return "RB";
        case JoystickButton::LT: return "LT";
        case JoystickButton::RT: return "RT";
        case JoystickButton::DpadUp: return "DPAD_UP";
        case JoystickButton::DpadDown: return "DPAD_DOWN";
        case JoystickButton::DpadLeft: return "DPAD_LEFT";
        case JoystickButton::DpadRight: return "DPAD_RIGHT";
        case JoystickButton::Start: return "START";
        case JoystickButton::Back: return "BACK";
        case JoystickButton::L3: return "L3";
        case JoystickButton::R3: return "R3";
        default: return "A";
    }
}

MappingProfile JsonMappingParser::parseFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Cannot open: " + path);
    return parseFromString({std::istreambuf_iterator<char>(in), {}});
}

MappingProfile JsonMappingParser::parseFromString(const std::string& jsonStr) {
    const auto j = nlohmann::json::parse(jsonStr);
    MappingProfile p;
    p.id = j.value("id", "default");
    p.name = j.value("name", "Default");
    p.exclusiveCapture = j.value("exclusiveCapture", false);

    if (j.contains("camera") && j["camera"].is_object()) {
        const auto& c = j["camera"];
        p.camera.enabled = c.value("enabled", false);
        p.camera.stick = c.value("stick", "right");
        p.camera.requireRightButton = c.value("requireRightButton", true);
        p.camera.sensitivity = c.value("sensitivity", 14.f);
        p.camera.deadzone = c.value("deadzone", 0.15f);
        p.camera.invertY = c.value("invertY", true);
    }

    if (!j.contains("mappings")) return p;

    for (const auto& m : j["mappings"]) {
        JoystickMapping map;
        map.id = m.value("id", "");
        map.button = buttonFromString(m.value("joystickButton", "A"));
        map.buttons.clear();
        if (m.contains("buttons") && m["buttons"].is_array()) {
            for (const auto& b : m["buttons"]) {
                map.buttons.push_back(buttonFromString(b.get<std::string>()));
            }
        }
        normalizeMappingButtons(map);
        if (m.contains("key") && !m["key"].get<std::string>().empty()) {
            map.key = keyCodeFromName(m["key"].get<std::string>());
            map.hasKey = true;
        }
        if (m.contains("modifiers") && m["modifiers"].is_array()) {
            std::vector<std::string> names;
            for (const auto& mod : m["modifiers"]) names.push_back(mod.get<std::string>());
            map.modifiers = modifiersFromNames(names);
        }
        map.enabled = m.value("enabled", true);
        p.mappings.push_back(map);
    }
    dedupeProfileMappings(p.mappings);
    return p;
}

bool JsonMappingParser::saveToFile(const MappingProfile& profile, const std::string& path) {
    std::ofstream out(path);
    if (!out) return false;
    out << serialize(profile);
    return true;
}

std::string JsonMappingParser::serialize(const MappingProfile& profile) {
    nlohmann::json j;
    j["id"] = profile.id;
    j["name"] = profile.name;
    j["exclusiveCapture"] = profile.exclusiveCapture;
    j["camera"] = {
        {"enabled", profile.camera.enabled},
        {"stick", profile.camera.stick},
        {"requireRightButton", profile.camera.requireRightButton},
        {"sensitivity", profile.camera.sensitivity},
        {"deadzone", profile.camera.deadzone},
        {"invertY", profile.camera.invertY},
    };
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& m : profile.mappings) {
        nlohmann::json item;
        item["id"] = m.id;
        auto btns = mappingButtons(m);
        nlohmann::json btnArr = nlohmann::json::array();
        for (auto b : btns) btnArr.push_back(buttonToString(b));
        item["buttons"] = btnArr;
        item["joystickButton"] = buttonToString(btns.empty() ? m.button : btns.front());
        item["key"] = m.hasKey ? keyCodeToName(m.key) : "";
        item["modifiers"] = modifiersToNames(m.modifiers);
        item["enabled"] = m.enabled;
        arr.push_back(item);
    }
    j["mappings"] = arr;
    return j.dump(2);
}

}  // namespace inputflow
