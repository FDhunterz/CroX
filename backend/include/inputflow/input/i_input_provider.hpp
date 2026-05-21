#pragma once

#include <memory>
#include <string>
#include "inputflow/types.hpp"

namespace inputflow {

/// Abstraction for keyboard / future HID / virtual input backends.
/// Implementations must use official OS APIs only.
class IInputProvider {
public:
    virtual ~IInputProvider() = default;

    virtual std::string name() const = 0;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    /// Perbarui status izin Accessibility (macOS). No-op di platform lain.
    virtual void refreshAccessibility() {}

    /// Apakah keyboard sintetis diizinkan oleh OS (macOS Accessibility).
    virtual bool accessibilityGranted() const { return true; }

    virtual bool sendKeyDown(KeyCode key, Modifier mods) = 0;
    virtual bool sendKeyUp(KeyCode key, Modifier mods) = 0;
    virtual bool sendKeyPress(KeyCode key, Modifier mods) = 0;

    /// Hanya modifier (LB=Alt+Shift) tanpa tombol utama.
    virtual bool sendModifiersDown(Modifier mods) { return false; }
    virtual bool sendModifiersUp(Modifier mods) { return false; }

    /// Release all keys tracked as held — prevents stuck keys on cancel/error.
    virtual void releaseAllHeld() = 0;

    virtual bool moveMouseRelative(int dx, int dy) { (void)dx; (void)dy; return false; }
    virtual bool rightMouseDown() { return false; }
    virtual bool rightMouseUp() { return false; }
    virtual void releaseCameraMouse() {}
};

std::unique_ptr<IInputProvider> createPlatformInputProvider();

}  // namespace inputflow
