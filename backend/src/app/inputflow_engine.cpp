#include "inputflow/app/inputflow_engine.hpp"
#include "inputflow/config/json_profile_parser.hpp"
#include <filesystem>

namespace fs = std::filesystem;

namespace inputflow {

InputFlowEngine::InputFlowEngine(std::string profilesDirectory)
    : config_(std::move(profilesDirectory)) {}

InputFlowEngine::~InputFlowEngine() {
    shutdown();
}

bool InputFlowEngine::initialize() {
    input_ = createPlatformInputProvider();
    if (!input_->initialize()) {
        emitLog(LogLevel::Error, "Failed to initialize input provider: " + input_->name());
        return false;
    }

    taskQueue_.start();
    macroEngine_ = std::make_unique<MacroEngine>(bus_, state_, *input_, taskQueue_);
    remapper_ = std::make_unique<KeyRemapper>(*input_);
    hookService_ = std::make_unique<KeyboardHookService>(bus_);

    bus_.subscribe(TOPIC_KEY_EVENT, [this](const void* payload) {
        if (payload) onKeyEvent(*static_cast<const KeyEvent*>(payload));
    });

    const auto defaultPath = fs::path(config_.profilesPath()).parent_path() / ".." / "configs" / "default.json";
    if (fs::exists(defaultPath)) {
        try {
            config_.setActiveProfile(JsonProfileParser::parseFromFile(defaultPath.string()));
        } catch (...) {
            emitLog(LogLevel::Warn, "Could not load bundled default profile");
        }
    }

    remapper_->setBindings(config_.activeProfile().remaps);
    debounce_.setDebounceMs(config_.activeProfile().debounceMs);

    emitLog(LogLevel::Info, "InputFlow engine initialized");
    return true;
}

void InputFlowEngine::shutdown() {
    stop();
    if (macroEngine_) macroEngine_->shutdown();
    taskQueue_.stop();
    if (hookService_) hookService_->stop();
    if (input_) input_->shutdown();
    state_.setEngineState(EngineState::Stopped);
}

bool InputFlowEngine::start() {
    if (state_.engineState() == EngineState::Running) return true;
    state_.setEngineState(EngineState::Starting);
    publishState();

    if (!hookService_->start()) {
        emitLog(LogLevel::Error, "Failed to install global keyboard hook");
        state_.setEngineState(EngineState::Error);
        publishState();
        return false;
    }

    state_.setEngineState(EngineState::Running);
    emitLog(LogLevel::Info, "Engine started");
    publishState();
    return true;
}

bool InputFlowEngine::stop() {
    if (state_.engineState() == EngineState::Stopped) return true;
    state_.setEngineState(EngineState::Stopping);
    publishState();

    cancelMacro();
    if (hookService_) hookService_->stop();
    if (input_) input_->releaseAllHeld();

    state_.setEngineState(EngineState::Stopped);
    emitLog(LogLevel::Info, "Engine stopped");
    publishState();
    return true;
}

EngineState InputFlowEngine::state() const {
    return state_.engineState();
}

bool InputFlowEngine::loadProfile(const std::string& profileId) {
    if (!config_.loadProfile(profileId)) return false;
    remapper_->setBindings(config_.activeProfile().remaps);
    debounce_.setDebounceMs(config_.activeProfile().debounceMs);
    emitLog(LogLevel::Info, "Loaded profile: " + profileId);
    return true;
}

bool InputFlowEngine::saveActiveProfile() {
    return config_.saveProfile(config_.activeProfile());
}

std::vector<std::string> InputFlowEngine::listProfiles() const {
    return config_.listProfiles();
}

void InputFlowEngine::triggerMacro(const std::string& macroId) {
    if (state_.isMacroRunning()) {
        emitLog(LogLevel::Warn, "Macro already running");
        return;
    }
    for (const auto& m : config_.activeProfile().macros) {
        if (m.id == macroId && m.enabled) {
            macroEngine_->runMacro(m);
            emitLog(LogLevel::Info, "Triggered macro: " + macroId);
            return;
        }
    }
    emitLog(LogLevel::Warn, "Macro not found: " + macroId);
}

void InputFlowEngine::cancelMacro() {
    if (macroEngine_) macroEngine_->cancelActive();
}

ConfigManager& InputFlowEngine::config() { return config_; }
IEventBus& InputFlowEngine::eventBus() { return bus_; }
ExecutionStateManager& InputFlowEngine::executionState() { return state_; }

void InputFlowEngine::onKeyEvent(const KeyEvent& event) {
    if (state_.engineState() != EngineState::Running) return;
    if (!debounce_.shouldProcess(event)) return;

    handleHotkeys(event);

    if (remapper_->tryRemap(event)) return;

    // Future: route to macro triggers bound to keys
}

void InputFlowEngine::handleHotkeys(const KeyEvent& event) {
    if (event.action != KeyAction::Down) return;

    const auto& profile = config_.activeProfile();
    const auto match = [&](const HotkeyBinding& h) {
        return h.key == event.key && h.modifiers == event.modifiers;
    };

    if (match(profile.startHotkey)) {
        if (profile.startHotkey.action == "start_engine") start();
        else if (profile.startHotkey.action == "toggle_engine") {
            if (state_.engineState() == EngineState::Running) stop();
            else start();
        }
    }
    if (match(profile.stopHotkey)) {
        if (profile.stopHotkey.action == "stop_engine") stop();
    }
}

void InputFlowEngine::emitLog(LogLevel level, const std::string& message) {
    LogEntry entry{level, message, std::chrono::system_clock::now()};
    bus_.publish(TOPIC_LOG, &entry);
}

void InputFlowEngine::publishState() {
    const auto s = state_.engineState();
    bus_.publish(TOPIC_STATE, &s);
}

}  // namespace inputflow
