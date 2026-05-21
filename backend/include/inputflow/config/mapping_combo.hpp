#pragma once

#include "inputflow/types.hpp"
#include "inputflow/joystick/i_joystick_provider.hpp"

namespace inputflow {

void normalizeMappingButtons(JoystickMapping& m);
std::vector<JoystickButton> mappingButtons(const JoystickMapping& m);
std::string comboSignature(const std::vector<JoystickButton>& buttons);
bool allComboPressed(const JoystickMapping& m, const JoystickState& state);
void dedupeProfileMappings(std::vector<JoystickMapping>& mappings);

}  // namespace inputflow
