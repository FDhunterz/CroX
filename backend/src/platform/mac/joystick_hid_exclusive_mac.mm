#include "inputflow/joystick/i_joystick_provider.hpp"
#import <Foundation/Foundation.h>
#import <IOKit/hid/IOHIDManager.h>
#import <IOKit/hid/IOHIDKeys.h>
#import <IOKit/hid/IOHIDUsageTables.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <future>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

namespace inputflow {
namespace {

size_t btnIndex(JoystickButton b) {
    return static_cast<size_t>(b);
}

bool elementPressed(IOHIDElementRef elem, IOHIDValueRef value) {
    (void)elem;
    const CFIndex v = IOHIDValueGetIntegerValue(value);
    if (v != 0) return true;
    const double scaled = IOHIDValueGetScaledValue(value, kIOHIDValueScaleTypePhysical);
    return scaled > 0.55;
}

float axisNormalized(IOHIDElementRef elem, IOHIDValueRef value) {
    const CFIndex logical = IOHIDValueGetIntegerValue(value);
    const CFIndex min = IOHIDElementGetLogicalMin(elem);
    const CFIndex max = IOHIDElementGetLogicalMax(elem);
    if (max <= min) {
        const double scaled = IOHIDValueGetScaledValue(value, kIOHIDValueScaleTypePhysical);
        return static_cast<float>(std::clamp(scaled, -1.0, 1.0));
    }
    const double mid = (static_cast<double>(min) + static_cast<double>(max)) * 0.5;
    const double half = (static_cast<double>(max) - static_cast<double>(min)) * 0.5;
    if (half < 1e-6) return 0.f;
    return static_cast<float>(std::clamp((static_cast<double>(logical) - mid) / half, -1.0, 1.0));
}

uint32_t devicePropertyU32(IOHIDDeviceRef device, CFStringRef key) {
    if (!device) return 0;
    CFTypeRef prop = IOHIDDeviceGetProperty(device, key);
    if (!prop || CFGetTypeID(prop) != CFNumberGetTypeID()) return 0;
    int32_t v = 0;
    CFNumberGetValue(static_cast<CFNumberRef>(prop), kCFNumberSInt32Type, &v);
    return static_cast<uint32_t>(v);
}

uint32_t deviceVendorId(IOHIDDeviceRef device) {
    return devicePropertyU32(device, CFSTR(kIOHIDVendorIDKey));
}

uint32_t deviceProductId(IOHIDDeviceRef device) {
    return devicePropertyU32(device, CFSTR(kIOHIDProductIDKey));
}

std::optional<JoystickButton> mapXboxStyleButton(uint32_t usage) {
    switch (usage) {
        case 1: return JoystickButton::A;
        case 2: return JoystickButton::B;
        case 3: return JoystickButton::X;
        case 4: return JoystickButton::Y;
        case 5: return JoystickButton::LB;
        case 6: return JoystickButton::RB;
        case 7: return JoystickButton::LT;
        case 8: return JoystickButton::RT;
        case 9: return JoystickButton::Back;
        case 10: return JoystickButton::Start;
        case 11: return JoystickButton::L3;
        case 12: return JoystickButton::R3;
        case 13: return JoystickButton::DpadUp;
        case 14: return JoystickButton::DpadDown;
        case 15: return JoystickButton::DpadLeft;
        case 16: return JoystickButton::DpadRight;
        default: return std::nullopt;
    }
}

std::optional<JoystickButton> mapPlayStationButton(uint32_t usage) {
    switch (usage) {
        case 1: return JoystickButton::X;
        case 2: return JoystickButton::A;
        case 3: return JoystickButton::B;
        case 4: return JoystickButton::Y;
        case 5: return JoystickButton::LB;
        case 6: return JoystickButton::RB;
        case 7: return JoystickButton::LT;
        case 8: return JoystickButton::RT;
        case 9: return JoystickButton::Back;
        case 10: return JoystickButton::Start;
        case 11: return JoystickButton::L3;
        case 12: return JoystickButton::R3;
        case 13: return JoystickButton::DpadUp;
        case 14: return JoystickButton::DpadDown;
        case 15: return JoystickButton::DpadLeft;
        case 16: return JoystickButton::DpadRight;
        default: return std::nullopt;
    }
}

std::optional<JoystickButton> mapHidButton(uint32_t vendorId, uint32_t usage) {
    if (vendorId == 0x054C) return mapPlayStationButton(usage);
    return mapXboxStyleButton(usage);
}

int countButtonElements(IOHIDDeviceRef device) {
    int n = 0;
    CFArrayRef elements = IOHIDDeviceCopyMatchingElements(device, 0, 0);
    if (!elements) return 0;
    for (CFIndex i = 0; i < CFArrayGetCount(elements); ++i) {
        auto* el = (IOHIDElementRef)CFArrayGetValueAtIndex(elements, i);
        if (!el) continue;
        if (IOHIDElementGetUsagePage(el) == kHIDPage_Button ||
            IOHIDElementGetType(el) == kIOHIDElementTypeInput_Button) {
            ++n;
        }
    }
    CFRelease(elements);
    return n;
}

bool isControllerCandidate(IOHIDDeviceRef device) {
    if (!device) return false;
    if (countButtonElements(device) >= 4) return true;
    const uint32_t page = devicePropertyU32(device, CFSTR(kIOHIDPrimaryUsagePageKey));
    const uint32_t usage = devicePropertyU32(device, CFSTR(kIOHIDPrimaryUsageKey));
    return page == kHIDPage_GenericDesktop &&
           (usage == kHIDUsage_GD_GamePad || usage == kHIDUsage_GD_Joystick ||
            usage == kHIDUsage_GD_MultiAxisController);
}

}  // namespace

class MacHidExclusiveJoystick final : public IJoystickProvider {
public:
    std::string name() const override { return "hid_exclusive"; }
    std::string captureMode() const override { return "exclusive"; }

    bool initialize() override {
        if (running_.load()) return true;

        if (loopThread_.joinable()) {
            quickShutdown();
        }

        running_ = true;
        connected_ = false;
        loopThread_ = std::thread([this] { runLoopThread(); });
        // Jangan blok thread pemanggil (Electron main) — koneksi HID async.
        return loopThread_.joinable();
    }

    void shutdown() override {
        running_ = false;
        requestRunLoopTeardown();
        joinLoopThreadWithTimeout(350);
        resetState();
    }

    void quickShutdown() {
        running_ = false;
        requestRunLoopTeardown();
        joinLoopThreadWithTimeout(200);
        resetState();
    }

    bool isConnected() const override {
        return connected_.load();
    }

    bool poll(JoystickState& out) override {
        if (!connected_.load()) return false;
        std::lock_guard lock(stateMutex_);
        out = state_;
        return true;
    }

private:
    bool sameController(IOHIDDeviceRef device) const {
        if (!device) return false;
        if (anchorVid_ == 0 && anchorPid_ == 0) return true;
        return deviceVendorId(device) == anchorVid_ && deviceProductId(device) == anchorPid_;
    }

    bool isSeized(IOHIDDeviceRef device) const {
        return std::find(seizedDevices_.begin(), seizedDevices_.end(), device) != seizedDevices_.end();
    }

    void runLoopThread() {
        @autoreleasepool {
            IOHIDManagerRef mgr = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
            if (!mgr) {
                running_ = false;
                return;
            }

            // Lebar: semua gamepad/joystick + vendor umum (DS4/Xbox).
            NSArray* matches = @[
                @{
                    @kIOHIDDeviceUsagePageKey: @((NSUInteger)kHIDPage_GenericDesktop),
                    @kIOHIDDeviceUsageKey: @((NSUInteger)kHIDUsage_GD_GamePad),
                },
                @{
                    @kIOHIDDeviceUsagePageKey: @((NSUInteger)kHIDPage_GenericDesktop),
                    @kIOHIDDeviceUsageKey: @((NSUInteger)kHIDUsage_GD_Joystick),
                },
                @{
                    @kIOHIDDeviceUsagePageKey: @((NSUInteger)kHIDPage_GenericDesktop),
                    @kIOHIDDeviceUsageKey: @((NSUInteger)kHIDUsage_GD_MultiAxisController),
                },
                @{ @kIOHIDVendorIDKey: @((NSUInteger)0x054C) },
                @{ @kIOHIDVendorIDKey: @((NSUInteger)0x045E) },
            ];
            IOHIDManagerSetDeviceMatchingMultiple(mgr, (__bridge CFArrayRef)matches);
            IOHIDManagerRegisterDeviceMatchingCallback(mgr, deviceMatched, this);
            IOHIDManagerRegisterDeviceRemovalCallback(mgr, deviceRemoved, this);
            IOHIDManagerScheduleWithRunLoop(mgr, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

            IOReturn openResult = IOHIDManagerOpen(mgr, kIOHIDOptionsTypeSeizeDevice);
            managerOpenedWithSeize_ = true;
            perDeviceSeize_ = false;
            if (openResult != kIOReturnSuccess) {
                openResult = IOHIDManagerOpen(mgr, kIOHIDOptionsTypeNone);
                managerOpenedWithSeize_ = false;
                perDeviceSeize_ = true;
            }
            if (openResult != kIOReturnSuccess) {
                IOHIDManagerClose(mgr, kIOHIDOptionsTypeNone);
                CFRelease(mgr);
                running_ = false;
                return;
            }

            manager_ = mgr;
            {
                std::lock_guard lock(runLoopMutex_);
                runLoop_ = CFRunLoopGetCurrent();
            }

            seizeAllMatchingDevices();

            CFRunLoopRun();

            // teardownOnRunLoopThread() sudah dipanggil dari shutdown(); ini cadangan.
            teardownOnRunLoopThread();
        }
    }

    void requestRunLoopTeardown() {
        CFRunLoopRef rl = nullptr;
        {
            std::lock_guard lock(runLoopMutex_);
            rl = runLoop_;
        }
        if (!rl) return;

        MacHidExclusiveJoystick* self = this;
        CFRunLoopPerformBlock(rl, kCFRunLoopDefaultMode, ^{
            self->teardownOnRunLoopThread();
        });
        CFRunLoopWakeUp(rl);
    }

    void teardownOnRunLoopThread() {
        if (!manager_ && seizedDevices_.empty()) {
            connected_ = false;
            CFRunLoopStop(CFRunLoopGetCurrent());
            return;
        }
        closeAllDevices();
        if (manager_) {
            IOHIDManagerRegisterDeviceMatchingCallback(manager_, nullptr, nullptr);
            IOHIDManagerRegisterDeviceRemovalCallback(manager_, nullptr, nullptr);
            if (runLoop_) {
                IOHIDManagerUnscheduleFromRunLoop(manager_, runLoop_, kCFRunLoopDefaultMode);
            }
            const IOHIDOptionsType closeMode =
                managerOpenedWithSeize_ ? kIOHIDOptionsTypeSeizeDevice : kIOHIDOptionsTypeNone;
            IOHIDManagerClose(manager_, closeMode);
            CFRelease(manager_);
            manager_ = nullptr;
        }
        {
            std::lock_guard lock(runLoopMutex_);
            runLoop_ = nullptr;
        }
        connected_ = false;
        CFRunLoopStop(CFRunLoopGetCurrent());
    }

    void joinLoopThreadWithTimeout(int ms) {
        if (!loopThread_.joinable()) return;
        auto fut = std::async(std::launch::async, [this]() {
            if (loopThread_.joinable()) loopThread_.join();
        });
        if (fut.wait_for(std::chrono::milliseconds(ms)) != std::future_status::ready) {
            emergencyReleaseHid();
            if (loopThread_.joinable()) loopThread_.detach();
        }
    }

    void emergencyReleaseHid() {
        closeAllDevices();
        if (manager_) {
            IOHIDManagerClose(manager_, kIOHIDOptionsTypeSeizeDevice);
            IOHIDManagerClose(manager_, kIOHIDOptionsTypeNone);
            CFRelease(manager_);
            manager_ = nullptr;
        }
        {
            std::lock_guard lock(runLoopMutex_);
            runLoop_ = nullptr;
        }
        connected_ = false;
    }

    void resetState() {
        std::lock_guard lock(stateMutex_);
        state_.pressed.reset();
        state_.lx = state_.ly = state_.rx = state_.ry = 0.f;
        cookieToButton_.clear();
        anchorVid_ = anchorPid_ = 0;
        vendorId_ = 0;
    }

    void seizeAllMatchingDevices() {
        if (!manager_) return;
        CFSetRef current = IOHIDManagerCopyDevices(manager_);
        if (!current) return;

        const CFIndex n = CFSetGetCount(current);
        std::vector<IOHIDDeviceRef> devices(static_cast<size_t>(n), nullptr);
        CFSetGetValues(current, (const void**)devices.data());

        IOHIDDeviceRef anchor = nullptr;
        int bestButtons = -1;
        for (IOHIDDeviceRef dev : devices) {
            if (!isControllerCandidate(dev)) continue;
            const int btn = countButtonElements(dev);
            if (btn > bestButtons) {
                bestButtons = btn;
                anchor = dev;
            }
        }

        if (anchor) {
            anchorVid_ = deviceVendorId(anchor);
            anchorPid_ = deviceProductId(anchor);
        }

        for (IOHIDDeviceRef dev : devices) {
            if (!isControllerCandidate(dev)) continue;
            if (anchorVid_ != 0 && !sameController(dev)) continue;
            tryAttachDevice(dev);
        }

        CFRelease(current);
        connected_ = !seizedDevices_.empty();
    }

    static void deviceMatched(void* context, IOReturn, void* sender, IOHIDDeviceRef device) {
        (void)sender;
        static_cast<MacHidExclusiveJoystick*>(context)->onDeviceMatched(device);
    }

    static void deviceRemoved(void* context, IOReturn, void* sender, IOHIDDeviceRef device) {
        (void)sender;
        static_cast<MacHidExclusiveJoystick*>(context)->onDeviceRemoved(device);
    }

    void onDeviceMatched(IOHIDDeviceRef device) {
        if (!running_.load() || !device || isSeized(device)) return;
        if (!isControllerCandidate(device)) return;

        if (seizedDevices_.empty()) {
            anchorVid_ = deviceVendorId(device);
            anchorPid_ = deviceProductId(device);
        } else if (!sameController(device)) {
            return;
        }

        tryAttachDevice(device);
        connected_ = !seizedDevices_.empty();
    }

    void onDeviceRemoved(IOHIDDeviceRef device) {
        if (!device) return;
        auto it = std::find(seizedDevices_.begin(), seizedDevices_.end(), device);
        if (it == seizedDevices_.end()) return;
        detachDevice(*it);
        seizedDevices_.erase(it);
        connected_ = !seizedDevices_.empty();
        if (!connected_) {
            std::lock_guard lock(stateMutex_);
            state_.pressed.reset();
            state_.lx = state_.ly = state_.rx = state_.ry = 0.f;
            cookieToButton_.clear();
            anchorVid_ = anchorPid_ = 0;
        }
    }

    void tryAttachDevice(IOHIDDeviceRef device) {
        if (!device || !runLoop_ || isSeized(device)) return;

        if (perDeviceSeize_) {
            const IOReturn openResult = IOHIDDeviceOpen(device, kIOHIDOptionsTypeSeizeDevice);
            if (openResult != kIOReturnSuccess) return;
        }

        IOHIDDeviceRegisterInputValueCallback(device, hidInput, this);
        IOHIDDeviceScheduleWithRunLoop(device, runLoop_, kCFRunLoopDefaultMode);
        CFRetain(device);
        seizedDevices_.push_back(device);

        if (vendorId_ == 0) vendorId_ = deviceVendorId(device);
        mergeElementMap(device);
    }

    void detachDevice(IOHIDDeviceRef device) {
        if (!device || !runLoop_) return;
        IOHIDDeviceRegisterInputValueCallback(device, nullptr, nullptr);
        IOHIDDeviceUnscheduleFromRunLoop(device, runLoop_, kCFRunLoopDefaultMode);
        if (perDeviceSeize_) {
            IOHIDDeviceClose(device, kIOHIDOptionsTypeSeizeDevice);
        }
        CFRelease(device);
        pruneCookieMapForDevice(device);
    }

    void closeAllDevices() {
        for (IOHIDDeviceRef dev : seizedDevices_) {
            detachDevice(dev);
        }
        seizedDevices_.clear();
    }

    void mergeElementMap(IOHIDDeviceRef device) {
        const uint32_t vid = deviceVendorId(device);
        CFArrayRef elements = IOHIDDeviceCopyMatchingElements(device, 0, 0);
        if (!elements) return;

        std::vector<std::pair<uint32_t, IOHIDElementCookie>> unmapped;

        for (CFIndex i = 0; i < CFArrayGetCount(elements); ++i) {
            auto* el = (IOHIDElementRef)CFArrayGetValueAtIndex(elements, i);
            if (!el) continue;

            const uint32_t page = IOHIDElementGetUsagePage(el);
            const uint32_t usage = IOHIDElementGetUsage(el);
            const IOHIDElementCookie cookie = IOHIDElementGetCookie(el);

            if (page != kHIDPage_Button &&
                IOHIDElementGetType(el) != kIOHIDElementTypeInput_Button) {
                continue;
            }
            if (cookieToButton_.count(cookie)) continue;

            if (auto btn = mapHidButton(vid, usage)) {
                cookieToButton_[cookie] = *btn;
            } else {
                unmapped.emplace_back(usage, cookie);
            }
        }
        CFRelease(elements);

        if (unmapped.empty()) return;
        std::sort(unmapped.begin(), unmapped.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });

        static const JoystickButton kPsOrder[] = {
            JoystickButton::X, JoystickButton::A, JoystickButton::B, JoystickButton::Y,
            JoystickButton::LB, JoystickButton::RB, JoystickButton::LT, JoystickButton::RT,
            JoystickButton::Back, JoystickButton::Start, JoystickButton::L3, JoystickButton::R3,
            JoystickButton::DpadUp, JoystickButton::DpadDown, JoystickButton::DpadLeft,
            JoystickButton::DpadRight,
        };
        static const JoystickButton kXboxOrder[] = {
            JoystickButton::A, JoystickButton::B, JoystickButton::X, JoystickButton::Y,
            JoystickButton::LB, JoystickButton::RB, JoystickButton::LT, JoystickButton::RT,
            JoystickButton::Back, JoystickButton::Start, JoystickButton::L3, JoystickButton::R3,
            JoystickButton::DpadUp, JoystickButton::DpadDown, JoystickButton::DpadLeft,
            JoystickButton::DpadRight,
        };
        const JoystickButton* order = (vid == 0x054C) ? kPsOrder : kXboxOrder;
        const size_t orderLen = (vid == 0x054C) ? sizeof(kPsOrder) / sizeof(kPsOrder[0])
                                                : sizeof(kXboxOrder) / sizeof(kXboxOrder[0]);

        for (size_t i = 0; i < unmapped.size() && i < orderLen; ++i) {
            cookieToButton_[unmapped[i].second] = order[i];
        }
    }

    void pruneCookieMapForDevice(IOHIDDeviceRef /*device*/) {
        cookieToButton_.clear();
        for (IOHIDDeviceRef dev : seizedDevices_) {
            mergeElementMap(dev);
        }
    }

    static void hidInput(void* context, IOReturn, void*, IOHIDValueRef value) {
        static_cast<MacHidExclusiveJoystick*>(context)->handleValue(value);
    }

    void handleValue(IOHIDValueRef value) {
        const IOHIDElementRef elem = IOHIDValueGetElement(value);
        if (!elem) return;

        const uint32_t page = IOHIDElementGetUsagePage(elem);
        const uint32_t usage = IOHIDElementGetUsage(elem);
        const IOHIDElementCookie cookie = IOHIDElementGetCookie(elem);

        if (page == kHIDPage_Button ||
            IOHIDElementGetType(elem) == kIOHIDElementTypeInput_Button) {
            JoystickButton btn = JoystickButton::A;
            const auto it = cookieToButton_.find(cookie);
            if (it != cookieToButton_.end()) {
                btn = it->second;
            } else if (auto mapped = mapHidButton(vendorId_, usage)) {
                btn = *mapped;
            } else {
                return;
            }

            std::lock_guard lock(stateMutex_);
            if (elementPressed(elem, value)) state_.pressed.set(btnIndex(btn));
            else state_.pressed.reset(btnIndex(btn));
            return;
        }

        if (page == kHIDPage_GenericDesktop) {
            const bool pressed = elementPressed(elem, value);
            std::lock_guard lock(stateMutex_);

            if (usage == kHIDUsage_GD_Hatswitch) {
                state_.pressed.reset(btnIndex(JoystickButton::DpadUp));
                state_.pressed.reset(btnIndex(JoystickButton::DpadDown));
                state_.pressed.reset(btnIndex(JoystickButton::DpadLeft));
                state_.pressed.reset(btnIndex(JoystickButton::DpadRight));
                const CFIndex hv = IOHIDValueGetIntegerValue(value);
                if (pressed || hv != 0) {
                    if (hv == 0 || hv == 1) state_.pressed.set(btnIndex(JoystickButton::DpadUp));
                    if (hv == 2 || hv == 3) state_.pressed.set(btnIndex(JoystickButton::DpadRight));
                    if (hv == 4 || hv == 5) state_.pressed.set(btnIndex(JoystickButton::DpadDown));
                    if (hv == 6 || hv == 7) state_.pressed.set(btnIndex(JoystickButton::DpadLeft));
                }
                return;
            }

            if (usage == kHIDUsage_Sim_Brake) {
                if (elementPressed(elem, value)) state_.pressed.set(btnIndex(JoystickButton::LT));
                else state_.pressed.reset(btnIndex(JoystickButton::LT));
                return;
            }
            if (usage == kHIDUsage_Sim_Accelerator) {
                if (elementPressed(elem, value)) state_.pressed.set(btnIndex(JoystickButton::RT));
                else state_.pressed.reset(btnIndex(JoystickButton::RT));
                return;
            }

            const float axis = axisNormalized(elem, value);
            switch (usage) {
                case kHIDUsage_GD_X:
                    state_.lx = axis;
                    break;
                case kHIDUsage_GD_Y:
                    state_.ly = axis;
                    break;
                case kHIDUsage_GD_Z:
                    state_.rx = axis;
                    break;
                case kHIDUsage_GD_Rz:
                    state_.ry = axis;
                    break;
                case kHIDUsage_GD_Rx:
                    state_.rx = axis;
                    break;
                case kHIDUsage_GD_Ry:
                    state_.ry = axis;
                    break;
                default:
                    break;
            }
        }
    }

    IOHIDManagerRef manager_{nullptr};
    std::vector<IOHIDDeviceRef> seizedDevices_;
    CFRunLoopRef runLoop_{nullptr};
    std::thread loopThread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};
    uint32_t anchorVid_{0};
    uint32_t anchorPid_{0};
    uint32_t vendorId_{0};
    std::mutex stateMutex_;
    JoystickState state_;
    std::unordered_map<IOHIDElementCookie, JoystickButton> cookieToButton_;
    bool perDeviceSeize_{false};
    bool managerOpenedWithSeize_{false};
    std::mutex runLoopMutex_;
};

std::unique_ptr<IJoystickProvider> createMacHidExclusiveJoystickProvider() {
    return std::make_unique<MacHidExclusiveJoystick>();
}

}  // namespace inputflow
