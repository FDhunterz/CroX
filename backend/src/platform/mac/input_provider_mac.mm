#include "inputflow/input/i_input_provider.hpp"
#include "inputflow/util/special_keys.hpp"
#import <Foundation/Foundation.h>
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <set>

namespace inputflow {
namespace {

CGEventFlags flagsFromModifiers(Modifier mods) {
    CGEventFlags f = 0;
    if (hasModifier(mods, Modifier::Ctrl)) f |= kCGEventFlagMaskControl;
    if (hasModifier(mods, Modifier::Shift)) f |= kCGEventFlagMaskShift;
    if (hasModifier(mods, Modifier::Alt)) f |= kCGEventFlagMaskAlternate;
    if (hasModifier(mods, Modifier::Meta)) f |= kCGEventFlagMaskCommand;
    return f;
}

bool queryAccessibilityTrusted(bool prompt) {
    NSDictionary* opts = @{(__bridge id)kAXTrustedCheckOptionPrompt: @(prompt)};
    return AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)opts);
}

CGEventSourceRef makeEventSource() {
    return CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
}

void postEvent(CGEventRef ev) {
    if (!ev) return;
    // Annotated session tap delivers ke app yang sedang fokus (macOS modern).
    CGEventPost(kCGAnnotatedSessionEventTap, ev);
    CFRelease(ev);
}

CGPoint currentMousePoint() {
    const CGEventRef ev = CGEventCreate(nullptr);
    if (!ev) return CGPointZero;
    const CGPoint p = CGEventGetLocation(ev);
    CFRelease(ev);
    return p;
}

bool postMouseButton(CGMouseButton button, bool down) {
    const CGPoint pt = currentMousePoint();
    const CGEventType type = (button == kCGMouseButtonLeft)
        ? (down ? kCGEventLeftMouseDown : kCGEventLeftMouseUp)
        : (down ? kCGEventRightMouseDown : kCGEventRightMouseUp);
    CGEventRef ev = CGEventCreateMouseEvent(nullptr, type, pt, button);
    if (!ev) return false;
    postEvent(ev);
    return true;
}

CGEventRef makeKeyEvent(CGEventSourceRef source, KeyCode key, bool down, CGEventFlags extra) {
    CGEventRef ev = CGEventCreateKeyboardEvent(source, static_cast<CGKeyCode>(key), down);
    if (!ev) return nullptr;
    CGEventSetFlags(ev, CGEventGetFlags(ev) | extra);
    return ev;
}

class MacInputProvider final : public IInputProvider {
public:
    std::string name() const override { return "cg_event_post"; }

    bool initialize() override {
        refreshAccessibility();
        if (!trusted_) {
            std::cerr << "[InputFlow] Accessibility tidak aktif — keyboard tidak akan terkirim.\n";
            std::cerr << "[InputFlow] System Settings → Privacy & Security → Accessibility → InputFlow\n";
        }
        return true;
    }

    void shutdown() override { releaseAllHeld(); }

    void refreshAccessibility() override {
        trusted_ = queryAccessibilityTrusted(false);
    }

    bool accessibilityGranted() const override { return trusted_; }

    bool sendModifiersDown(Modifier mods) override {
        refreshAccessibility();
        if (!trusted_) return false;
        return postModifiers(mods, true);
    }

    bool sendModifiersUp(Modifier mods) override {
        refreshAccessibility();
        if (!trusted_) return false;
        return postModifiers(mods, false);
    }

    bool sendKeyDown(KeyCode key, Modifier mods) override {
        refreshAccessibility();
        if (!trusted_) return false;

        postModifiers(mods, true);

        if (isMouseKey(key)) {
            const auto btn = key == kKeyMouseLeft ? kCGMouseButtonLeft : kCGMouseButtonRight;
            if (!postMouseButton(btn, true)) return false;
            track(key, mods);
            return true;
        }

        CGEventSourceRef src = makeEventSource();
        CGEventRef ev = makeKeyEvent(src, key, true, flagsFromModifiers(mods));
        if (src) CFRelease(src);
        if (!ev) return false;
        postEvent(ev);
        track(key, mods);
        return true;
    }

    bool sendKeyUp(KeyCode key, Modifier mods) override {
        refreshAccessibility();
        if (!trusted_) return false;

        if (isMouseKey(key)) {
            const auto btn = key == kKeyMouseLeft ? kCGMouseButtonLeft : kCGMouseButtonRight;
            if (!postMouseButton(btn, false)) return false;
            untrack(key, mods);
            postModifiers(mods, false);
            return true;
        }

        CGEventSourceRef src = makeEventSource();
        CGEventRef ev = makeKeyEvent(src, key, false, flagsFromModifiers(mods));
        if (src) CFRelease(src);
        if (!ev) return false;
        postEvent(ev);
        untrack(key, mods);

        postModifiers(mods, false);
        return true;
    }

    bool sendKeyPress(KeyCode key, Modifier mods) override {
        return sendKeyDown(key, mods) && sendKeyUp(key, mods);
    }

    void releaseAllHeld() override {
        releaseCameraMouse();
        std::set<std::pair<KeyCode, Modifier>> copy;
        {
            std::lock_guard lock(mutex_);
            copy.swap(held_);
        }
        for (const auto& [k, m] : copy) sendKeyUp(k, m);
    }

    bool moveMouseRelative(int dx, int dy) override {
        refreshAccessibility();
        if (!trusted_ || (dx == 0 && dy == 0)) return false;
        const CGPoint p = currentMousePoint();
        const CGPoint next = {p.x + static_cast<CGFloat>(dx), p.y + static_cast<CGFloat>(dy)};
        CGEventType type = rightMouseHeld_
            ? kCGEventRightMouseDragged
            : kCGEventMouseMoved;
        const CGMouseButton btn = rightMouseHeld_ ? kCGMouseButtonRight : kCGMouseButtonLeft;
        CGEventRef ev = CGEventCreateMouseEvent(nullptr, type, next, btn);
        if (!ev) return false;
        postEvent(ev);
        return true;
    }

    bool rightMouseDown() override {
        refreshAccessibility();
        if (!trusted_ || rightMouseHeld_) return false;
        const CGPoint p = currentMousePoint();
        CGEventRef ev = CGEventCreateMouseEvent(nullptr, kCGEventRightMouseDown, p, kCGMouseButtonRight);
        if (!ev) return false;
        postEvent(ev);
        rightMouseHeld_ = true;
        return true;
    }

    bool rightMouseUp() override {
        refreshAccessibility();
        if (!trusted_ || !rightMouseHeld_) return false;
        const CGPoint p = currentMousePoint();
        CGEventRef ev = CGEventCreateMouseEvent(nullptr, kCGEventRightMouseUp, p, kCGMouseButtonRight);
        if (!ev) return false;
        postEvent(ev);
        rightMouseHeld_ = false;
        return true;
    }

    void releaseCameraMouse() override {
        if (rightMouseHeld_) rightMouseUp();
    }

private:
    bool postModifiers(Modifier mods, bool down) {
        auto post = [&](CGKeyCode code) {
            CGEventSourceRef src = makeEventSource();
            CGEventRef ev = CGEventCreateKeyboardEvent(src, code, down);
            if (src) CFRelease(src);
            postEvent(ev);
        };
        bool any = false;
        if (hasModifier(mods, Modifier::Meta)) { post(kVK_Command); any = true; }
        if (hasModifier(mods, Modifier::Ctrl)) { post(kVK_Control); any = true; }
        if (hasModifier(mods, Modifier::Alt)) { post(kVK_Option); any = true; }
        if (hasModifier(mods, Modifier::Shift)) { post(kVK_Shift); any = true; }
        return any;
    }

    void track(KeyCode k, Modifier m) {
        std::lock_guard lock(mutex_);
        held_.insert({k, m});
    }
    void untrack(KeyCode k, Modifier m) {
        std::lock_guard lock(mutex_);
        held_.erase({k, m});
    }

    bool trusted_{false};
    bool rightMouseHeld_{false};
    std::mutex mutex_;
    std::set<std::pair<KeyCode, Modifier>> held_;
};

}  // namespace

std::unique_ptr<IInputProvider> createMacInputProvider() {
    return std::make_unique<MacInputProvider>();
}

}  // namespace inputflow
