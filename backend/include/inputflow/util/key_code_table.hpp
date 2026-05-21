#pragma once

#include <optional>
#include <string>
#include "inputflow/types.hpp"

namespace inputflow {

KeyCode keyCodeFromName(const std::string& name);
std::string keyCodeToName(KeyCode code);
Modifier modifiersFromNames(const std::vector<std::string>& names);
std::vector<std::string> modifiersToNames(Modifier mods);

}  // namespace inputflow
