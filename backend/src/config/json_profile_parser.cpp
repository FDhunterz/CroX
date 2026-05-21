#include "inputflow/config/json_profile_parser.hpp"
#include "inputflow/util/key_code_table.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

namespace inputflow {
namespace {

Modifier parseModifiers(const nlohmann::json& arr) {
    if (!arr.is_array()) return Modifier::None;
    std::vector<std::string> names;
    for (const auto& v : arr) names.push_back(v.get<std::string>());
    return modifiersFromNames(names);
}

MacroStepType stepTypeFromString(const std::string& s) {
    if (s == "keyPress") return MacroStepType::KeyPress;
    if (s == "keyDown") return MacroStepType::KeyDown;
    if (s == "keyUp") return MacroStepType::KeyUp;
    if (s == "combo") return MacroStepType::Combo;
    return MacroStepType::Delay;
}

std::string stepTypeToString(MacroStepType t) {
    switch (t) {
        case MacroStepType::KeyPress: return "keyPress";
        case MacroStepType::KeyDown: return "keyDown";
        case MacroStepType::KeyUp: return "keyUp";
        case MacroStepType::Combo: return "combo";
        default: return "delay";
    }
}

HotkeyBinding parseHotkey(const nlohmann::json& j) {
    HotkeyBinding h;
    if (j.contains("key")) h.key = keyCodeFromName(j["key"].get<std::string>());
    if (j.contains("modifiers")) h.modifiers = parseModifiers(j["modifiers"]);
    if (j.contains("action")) h.action = j["action"].get<std::string>();
    return h;
}

}  // namespace

Profile JsonProfileParser::parseFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Cannot open profile: " + path);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return parseFromString(content);
}

Profile JsonProfileParser::parseFromString(const std::string& jsonStr) {
    const auto j = nlohmann::json::parse(jsonStr);
    Profile p;
    p.id = j.value("id", "default");
    p.name = j.value("name", "Unnamed");
    p.debounceMs = j.value("debounceMs", 50u);

    if (j.contains("remaps")) {
        for (const auto& r : j["remaps"]) {
            KeyBinding b;
            b.triggerKey = keyCodeFromName(r["triggerKey"].get<std::string>());
            b.outputKey = keyCodeFromName(r["outputKey"].get<std::string>());
            if (r.contains("triggerModifiers")) b.triggerModifiers = parseModifiers(r["triggerModifiers"]);
            if (r.contains("outputModifiers")) b.outputModifiers = parseModifiers(r["outputModifiers"]);
            b.enabled = r.value("enabled", true);
            p.remaps.push_back(b);
        }
    }

    if (j.contains("macros")) {
        for (const auto& m : j["macros"]) {
            Macro macro;
            macro.id = m["id"].get<std::string>();
            macro.name = m.value("name", macro.id);
            macro.enabled = m.value("enabled", true);
            for (const auto& s : m["steps"]) {
                MacroStep step;
                step.type = stepTypeFromString(s["type"].get<std::string>());
                if (s.contains("key")) step.key = keyCodeFromName(s["key"].get<std::string>());
                if (s.contains("modifiers")) step.modifiers = parseModifiers(s["modifiers"]);
                step.delayMs = s.value("delayMs", 0u);
                if (s.contains("comboKeys")) {
                    for (const auto& k : s["comboKeys"]) {
                        step.comboKeys.push_back(keyCodeFromName(k.get<std::string>()));
                    }
                }
                macro.steps.push_back(step);
            }
            p.macros.push_back(macro);
        }
    }

    if (j.contains("startHotkey")) p.startHotkey = parseHotkey(j["startHotkey"]);
    if (j.contains("stopHotkey")) p.stopHotkey = parseHotkey(j["stopHotkey"]);

    return p;
}

bool JsonProfileParser::saveToFile(const Profile& profile, const std::string& path) {
    std::ofstream out(path);
    if (!out) return false;
    out << serialize(profile);
    return true;
}

std::string JsonProfileParser::serialize(const Profile& profile) {
    nlohmann::json j;
    j["id"] = profile.id;
    j["name"] = profile.name;
    j["debounceMs"] = profile.debounceMs;

    nlohmann::json remaps = nlohmann::json::array();
    for (const auto& b : profile.remaps) {
        remaps.push_back({
            {"triggerKey", keyCodeToName(b.triggerKey)},
            {"triggerModifiers", modifiersToNames(b.triggerModifiers)},
            {"outputKey", keyCodeToName(b.outputKey)},
            {"outputModifiers", modifiersToNames(b.outputModifiers)},
            {"enabled", b.enabled},
        });
    }
    j["remaps"] = remaps;

    nlohmann::json macros = nlohmann::json::array();
    for (const auto& m : profile.macros) {
        nlohmann::json steps = nlohmann::json::array();
        for (const auto& s : m.steps) {
            nlohmann::json sj;
            sj["type"] = stepTypeToString(s.type);
            if (s.type != MacroStepType::Delay) sj["key"] = keyCodeToName(s.key);
            sj["modifiers"] = modifiersToNames(s.modifiers);
            if (s.type == MacroStepType::Delay) sj["delayMs"] = s.delayMs;
            if (s.type == MacroStepType::Combo) {
                nlohmann::json keys = nlohmann::json::array();
                for (KeyCode k : s.comboKeys) keys.push_back(keyCodeToName(k));
                sj["comboKeys"] = keys;
            }
            steps.push_back(sj);
        }
        macros.push_back({
            {"id", m.id},
            {"name", m.name},
            {"enabled", m.enabled},
            {"steps", steps},
        });
    }
    j["macros"] = macros;

    auto hotkeyJson = [](const HotkeyBinding& h) {
        return nlohmann::json{
            {"key", keyCodeToName(h.key)},
            {"modifiers", modifiersToNames(h.modifiers)},
            {"action", h.action},
        };
    };
    j["startHotkey"] = hotkeyJson(profile.startHotkey);
    j["stopHotkey"] = hotkeyJson(profile.stopHotkey);

    return j.dump(2);
}

}  // namespace inputflow
