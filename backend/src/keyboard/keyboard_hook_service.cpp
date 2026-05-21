#include "inputflow/keyboard/keyboard_hook_service.hpp"
#include "inputflow/keyboard/i_keyboard_hook.hpp"

namespace inputflow {

KeyboardHookService::KeyboardHookService(IEventBus& bus) : bus_(bus) {}

bool KeyboardHookService::start() {
    if (!hook_) hook_ = createPlatformKeyboardHook();
    return hook_->install([this](const KeyEvent& e) { onRawKey(e); });
}

void KeyboardHookService::stop() {
    if (hook_) hook_->uninstall();
}

void KeyboardHookService::onRawKey(const KeyEvent& event) {
    bus_.publish(TOPIC_KEY_EVENT, &event);
}

}  // namespace inputflow
