#pragma once

#include <optional>
#include <string>
#include <vector>
#include "inputflow/types.hpp"

namespace inputflow {

class ConfigManager {
public:
    explicit ConfigManager(std::string profilesDirectory);

    bool loadProfile(const std::string& profileId);
    bool saveProfile(const Profile& profile) const;
    bool deleteProfile(const std::string& profileId) const;

    std::vector<std::string> listProfiles() const;
    const Profile& activeProfile() const;
    Profile& activeProfile();
    void setActiveProfile(Profile profile);

    std::string profilesPath() const;

private:
    std::string profilesDirectory_;
    Profile activeProfile_;
};

}  // namespace inputflow
