#include "inputflow/macro/macro_engine.hpp"
#include "inputflow/core/key_release_guard.hpp"
#include <thread>

namespace inputflow {

MacroEngine::MacroEngine(IEventBus& bus,
                         ExecutionStateManager& state,
                         IInputProvider& input,
                         MacroTaskQueue& queue)
    : bus_(bus), state_(state), input_(input), queue_(queue) {}

std::future<void> MacroEngine::runMacro(const Macro& macro) {
    return std::async(std::launch::async, [this, macro] {
        queue_.enqueue([this, macro] { executeSteps(macro); });
    });
}

void MacroEngine::cancelActive() {
    state_.requestCancel();
    input_.releaseAllHeld();
}

void MacroEngine::shutdown() {
    shutdown_ = true;
    cancelActive();
}

void MacroEngine::executeSteps(const Macro& macro) {
    if (!macro.enabled || macro.steps.empty()) return;

    state_.setMacroRunning(true, macro.id);
    bus_.publish(TOPIC_MACRO_STARTED, &macro.id);

    KeyReleaseGuard guard(input_);

    for (const auto& step : macro.steps) {
        if (shutdown_ || state_.cancelRequested()) break;
        if (!executeStep(step)) break;
    }

    guard.releaseAll();
    state_.setMacroRunning(false);
    state_.clearCancel();

    if (state_.cancelRequested()) {
        bus_.publish(TOPIC_MACRO_CANCELLED, &macro.id);
    } else {
        bus_.publish(TOPIC_MACRO_FINISHED, &macro.id);
    }
}

bool MacroEngine::executeStep(const MacroStep& step) {
    switch (step.type) {
        case MacroStepType::KeyPress:
            input_.sendKeyPress(step.key, step.modifiers);
            return true;
        case MacroStepType::KeyDown:
            input_.sendKeyDown(step.key, step.modifiers);
            return true;
        case MacroStepType::KeyUp:
            input_.sendKeyUp(step.key, step.modifiers);
            return true;
        case MacroStepType::Delay:
            return waitWithCancel(step.delayMs);
        case MacroStepType::Combo:
            for (KeyCode k : step.comboKeys) {
                if (state_.cancelRequested()) return false;
                input_.sendKeyDown(k, step.modifiers);
            }
            for (auto it = step.comboKeys.rbegin(); it != step.comboKeys.rend(); ++it) {
                input_.sendKeyUp(*it, step.modifiers);
            }
            return true;
        default:
            return false;
    }
}

bool MacroEngine::waitWithCancel(uint32_t delayMs) {
    const auto step = std::chrono::milliseconds(10);
    auto remaining = std::chrono::milliseconds(delayMs);
    while (remaining.count() > 0) {
        if (state_.cancelRequested() || shutdown_) return false;
        const auto slice = remaining > step ? step : remaining;
        std::this_thread::sleep_for(slice);
        remaining -= slice;
    }
    return true;
}

}  // namespace inputflow
