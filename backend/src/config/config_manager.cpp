#include "inputflow/config/config_manager.hpp"
#include "inputflow/config/json_profile_parser.hpp"
#include <filesystem>

namespace fs = std::filesystem;

namespace inputflow {

ConfigManager::ConfigManager(std::string profilesDirectory)
    : profilesDirectory_(std::move(profilesDirectory)) {
    fs::create_directories(profilesDirectory_);
    Profile defaultProfile;
    defaultProfile.id = "default";
    defaultProfile.name = "Default";
    activeProfile_ = std::move(defaultProfile);
}

bool ConfigManager::loadProfile(const std::string& profileId) {
    const auto path = (fs::path(profilesDirectory_) / (profileId + ".json")).string();
    if (!fs::exists(path)) return false;
    activeProfile_ = JsonProfileParser::parseFromFile(path);
    return true;
}

bool ConfigManager::saveProfile(const Profile& profile) const {
    const auto path = (fs::path(profilesDirectory_) / (profile.id + ".json")).string();
    return JsonProfileParser::saveToFile(profile, path);
}

bool ConfigManager::deleteProfile(const std::string& profileId) const {
    const auto path = fs::path(profilesDirectory_) / (profileId + ".json");
    if (!fs::exists(path)) return false;
    return fs::remove(path);
}

std::vector<std::string> ConfigManager::listProfiles() const {
    std::vector<std::string> ids;
    if (!fs::exists(profilesDirectory_)) return ids;
    for (const auto& entry : fs::directory_iterator(profilesDirectory_)) {
        if (entry.path().extension() == ".json") {
            ids.push_back(entry.path().stem().string());
        }
    }
    return ids;
}

const Profile& ConfigManager::activeProfile() const { return activeProfile_; }
Profile& ConfigManager::activeProfile() { return activeProfile_; }

void ConfigManager::setActiveProfile(Profile profile) {
    activeProfile_ = std::move(profile);
}

std::string ConfigManager::profilesPath() const { return profilesDirectory_; }

}  // namespace inputflow
