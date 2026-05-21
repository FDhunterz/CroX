#include "inputflow/core/execution_state.hpp"

namespace inputflow {

EngineState ExecutionStateManager::engineState() const {
    return engineState_.load();
}

void ExecutionStateManager::setEngineState(EngineState state) {
    engineState_.store(state);
}

bool ExecutionStateManager::isMacroRunning() const {
    return macroRunning_.load();
}

void ExecutionStateManager::setMacroRunning(bool running, const std::string& macroId) {
    std::lock_guard lock(mutex_);
    macroRunning_.store(running);
    if (running) {
        activeMacroId_ = macroId;
        cancelRequested_.store(false);
    } else if (macroId.empty()) {
        activeMacroId_.clear();
    }
}

std::string ExecutionStateManager::activeMacroId() const {
    std::lock_guard lock(mutex_);
    return activeMacroId_;
}

bool ExecutionStateManager::requestCancel() {
    if (!macroRunning_.load()) return false;
    cancelRequested_.store(true);
    return true;
}

bool ExecutionStateManager::cancelRequested() const {
    return cancelRequested_.load();
}

void ExecutionStateManager::clearCancel() {
    cancelRequested_.store(false);
}

}  // namespace inputflow
