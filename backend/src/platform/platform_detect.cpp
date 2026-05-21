#include <string>

namespace inputflow {

std::string platformName() {
#if defined(INPUTFLOW_PLATFORM_WINDOWS)
    return "windows";
#elif defined(INPUTFLOW_PLATFORM_MACOS)
    return "macos";
#else
    return "unknown";
#endif
}

}  // namespace inputflow
