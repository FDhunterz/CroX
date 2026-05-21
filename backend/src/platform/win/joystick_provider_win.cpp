#include "inputflow/joystick/i_joystick_provider.hpp"
#include <Windows.h>
#include <XInput.h>
#include <memory>

#pragma comment(lib, "xinput.lib")

namespace inputflow {
namespace {

size_t buttonIndex(JoystickButton b) {
    return static_cast<size_t>(b);
}

/// Cari gamepad XInput pertama yang terhubung (slot 0–3).
int findConnectedXInputIndex() {
    for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i) {
        XINPUT_STATE state{};
        if (XInputGetState(i, &state) == ERROR_SUCCESS) return static_cast<int>(i);
    }
    return -1;
}

class WinJoystickProvider final : public IJoystickProvider {
public:
    std::string name() const override { return "xinput"; }
    std::string captureMode() const override { return "shared"; }

    bool initialize() override {
        deviceIndex_ = findConnectedXInputIndex();
        return true;
    }

    void shutdown() override { deviceIndex_ = -1; }

    bool isConnected() const override {
        if (deviceIndex_ < 0) return findConnectedXInputIndex() >= 0;
        XINPUT_STATE state{};
        return XInputGetState(static_cast<DWORD>(deviceIndex_), &state) == ERROR_SUCCESS;
    }

    bool poll(JoystickState& out) override {
        out.pressed.reset();
        if (deviceIndex_ < 0) deviceIndex_ = findConnectedXInputIndex();
        if (deviceIndex_ < 0) return false;

        XINPUT_STATE state{};
        if (XInputGetState(static_cast<DWORD>(deviceIndex_), &state) != ERROR_SUCCESS) {
            deviceIndex_ = findConnectedXInputIndex();
            if (deviceIndex_ < 0) return false;
            if (XInputGetState(static_cast<DWORD>(deviceIndex_), &state) != ERROR_SUCCESS) return false;
        }

        const WORD b = state.Gamepad.wButtons;
        auto set = [&](JoystickButton btn, bool pressed) {
            if (pressed) out.pressed.set(buttonIndex(btn));
        };

        set(JoystickButton::A, b & XINPUT_GAMEPAD_A);
        set(JoystickButton::B, b & XINPUT_GAMEPAD_B);
        set(JoystickButton::X, b & XINPUT_GAMEPAD_X);
        set(JoystickButton::Y, b & XINPUT_GAMEPAD_Y);
        set(JoystickButton::LB, b & XINPUT_GAMEPAD_LEFT_SHOULDER);
        set(JoystickButton::RB, b & XINPUT_GAMEPAD_RIGHT_SHOULDER);
        set(JoystickButton::LT, state.Gamepad.bLeftTrigger > 128);
        set(JoystickButton::RT, state.Gamepad.bRightTrigger > 128);
        set(JoystickButton::DpadUp, b & XINPUT_GAMEPAD_DPAD_UP);
        set(JoystickButton::DpadDown, b & XINPUT_GAMEPAD_DPAD_DOWN);
        set(JoystickButton::DpadLeft, b & XINPUT_GAMEPAD_DPAD_LEFT);
        set(JoystickButton::DpadRight, b & XINPUT_GAMEPAD_DPAD_RIGHT);
        set(JoystickButton::Start, b & XINPUT_GAMEPAD_START);
        set(JoystickButton::Back, b & XINPUT_GAMEPAD_BACK);
        set(JoystickButton::L3, b & XINPUT_GAMEPAD_LEFT_THUMB);
        set(JoystickButton::R3, b & XINPUT_GAMEPAD_RIGHT_THUMB);
        return true;
    }

private:
    int deviceIndex_{-1};
};

}  // namespace

std::unique_ptr<IJoystickProvider> createWinJoystickProvider(bool /*exclusiveCapture*/) {
    return std::make_unique<WinJoystickProvider>();
}

}  // namespace inputflow
