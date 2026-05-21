#pragma once

#include <vector>
#include "inputflow/types.hpp"

namespace inputflow {

class MappingValidator {
public:
    /// Validasi: tidak ada tombol joystick duplikat, tidak ada output keyboard duplikat.
    static std::vector<ValidationError> validate(const std::vector<JoystickMapping>& mappings);

    static std::string outputSignature(bool hasKey, KeyCode key, Modifier mods);
};

}  // namespace inputflow
