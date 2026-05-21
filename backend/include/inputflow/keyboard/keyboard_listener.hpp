#pragma once

#include "inputflow/keyboard/i_keyboard_hook.hpp"

namespace inputflow {

/// High-level listener API (wraps platform hook).
class KeyboardListener {
public:
    bool start(KeyboardCallback cb);
    void stop();

private:
    std::unique_ptr<IKeyboardHook> hook_;
};

}  // namespace inputflow
