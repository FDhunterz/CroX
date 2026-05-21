#include "inputflow/input/i_input_provider.hpp"
#include <Windows.h>
#include <memory>
#include <set>
#include <mutex>

namespace inputflow {
namespace {

WORD modifierVk(Modifier m, bool down) {
    (void)down;
    if (hasModifier(m, Modifier::Ctrl)) return VK_CONTROL;
    return 0;
}

class WinInputProvider final : public IInputProvider {
public:
    std::string name() const override { return "win32_sendinput"; }

    bool initialize() override { return true; }
    void shutdown() override { releaseAllHeld(); }

    bool sendKeyDown(KeyCode key, Modifier mods) override {
        sendModifiers(mods, true);
        INPUT in{};
        in.type = INPUT_KEYBOARD;
        in.ki.wVk = static_cast<WORD>(key);
        in.ki.dwFlags = 0;
        const bool ok = SendInput(1, &in, sizeof(INPUT)) == 1;
        if (ok) track(key, mods);
        return ok;
    }

    bool sendKeyUp(KeyCode key, Modifier mods) override {
        INPUT in{};
        in.type = INPUT_KEYBOARD;
        in.ki.wVk = static_cast<WORD>(key);
        in.ki.dwFlags = KEYEVENTF_KEYUP;
        const bool ok = SendInput(1, &in, sizeof(INPUT)) == 1;
        untrack(key, mods);
        sendModifiers(mods, false);
        return ok;
    }

    bool sendKeyPress(KeyCode key, Modifier mods) override {
        return sendKeyDown(key, mods) && sendKeyUp(key, mods);
    }

    bool sendModifiersDown(Modifier mods) override {
        sendModifiers(mods, true);
        return true;
    }

    bool sendModifiersUp(Modifier mods) override {
        sendModifiers(mods, false);
        return true;
    }

    void releaseAllHeld() override {
        std::set<std::pair<KeyCode, Modifier>> copy;
        {
            std::lock_guard lock(mutex_);
            copy.swap(held_);
        }
        for (const auto& [k, m] : copy) sendKeyUp(k, m);
    }

private:
    void sendModifiers(Modifier mods, bool down) {
        auto sendMod = [&](WORD vk) {
            INPUT in{};
            in.type = INPUT_KEYBOARD;
            in.ki.wVk = vk;
            in.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
            SendInput(1, &in, sizeof(INPUT));
        };
        if (hasModifier(mods, Modifier::Ctrl)) sendMod(VK_CONTROL);
        if (hasModifier(mods, Modifier::Shift)) sendMod(VK_SHIFT);
        if (hasModifier(mods, Modifier::Alt)) sendMod(VK_MENU);
        if (hasModifier(mods, Modifier::Meta)) sendMod(VK_LWIN);
    }

    void track(KeyCode k, Modifier m) {
        std::lock_guard lock(mutex_);
        held_.insert({k, m});
    }
    void untrack(KeyCode k, Modifier m) {
        std::lock_guard lock(mutex_);
        held_.erase({k, m});
    }

    std::mutex mutex_;
    std::set<std::pair<KeyCode, Modifier>> held_;
};

}  // namespace

std::unique_ptr<IInputProvider> createWinInputProvider() {
    return std::make_unique<WinInputProvider>();
}

}  // namespace inputflow
