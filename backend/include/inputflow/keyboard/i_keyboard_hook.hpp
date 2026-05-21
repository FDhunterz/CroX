#pragma once

#include <functional>
#include <memory>
#include "inputflow/types.hpp"

namespace inputflow {

using KeyboardCallback = std::function<void(const KeyEvent&)>;

/// Low-level global keyboard hook (platform implementation).
class IKeyboardHook {
public:
    virtual ~IKeyboardHook() = default;

    virtual bool install(KeyboardCallback callback) = 0;
    virtual void uninstall() = 0;
    virtual bool isInstalled() const = 0;
};

std::unique_ptr<IKeyboardHook> createPlatformKeyboardHook();

}  // namespace inputflow
