#include "inputflow/input/i_input_provider.hpp"
#include <memory>

namespace inputflow {

#if defined(INPUTFLOW_PLATFORM_WINDOWS)
std::unique_ptr<IInputProvider> createWinInputProvider();
#endif
#if defined(INPUTFLOW_PLATFORM_MACOS)
std::unique_ptr<IInputProvider> createMacInputProvider();
#endif

std::unique_ptr<IInputProvider> createPlatformInputProvider() {
#if defined(INPUTFLOW_PLATFORM_WINDOWS)
    return createWinInputProvider();
#elif defined(INPUTFLOW_PLATFORM_MACOS)
    return createMacInputProvider();
#else
    struct NullProvider : IInputProvider {
        std::string name() const override { return "null"; }
        bool initialize() override { return false; }
        void shutdown() override {}
        bool sendKeyDown(KeyCode, Modifier) override { return false; }
        bool sendKeyUp(KeyCode, Modifier) override { return false; }
        bool sendKeyPress(KeyCode, Modifier) override { return false; }
        void releaseAllHeld() override {}
    };
    return std::make_unique<NullProvider>();
#endif
}

}  // namespace inputflow
