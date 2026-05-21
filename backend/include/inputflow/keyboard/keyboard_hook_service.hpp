#pragma once

#include <memory>
#include "inputflow/keyboard/i_keyboard_hook.hpp"
#include "inputflow/core/i_event_bus.hpp"

namespace inputflow {

class KeyboardHookService {
public:
    explicit KeyboardHookService(IEventBus& bus);

    bool start();
    void stop();

private:
    void onRawKey(const KeyEvent& event);

    IEventBus& bus_;
    std::unique_ptr<IKeyboardHook> hook_;
};

}  // namespace inputflow
