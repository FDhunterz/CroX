#include "inputflow/config/profile_manifest.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

namespace inputflow {

ProfileManifestStore::ProfileManifestStore(std::string configDirectory)
    : configDirectory_(std::move(configDirectory)) {}

std::string ProfileManifestStore::manifestPath() const {
    return (fs::path(configDirectory_) / "manifest.json").string();
}

bool ProfileManifestStore::isValidProfileName(const std::string& name) {
    if (name.empty() || name == "manifest") return false;
    for (char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c))) continue;
        if (c == '-' || c == '_') continue;
        return false;
    }
    return true;
}

ProfileManifest ProfileManifestStore::load() const {
    ProfileManifest m;
    m.profiles = {"default"};
    m.active = "default";

    const auto path = manifestPath();
    if (!fs::exists(path)) return m;

    try {
        std::ifstream in(path);
        const auto j = nlohmann::json::parse(in);
        m.active = j.value("active", "default");
        if (j.contains("profiles") && j["profiles"].is_array()) {
            m.profiles.clear();
            for (const auto& p : j["profiles"]) {
                const auto name = p.get<std::string>();
                if (isValidProfileName(name)) m.profiles.push_back(name);
            }
        }
        if (m.profiles.empty()) m.profiles = {"default"};
    } catch (...) {}

    return m;
}

bool ProfileManifestStore::save(const ProfileManifest& manifest) const {
    fs::create_directories(configDirectory_);
    nlohmann::json j;
    j["active"] = manifest.active;
    j["profiles"] = manifest.profiles;
    std::ofstream out(manifestPath());
    if (!out) return false;
    out << j.dump(2);
    return true;
}

std::vector<std::string> ProfileManifestStore::listProfileFiles() const {
    std::vector<std::string> names;
    if (!fs::exists(configDirectory_)) return names;
    for (const auto& entry : fs::directory_iterator(configDirectory_)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".json") continue;
        const auto stem = entry.path().stem().string();
        if (stem == "manifest") continue;
        if (isValidProfileName(stem)) names.push_back(stem);
    }
    std::sort(names.begin(), names.end());
    return names;
}

}  // namespace inputflow
