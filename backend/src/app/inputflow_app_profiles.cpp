#include "inputflow/app/inputflow_app.hpp"
#include "inputflow/config/json_mapping_parser.hpp"
#include "inputflow/config/profile_manifest.hpp"
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

namespace inputflow {

std::string InputFlowApp::activeProfileName() const {
    return activeProfile_;
}

std::vector<std::string> InputFlowApp::listProfiles() const {
    ProfileManifestStore store(configDirectory_);
    auto names = store.listProfileFiles();
    const auto manifest = store.load();
    for (const auto& p : manifest.profiles) {
        if (std::find(names.begin(), names.end(), p) == names.end()) {
            names.push_back(p);
        }
    }
    std::sort(names.begin(), names.end());
    if (names.empty()) return {"default"};
    return names;
}

bool InputFlowApp::switchProfile(const std::string& name) {
    if (!ProfileManifestStore::isValidProfileName(name)) return false;
    const auto path = configPath(name);
    if (!fs::exists(path)) return false;

    saveConfig(activeProfile_);
    if (!loadConfig(name)) return false;
    activeProfile_ = name;

    ProfileManifestStore store(configDirectory_);
    auto manifest = store.load();
    manifest.active = name;
    if (std::find(manifest.profiles.begin(), manifest.profiles.end(), name) ==
        manifest.profiles.end()) {
        manifest.profiles.push_back(name);
    }
    store.save(manifest);
    return true;
}

bool InputFlowApp::createProfile(const std::string& name, bool copyCurrent) {
    if (!ProfileManifestStore::isValidProfileName(name)) return false;
    const auto path = configPath(name);
    if (fs::exists(path)) return false;

    fs::create_directories(configDirectory_);

    MappingProfile p;
    if (copyCurrent) {
        p = profile_;
    } else {
        p.id = name;
        p.name = name;
        p.exclusiveCapture = false;
        p.mappings.clear();
        p.camera = CameraControlSettings{};
    }
    p.id = name;
    if (p.name.empty()) p.name = name;

    if (!JsonMappingParser::saveToFile(p, path)) return false;

    ProfileManifestStore store(configDirectory_);
    auto manifest = store.load();
    if (std::find(manifest.profiles.begin(), manifest.profiles.end(), name) ==
        manifest.profiles.end()) {
        manifest.profiles.push_back(name);
    }
    manifest.active = name;
    store.save(manifest);

    activeProfile_ = name;
    profile_ = std::move(p);
    profile_.id = name;
    if (engine_) {
        engine_->setMappings(profile_.mappings);
        engine_->setCamera(profile_.camera);
    }
    return true;
}

bool InputFlowApp::deleteProfile(const std::string& name) {
    if (!ProfileManifestStore::isValidProfileName(name)) return false;
    if (name == activeProfile_) return false;

    ProfileManifestStore store(configDirectory_);
    auto manifest = store.load();
    if (manifest.profiles.size() <= 1) return false;

    const auto path = configPath(name);
    if (fs::exists(path)) fs::remove(path);

    manifest.profiles.erase(
        std::remove(manifest.profiles.begin(), manifest.profiles.end(), name),
        manifest.profiles.end());
    store.save(manifest);
    return true;
}

bool InputFlowApp::renameProfile(const std::string& from, const std::string& to) {
    if (!ProfileManifestStore::isValidProfileName(from) ||
        !ProfileManifestStore::isValidProfileName(to)) {
        return false;
    }
    if (!fs::exists(configPath(from)) || fs::exists(configPath(to))) return false;

    fs::rename(configPath(from), configPath(to));

    ProfileManifestStore store(configDirectory_);
    auto manifest = store.load();
    for (auto& p : manifest.profiles) {
        if (p == from) p = to;
    }
    if (manifest.active == from) manifest.active = to;
    if (activeProfile_ == from) activeProfile_ = to;
    store.save(manifest);
    return true;
}

}  // namespace inputflow
