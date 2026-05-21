#include "inputflow/joystick/i_joystick_provider.hpp"
#import <GameController/GameController.h>
#include <memory>

namespace inputflow {

namespace {

size_t buttonIndex(JoystickButton b) {
    return static_cast<size_t>(b);
}

GCController* pickBestController() {
    // Pilih controller pertama yang punya extendedGamepad (DS4 / Xbox)
    for (GCController* c in [GCController controllers]) {
        if (c.extendedGamepad) return c;
    }
    for (GCController* c in [GCController controllers]) {
        if (c.microGamepad) return c;
    }
    return nil;
}

}  // namespace

class MacGameControllerJoystick final : public IJoystickProvider {
public:
    std::string name() const override { return "gamecontroller"; }
    std::string captureMode() const override { return "shared"; }

    bool initialize() override {
        if (@available(macOS 10.15, *)) {
            GCController.shouldMonitorBackgroundEvents = YES;
        }

        __block bool ok = false;
        MacGameControllerJoystick* self = this;
        void (^wireBlock)(void) = ^{
            self->controller_ = pickBestController();
            if (!self->controller_) return;
            if (self->controller_.extendedGamepad) {
                self->wireExtendedPad(self->controller_.extendedGamepad);
                ok = true;
            } else if (@available(macOS 12.0, *)) {
                if (self->controller_.microGamepad) {
                    self->wireMicroPad(self->controller_.microGamepad);
                    ok = true;
                }
            }
        };
        if ([NSThread isMainThread]) {
            wireBlock();
        } else {
            dispatch_sync(dispatch_get_main_queue(), wireBlock);
        }

        if (!ok) return false;

        connectObserver_ = [[NSNotificationCenter defaultCenter]
            addObserverForName:GCControllerDidConnectNotification
                        object:nil
                         queue:[NSOperationQueue mainQueue]
                    usingBlock:^(__unused NSNotification* note) {
                        dispatch_async(dispatch_get_main_queue(), ^{
                            if (!controller_ || !controller_.extendedGamepad) {
                                controller_ = pickBestController();
                                if (controller_.extendedGamepad) {
                                    wireExtendedPad(controller_.extendedGamepad);
                                }
                            }
                        });
                    }];

        handlersWired_ = true;
        return true;
    }

    void shutdown() override {
        if (connectObserver_) {
            [[NSNotificationCenter defaultCenter] removeObserver:connectObserver_];
            connectObserver_ = nil;
        }
        MacGameControllerJoystick* self = this;
        void (^clearBlock)(void) = ^{
            self->clearHandlers();
            self->controller_ = nil;
            self->handlersWired_ = false;
        };
        if ([NSThread isMainThread]) {
            clearBlock();
        } else {
            dispatch_sync(dispatch_get_main_queue(), clearBlock);
        }
        handlersWired_ = false;
        std::lock_guard lock(mutex_);
        state_.pressed.reset();
    }

    bool isConnected() const override {
        return handlersWired_ && controller_ != nil;
    }

    bool poll(JoystickState& out) override {
        if (!handlersWired_) return false;
        std::lock_guard lock(mutex_);
        out = state_;
        MacGameControllerJoystick* self = this;
        void (^readAxes)(void) = ^{
            if (self->lastPad_) {
                out.lx = static_cast<float>(self->lastPad_.leftThumbstick.xAxis.value);
                out.ly = static_cast<float>(self->lastPad_.leftThumbstick.yAxis.value);
                out.rx = static_cast<float>(self->lastPad_.rightThumbstick.xAxis.value);
                out.ry = static_cast<float>(self->lastPad_.rightThumbstick.yAxis.value);
            }
        };
        if ([NSThread isMainThread]) {
            readAxes();
        } else {
            dispatch_sync(dispatch_get_main_queue(), readAxes);
        }
        return true;
    }

private:
    void setButton(JoystickButton btn, bool pressed) {
        std::lock_guard lock(mutex_);
        if (pressed) state_.pressed.set(buttonIndex(btn));
        else state_.pressed.reset(buttonIndex(btn));
    }

    void wireExtendedPad(GCExtendedGamepad* pad) {
        clearHandlers();
        lastPad_ = pad;

        MacGameControllerJoystick* selfPad = this;
        auto bind = ^(GCControllerButtonInput* input, JoystickButton btn) {
            input.pressedChangedHandler = ^(GCControllerButtonInput*, float, BOOL pressed) {
                selfPad->setButton(btn, pressed);
            };
        };

        bind(pad.buttonA, JoystickButton::A);
        bind(pad.buttonB, JoystickButton::B);
        bind(pad.buttonX, JoystickButton::X);
        bind(pad.buttonY, JoystickButton::Y);
        bind(pad.leftShoulder, JoystickButton::LB);
        bind(pad.rightShoulder, JoystickButton::RB);
        bind(pad.buttonMenu, JoystickButton::Start);
        bind(pad.buttonOptions, JoystickButton::Back);
        bind(pad.leftThumbstickButton, JoystickButton::L3);
        bind(pad.rightThumbstickButton, JoystickButton::R3);

        MacGameControllerJoystick* self = this;
        pad.leftTrigger.pressedChangedHandler = ^(GCControllerButtonInput*, float value, BOOL pressed) {
            self->setButton(JoystickButton::LT, pressed || value > 0.5);
        };
        pad.rightTrigger.pressedChangedHandler = ^(GCControllerButtonInput*, float value, BOOL pressed) {
            self->setButton(JoystickButton::RT, pressed || value > 0.5);
        };

        pad.dpad.up.pressedChangedHandler = ^(GCControllerButtonInput*, float, BOOL pressed) {
            self->setButton(JoystickButton::DpadUp, pressed);
        };
        pad.dpad.down.pressedChangedHandler = ^(GCControllerButtonInput*, float, BOOL pressed) {
            self->setButton(JoystickButton::DpadDown, pressed);
        };
        pad.dpad.left.pressedChangedHandler = ^(GCControllerButtonInput*, float, BOOL pressed) {
            self->setButton(JoystickButton::DpadLeft, pressed);
        };
        pad.dpad.right.pressedChangedHandler = ^(GCControllerButtonInput*, float, BOOL pressed) {
            self->setButton(JoystickButton::DpadRight, pressed);
        };
    }

    void wireMicroPad(GCMicroGamepad* pad) {
        clearHandlers();
        lastMicroPad_ = pad;
        MacGameControllerJoystick* self = this;
        pad.buttonA.pressedChangedHandler = ^(GCControllerButtonInput*, float, BOOL pressed) {
            self->setButton(JoystickButton::A, pressed);
        };
        pad.buttonX.pressedChangedHandler = ^(GCControllerButtonInput*, float, BOOL pressed) {
            self->setButton(JoystickButton::X, pressed);
        };
    }

    void clearHandlers() {
        if (lastPad_) {
            lastPad_.buttonA.pressedChangedHandler = nil;
            lastPad_.buttonB.pressedChangedHandler = nil;
            lastPad_.buttonX.pressedChangedHandler = nil;
            lastPad_.buttonY.pressedChangedHandler = nil;
            lastPad_.leftShoulder.pressedChangedHandler = nil;
            lastPad_.rightShoulder.pressedChangedHandler = nil;
            lastPad_.buttonMenu.pressedChangedHandler = nil;
            lastPad_.buttonOptions.pressedChangedHandler = nil;
            lastPad_.leftThumbstickButton.pressedChangedHandler = nil;
            lastPad_.rightThumbstickButton.pressedChangedHandler = nil;
            lastPad_.leftTrigger.pressedChangedHandler = nil;
            lastPad_.rightTrigger.pressedChangedHandler = nil;
            lastPad_.dpad.up.pressedChangedHandler = nil;
            lastPad_.dpad.down.pressedChangedHandler = nil;
            lastPad_.dpad.left.pressedChangedHandler = nil;
            lastPad_.dpad.right.pressedChangedHandler = nil;
            lastPad_ = nil;
        }
        if (lastMicroPad_) {
            lastMicroPad_.buttonA.pressedChangedHandler = nil;
            lastMicroPad_.buttonX.pressedChangedHandler = nil;
            lastMicroPad_ = nil;
        }
    }

    GCController* controller_{nil};
    GCExtendedGamepad* lastPad_{nil};
    GCMicroGamepad* lastMicroPad_{nil};
    id connectObserver_{nil};
    bool handlersWired_{false};
    std::mutex mutex_;
    JoystickState state_;
};

std::unique_ptr<IJoystickProvider> createMacGameControllerJoystickProvider() {
    return std::make_unique<MacGameControllerJoystick>();
}

std::unique_ptr<IJoystickProvider> createMacJoystickProvider(bool /*exclusiveCapture*/) {
    return createMacGameControllerJoystickProvider();
}

}  // namespace inputflow
