#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace inputflow {

using KeyCode = uint32_t;

enum class Modifier : uint8_t {
    None  = 0,
    Ctrl  = 1 << 0,
    Shift = 1 << 1,
    Alt   = 1 << 2,
    Meta  = 1 << 3,
};

inline Modifier operator|(Modifier a, Modifier b) {
    return static_cast<Modifier>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline bool hasModifier(Modifier flags, Modifier flag) {
    return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(flag)) != 0;
}

/// Tombol joystick yang didukung (Xbox layout).
enum class JoystickButton : uint8_t {
    A,
    B,
    X,
    Y,
    LB,
    RB,
    LT,
    RT,
    DpadUp,
    DpadDown,
    DpadLeft,
    DpadRight,
    Start,
    Back,
    L3,
    R3,
};

/// Satu pemetaan: tombol joystick (atau kombinasi) → keyboard (+ modifier opsional).
struct JoystickMapping {
    std::string id;
    /// Semua tombol harus ditekan bersamaan (mis. RB + A).
    std::vector<JoystickButton> buttons;
    /// Legacy / kompatibilitas JSON lama (tombol pertama).
    JoystickButton button{JoystickButton::A};
    KeyCode key{0};
    /// true jika field "key" di JSON diisi (penting: kVK_ANSI_A == 0 di macOS)
    bool hasKey{false};
    Modifier modifiers{Modifier::None};
    bool enabled{true};
};

/// Kamera: right stick + tombol kanan mouse (swipe arah).
struct CameraControlSettings {
    bool enabled{false};
    /// right | left
    std::string stick{"right"};
    bool requireRightButton{true};
    float sensitivity{14.f};
    float deadzone{0.15f};
    bool invertY{true};
};

struct MappingProfile {
    std::string id{"default"};
    std::string name{"Default"};
    std::vector<JoystickMapping> mappings;
    /// true = coba rebut akses HID agar app lain tidak terima input gamepad (macOS)
    bool exclusiveCapture{false};
    CameraControlSettings camera;
};

enum class EngineState : uint8_t {
    Stopped,
    Running,
    Error,
};

struct ValidationError {
    std::string field;
    std::string message;
};

}  // namespace inputflow
