#pragma once

#include <string>
#include <vector>

namespace inputflow {

struct ProfileManifest {
    std::string active{"default"};
    std::vector<std::string> profiles;
};

class ProfileManifestStore {
public:
    explicit ProfileManifestStore(std::string configDirectory);

    ProfileManifest load() const;
    bool save(const ProfileManifest& manifest) const;

    std::vector<std::string> listProfileFiles() const;
    static bool isValidProfileName(const std::string& name);

private:
    std::string manifestPath() const;

    std::string configDirectory_;
};

}  // namespace inputflow
