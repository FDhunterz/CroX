#include "inputflow/core/key_release_guard.hpp"

namespace inputflow {

bool KeyReleaseGuard::HeldKey::operator<(const HeldKey& o) const {
    if (key != o.key) return key < o.key;
    return static_cast<uint8_t>(mods) < static_cast<uint8_t>(o.mods);
}

KeyReleaseGuard::KeyReleaseGuard(IInputProvider& input) : input_(input) {}

void KeyReleaseGuard::trackDown(KeyCode key, Modifier mods) {
    std::lock_guard lock(mutex_);
    held_.insert({key, mods});
}

void KeyReleaseGuard::trackUp(KeyCode key, Modifier mods) {
    std::lock_guard lock(mutex_);
    held_.erase({key, mods});
}

void KeyReleaseGuard::releaseAll() {
    std::set<HeldKey> copy;
    {
        std::lock_guard lock(mutex_);
        copy = std::move(held_);
        held_.clear();
    }
    for (const auto& h : copy) {
        input_.sendKeyUp(h.key, h.mods);
    }
    input_.releaseAllHeld();
}

}  // namespace inputflow
