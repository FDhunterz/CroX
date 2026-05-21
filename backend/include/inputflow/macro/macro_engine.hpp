#pragma once

#include <memory>
#include <string>
#include <future>
#include "inputflow/types.hpp"
#include "inputflow/core/execution_state.hpp"
#include "inputflow/core/i_event_bus.hpp"
#include "inputflow/input/i_input_provider.hpp"
#include "inputflow/macro/macro_task_queue.hpp"

namespace inputflow {

/// Executes macro sequences asynchronously with cancellation support.
class MacroEngine {
public:
    MacroEngine(IEventBus& bus,
                ExecutionStateManager& state,
                IInputProvider& input,
                MacroTaskQueue& queue);

    std::future<void> runMacro(const Macro& macro);
    void cancelActive();
    void shutdown();

private:
    void executeSteps(const Macro& macro);
    bool executeStep(const MacroStep& step);
    bool waitWithCancel(uint32_t delayMs);

    IEventBus& bus_;
    ExecutionStateManager& state_;
    IInputProvider& input_;
    MacroTaskQueue& queue_;
    std::atomic<bool> shutdown_{false};
};

}  // namespace inputflow
