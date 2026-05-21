#include "inputflow/keyboard/i_keyboard_hook.hpp"
#include "inputflow/types.hpp"
#include <Windows.h>
#include <memory>
#include <atomic>

namespace inputflow {
namespace {

std::atomic<HHOOK> g_hook{nullptr};
KeyboardCallback g_callback;
HWND g_messageHwnd{nullptr};

Modifier modifiersFromWin(DWORD flags) {
    Modifier m = Modifier::None;
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) m = m | Modifier::Ctrl;
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) m = m | Modifier::Shift;
    if (GetAsyncKeyState(VK_MENU) & 0x8000) m = m | Modifier::Alt;
    if ((GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000))
        m = m | Modifier::Meta;
    return m;
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && g_callback) {
        const auto* kbd = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        KeyEvent ev;
        ev.key = static_cast<KeyCode>(kbd->vkCode);
        ev.modifiers = modifiersFromWin(kbd->flags);
        ev.timestamp = std::chrono::steady_clock::now();

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) ev.action = KeyAction::Down;
        else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) ev.action = KeyAction::Up;
        else return CallNextHookEx(g_hook.load(), nCode, wParam, lParam);

        g_callback(ev);
    }
    return CallNextHookEx(g_hook.load(), nCode, wParam, lParam);
}

DWORD WINAPI MessagePumpThread(LPVOID) {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

class WinKeyboardHook final : public IKeyboardHook {
public:
    bool install(KeyboardCallback callback) override {
        if (g_hook.load()) return true;
        g_callback = std::move(callback);

        g_hook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc,
                                   GetModuleHandleW(nullptr), 0);
        if (!g_hook) return false;

        CreateThread(nullptr, 0, MessagePumpThread, nullptr, 0, nullptr);
        return true;
    }

    void uninstall() override {
        if (g_hook) {
            UnhookWindowsHookEx(g_hook.exchange(nullptr));
        }
        g_callback = nullptr;
    }

    bool isInstalled() const override { return g_hook.load() != nullptr; }
};

}  // namespace

std::unique_ptr<IKeyboardHook> createPlatformKeyboardHook() {
    return std::make_unique<WinKeyboardHook>();
}

}  // namespace inputflow
