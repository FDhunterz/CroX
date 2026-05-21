#include "inputflow/joystick/i_joystick_provider.hpp"
#include <memory>

namespace inputflow {

#if defined(INPUTFLOW_PLATFORM_WINDOWS)
std::unique_ptr<IJoystickProvider> createWinJoystickProvider(bool exclusiveCapture);
#endif
#if defined(INPUTFLOW_PLATFORM_MACOS)
std::unique_ptr<IJoystickProvider> createMacJoystickProvider(bool exclusiveCapture);
#endif

std::unique_ptr<IJoystickProvider> createPlatformJoystickProvider(bool exclusiveCapture) {
#if defined(INPUTFLOW_PLATFORM_WINDOWS)
    return createWinJoystickProvider(exclusiveCapture);
#elif defined(INPUTFLOW_PLATFORM_MACOS)
    return createMacJoystickProvider(exclusiveCapture);
#else
    (void)exclusiveCapture;
    struct NullJoystick : IJoystickProvider {
        std::string name() const override { return "null"; }
        bool initialize() override { return false; }
        void shutdown() override {}
        bool poll(JoystickState&) override { return false; }
        bool isConnected() const override { return false; }
    };
    return std::make_unique<NullJoystick>();
#endif
}

}  // namespace inputflow
