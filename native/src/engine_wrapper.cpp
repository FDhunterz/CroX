#include "engine_wrapper.hpp"
#include "inputflow/config/json_mapping_parser.hpp"
#include "inputflow/config/mapping_validator.hpp"
#include <nlohmann/json.hpp>

namespace inputflow::napi_bridge {

EngineWrapper::EngineWrapper(std::string configDirectory)
    : app_(std::make_unique<InputFlowApp>(std::move(configDirectory))) {}

bool EngineWrapper::initialize() {
    return app_->initialize();
}

void EngineWrapper::shutdown() {
    app_->shutdown();
}

bool EngineWrapper::start() {
    return app_->start();
}

void EngineWrapper::stop() {
    app_->stop();
}

std::string EngineWrapper::state() const {
    switch (app_->state()) {
        case EngineState::Running: return "running";
        case EngineState::Error: return "error";
        default: return "stopped";
    }
}

bool EngineWrapper::loadConfig(const std::string& name) {
    return app_->loadConfig(name);
}

bool EngineWrapper::saveConfig(const std::string& name) {
    return app_->saveConfig(name);
}

std::string EngineWrapper::getConfigJson() const {
    return JsonMappingParser::serialize(app_->profile());
}

bool EngineWrapper::setConfigJson(const std::string& json) {
    try {
        auto profile = JsonMappingParser::parseFromString(json);
        const auto errors = MappingValidator::validate(profile.mappings);
        if (!errors.empty()) return false;
        app_->setProfile(std::move(profile));
        return true;
    } catch (...) {
        return false;
    }
}

std::string EngineWrapper::validateMappingsJson(const std::string& json) const {
    try {
        const auto profile = JsonMappingParser::parseFromString(json);
        const auto errors = MappingValidator::validate(profile.mappings);
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& e : errors) {
            arr.push_back({{"field", e.field}, {"message", e.message}});
        }
        return arr.dump();
    } catch (const std::exception& ex) {
        return nlohmann::json::array({{{"field", "json"}, {"message", ex.what()}}}).dump();
    }
}

bool EngineWrapper::joystickConnected() const {
    return app_->joystickConnected();
}

std::string EngineWrapper::captureMode() const {
    return app_->captureMode();
}

bool EngineWrapper::keyboardAccessibilityGranted() {
    return app_->keyboardAccessibilityGranted();
}

std::string EngineWrapper::activeProfileName() const {
    return app_->activeProfileName();
}

std::string EngineWrapper::listProfilesJson() const {
    return nlohmann::json(app_->listProfiles()).dump();
}

bool EngineWrapper::switchProfile(const std::string& name) {
    return app_->switchProfile(name);
}

bool EngineWrapper::createProfile(const std::string& name, bool copyCurrent) {
    return app_->createProfile(name, copyCurrent);
}

bool EngineWrapper::deleteProfile(const std::string& name) {
    return app_->deleteProfile(name);
}

}  // namespace inputflow::napi_bridge
