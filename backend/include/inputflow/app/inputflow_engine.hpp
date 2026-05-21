#pragma once

#include <memory>
#include <string>
#include "inputflow/types.hpp"
#include "inputflow/core/i_event_bus.hpp"
#include "inputflow/core/execution_state.hpp"
#include "inputflow/config/config_manager.hpp"
#include "inputflow/macro/macro_engine.hpp"
#include "inputflow/macro/macro_task_queue.hpp"
#include "inputflow/remap/key_remapper.hpp"
#include "inputflow/keyboard/keyboard_hook_service.hpp"
#include "inputflow/input/i_input_provider.hpp"
#include "inputflow/core/debounce_filter.hpp"

namespace inputflow {

/// Main application orchestrator — wires hooks, remap, macros, and config.
class InputFlowEngine {
public:
    explicit InputFlowEngine(std::string profilesDirectory);
    ~InputFlowEngine();

    InputFlowEngine(const InputFlowEngine&) = delete;
    InputFlowEngine& operator=(const InputFlowEngine&) = delete;

    bool initialize();
    void shutdown();

    bool start();
    bool stop();

    EngineState state() const;
    bool loadProfile(const std::string& profileId);
    bool saveActiveProfile();
    std::vector<std::string> listProfiles() const;

    void triggerMacro(const std::string& macroId);
    void cancelMacro();

    ConfigManager& config();
    IEventBus& eventBus();
    ExecutionStateManager& executionState();

private:
    void onKeyEvent(const KeyEvent& event);
    void handleHotkeys(const KeyEvent& event);
    void emitLog(LogLevel level, const std::string& message);
    void publishState();

    EventBus bus_;
    ExecutionStateManager state_;
    ConfigManager config_;
    MacroTaskQueue taskQueue_;
    std::unique_ptr<IInputProvider> input_;
    std::unique_ptr<MacroEngine> macroEngine_;
    std::unique_ptr<KeyRemapper> remapper_;
    std::unique_ptr<KeyboardHookService> hookService_;
    DebounceFilter debounce_;
};

}  // namespace inputflow
