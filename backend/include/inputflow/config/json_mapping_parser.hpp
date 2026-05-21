#pragma once

#include <string>
#include "inputflow/types.hpp"

namespace inputflow {

class JsonMappingParser {
public:
    static MappingProfile parseFromFile(const std::string& path);
    static MappingProfile parseFromString(const std::string& json);
    static bool saveToFile(const MappingProfile& profile, const std::string& path);
    static std::string serialize(const MappingProfile& profile);

    static JoystickButton buttonFromString(const std::string& s);
    static std::string buttonToString(JoystickButton b);
};

}  // namespace inputflow
