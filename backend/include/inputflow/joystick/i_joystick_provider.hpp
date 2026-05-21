#pragma once

#include <bitset>
#include <cstddef>
#include <functional>
#include <memory>
#include "inputflow/types.hpp"

namespace inputflow {

struct JoystickState {
    std::bitset<16> pressed{};
    float lx{0.f};
    float ly{0.f};
    float rx{0.f};
    float ry{0.f};
};

class IJoystickProvider {
public:
    virtual ~IJoystickProvider() = default;

    virtual std::string name() const = 0;
    /// shared = GameController/XInput, exclusive = HID seize (macOS)
    virtual std::string captureMode() const { return "shared"; }
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool poll(JoystickState& out) = 0;
    virtual bool isConnected() const = 0;
};

std::unique_ptr<IJoystickProvider> createPlatformJoystickProvider(bool exclusiveCapture = false);

}  // namespace inputflow
