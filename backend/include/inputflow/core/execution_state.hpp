#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include "inputflow/types.hpp"

namespace inputflow {

/// Thread-safe execution state manager for engine and macro runs.
class ExecutionStateManager {
public:
    EngineState engineState() const;
    void setEngineState(EngineState state);

    bool isMacroRunning() const;
    void setMacroRunning(bool running, const std::string& macroId = "");

    std::string activeMacroId() const;
    bool requestCancel();
    bool cancelRequested() const;
    void clearCancel();

private:
    mutable std::mutex mutex_;
    std::atomic<EngineState> engineState_{EngineState::Stopped};
    std::atomic<bool> macroRunning_{false};
    std::atomic<bool> cancelRequested_{false};
    std::string activeMacroId_;
};

}  // namespace inputflow
