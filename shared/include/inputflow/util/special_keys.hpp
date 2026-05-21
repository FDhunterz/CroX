#pragma once

#include "inputflow/types.hpp"

namespace inputflow {

constexpr KeyCode kKeyMouseLeft  = 0xE0000001u;
constexpr KeyCode kKeyMouseRight = 0xE0000002u;

inline bool isMouseKey(KeyCode code) {
    return code == kKeyMouseLeft || code == kKeyMouseRight;
}

}  // namespace inputflow
