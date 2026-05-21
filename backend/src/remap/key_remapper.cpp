#include "inputflow/remap/key_remapper.hpp"

namespace inputflow {

KeyRemapper::KeyRemapper(IInputProvider& input) : input_(input) {}

void KeyRemapper::setBindings(const std::vector<KeyBinding>& bindings) {
    bindings_ = bindings;
}

bool KeyRemapper::tryRemap(const KeyEvent& event) {
    if (event.action != KeyAction::Down) return false;

    for (const auto& b : bindings_) {
        if (!b.enabled) continue;
        if (b.triggerKey == event.key && b.triggerModifiers == event.modifiers) {
            input_.sendKeyPress(b.outputKey, b.outputModifiers);
            return true;
        }
    }
    return false;
}

}  // namespace inputflow
