#include "inputflow/keyboard/i_keyboard_hook.hpp"
#include "inputflow/types.hpp"
#include <ApplicationServices/ApplicationServices.h>
#include <atomic>
#include <memory>

namespace inputflow {
namespace {

std::atomic<CFMachPortRef> g_tap{nullptr};
KeyboardCallback g_callback;

Modifier modifiersFromCg(CGEventFlags flags) {
    Modifier m = Modifier::None;
    if (flags & kCGEventFlagMaskControl) m = m | Modifier::Ctrl;
    if (flags & kCGEventFlagMaskShift) m = m | Modifier::Shift;
    if (flags & kCGEventFlagMaskAlternate) m = m | Modifier::Alt;
    if (flags & kCGEventFlagMaskCommand) m = m | Modifier::Meta;
    return m;
}

CGEventRef tapCallback(CGEventTapProxy, CGEventType type, CGEventRef event, void*) {
    if (!g_callback) return event;

  if (type != kCGEventKeyDown && type != kCGEventKeyUp && type != kCGEventFlagsChanged) {
        return event;
    }

    const auto keyCode = static_cast<KeyCode>(CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode));
    KeyEvent ev;
    ev.key = keyCode;
    ev.modifiers = modifiersFromCg(CGEventGetFlags(event));
    ev.timestamp = std::chrono::steady_clock::now();
    ev.action = (type == kCGEventKeyUp) ? KeyAction::Up : KeyAction::Down;

    g_callback(ev);
    return event;
}

class MacKeyboardHook final : public IKeyboardHook {
public:
    bool install(KeyboardCallback callback) override {
        g_callback = std::move(callback);

        const CGEventMask mask = CGEventMaskBit(kCGEventKeyDown) |
                                 CGEventMaskBit(kCGEventKeyUp) |
                                 CGEventMaskBit(kCGEventFlagsChanged);

        g_tap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap,
                                 kCGEventTapOptionListenOnly, mask, tapCallback, nullptr);
        if (!g_tap) return false;

        CFRunLoopSourceRef source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, g_tap, 0);
        CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopCommonModes);
        CGEventTapEnable(g_tap, true);
        CFRelease(source);
        return true;
    }

    void uninstall() override {
        if (g_tap) {
            CGEventTapEnable(g_tap, false);
            CFRelease(g_tap);
            g_tap = nullptr;
        }
        g_callback = nullptr;
    }

    bool isInstalled() const override { return g_tap != nullptr; }
};

}  // namespace

std::unique_ptr<IKeyboardHook> createPlatformKeyboardHook() {
    return std::make_unique<MacKeyboardHook>();
}

}  // namespace inputflow
