#pragma once

#include <string>
#include "inputflow/types.hpp"

namespace inputflow {

class JsonProfileParser {
public:
    static Profile parseFromFile(const std::string& path);
    static Profile parseFromString(const std::string& json);
    static bool saveToFile(const Profile& profile, const std::string& path);
    static std::string serialize(const Profile& profile);
};

}  // namespace inputflow
