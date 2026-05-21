#include "inputflow/app/inputflow_app.hpp"
#include "inputflow/config/json_mapping_parser.hpp"
#include <nlohmann/json.hpp>
#include "inputflow/config/mapping_combo.hpp"
#include "inputflow/config/profile_manifest.hpp"
#include "inputflow/input/i_input_provider.hpp"
#include <algorithm>
#include "inputflow/joystick/i_joystick_provider.hpp"
#include <filesystem>

#if defined(INPUTFLOW_PLATFORM_MACOS)
#include <ApplicationServices/ApplicationServices.h>
#endif

namespace fs = std::filesystem;

namespace inputflow {

InputFlowApp::InputFlowApp(std::string configDirectory)
    : configDirectory_(std::move(configDirectory)) {}

namespace {
bool checkMacAccessibility(bool prompt = false) {
#if defined(INPUTFLOW_PLATFORM_MACOS)
    if (!prompt) return AXIsProcessTrusted();
    const void* keys[] = {kAXTrustedCheckOptionPrompt};
    const void* vals[] = {kCFBooleanTrue};
    CFDictionaryRef opts = CFDictionaryCreate(
        kCFAllocatorDefault, keys, vals, 1,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    const bool ok = AXIsProcessTrustedWithOptions(opts);
    CFRelease(opts);
    return ok;
#else
    (void)prompt;
    return true;
#endif
}
}  // namespace

bool InputFlowApp::initialize() {
    keyboard_ = createPlatformInputProvider();
    if (!keyboard_->initialize()) return false;
    keyboard_->refreshAccessibility();
    keyboardTrusted_ = keyboard_->accessibilityGranted();

    profile_.exclusiveCapture = false;
    joystick_ = createPlatformJoystickProvider(false);
    engine_ = std::make_unique<JoystickMappingEngine>(*joystick_, *keyboard_);
    engine_->setMappings(profile_.mappings);
    engine_->setCamera(profile_.camera);
    captureMode_ = joystick_->captureMode();

    fs::create_directories(configDirectory_);
    ProfileManifestStore manifestStore(configDirectory_);
    auto manifest = manifestStore.load();
    activeProfile_ = manifest.active;

    if (!loadConfig(activeProfile_)) {
        if (!loadConfig("default")) {
            const auto bundled = fs::current_path() / "configs" / "default.json";
            if (fs::exists(bundled)) {
                try {
                    profile_ = JsonMappingParser::parseFromFile(bundled.string());
                    engine_->setMappings(profile_.mappings);
                    engine_->setCamera(profile_.camera);
                } catch (...) {}
            }
        }
        activeProfile_ = profile_.id.empty() ? "default" : profile_.id;
        saveConfig(activeProfile_);
    }

    manifest.active = activeProfile_;
    if (std::find(manifest.profiles.begin(), manifest.profiles.end(), activeProfile_) ==
        manifest.profiles.end()) {
        manifest.profiles.push_back(activeProfile_);
    }
    for (const auto& f : manifestStore.listProfileFiles()) {
        if (std::find(manifest.profiles.begin(), manifest.profiles.end(), f) ==
            manifest.profiles.end()) {
            manifest.profiles.push_back(f);
        }
    }
    manifestStore.save(manifest);
    return true;
}

void InputFlowApp::shutdown() {
    stop();
    if (engine_) engine_.reset();
    if (joystick_) {
        joystick_->shutdown();
        joystick_.reset();
    }
    if (keyboard_) keyboard_->shutdown();
}

bool InputFlowApp::rebuildJoystickPipeline() {
    // Hentikan & hancurkan engine DULU (jangan shutdown joystick yang sudah diganti)
    if (engine_) {
        engine_->stop();
        engine_.reset();
    }
    if (joystick_) {
        joystick_->shutdown();
        joystick_.reset();
    }

    profile_.exclusiveCapture = false;
    joystick_ = createPlatformJoystickProvider(false);
    if (!joystick_) return false;

    engine_ = std::make_unique<JoystickMappingEngine>(*joystick_, *keyboard_);
    engine_->setMappings(profile_.mappings);
    engine_->setCamera(profile_.camera);
    captureMode_ = joystick_->captureMode();
    return true;
}

bool InputFlowApp::start() {
    const auto errors = MappingValidator::validate(profile_.mappings);
    if (!errors.empty()) {
        state_ = EngineState::Error;
        return false;
    }
    if (!rebuildJoystickPipeline()) {
        state_ = EngineState::Error;
        return false;
    }
#if defined(INPUTFLOW_PLATFORM_MACOS)
    if (keyboard_) keyboard_->refreshAccessibility();
    keyboardTrusted_ = keyboard_ ? keyboard_->accessibilityGranted() : checkMacAccessibility();
    if (!keyboardTrusted_) {
        checkMacAccessibility(true);
        if (keyboard_) keyboard_->refreshAccessibility();
        keyboardTrusted_ = keyboard_ ? keyboard_->accessibilityGranted() : checkMacAccessibility();
    }
    if (!keyboardTrusted_) {
        state_ = EngineState::Error;
        return false;
    }
#else
    keyboardTrusted_ = true;
#endif
    if (!engine_->start()) {
        state_ = EngineState::Error;
        return false;
    }
    captureMode_ = joystick_->captureMode();
    state_ = EngineState::Running;
    return true;
}

void InputFlowApp::stop() {
    if (engine_) {
        engine_->stop();
        engine_.reset();
    }
    if (joystick_) {
        joystick_->shutdown();
        joystick_.reset();
    }
    if (keyboard_) {
        keyboard_->releaseAllHeld();
    }
    captureMode_ = "shared";
    state_ = EngineState::Stopped;
}

EngineState InputFlowApp::state() const {
    return state_;
}

MappingProfile InputFlowApp::profile() const {
    return profile_;
}

void InputFlowApp::setProfile(MappingProfile profile) {
    dedupeProfileMappings(profile.mappings);
    profile_ = std::move(profile);
    profile_.id = activeProfile_;
    if (engine_) {
        engine_->setMappings(profile_.mappings);
        engine_->setCamera(profile_.camera);
    }
    saveConfig(activeProfile_);
}

std::vector<ValidationError> InputFlowApp::setMappings(
    const std::vector<JoystickMapping>& mappings) {
    const auto errors = MappingValidator::validate(mappings);
    if (!errors.empty()) return errors;
    profile_.mappings = mappings;
    if (engine_) engine_->setMappings(mappings);
    return {};
}

bool InputFlowApp::loadConfig(const std::string& name) {
    const auto path = configPath(name);
    if (!fs::exists(path)) return false;
    profile_ = JsonMappingParser::parseFromFile(path);
    activeProfile_ = name;
    if (engine_) {
        engine_->setMappings(profile_.mappings);
        engine_->setCamera(profile_.camera);
    }
    return true;
}

bool InputFlowApp::saveConfig(const std::string& name) const {
    fs::create_directories(configDirectory_);
    return JsonMappingParser::saveToFile(profile_, configPath(name));
}

bool InputFlowApp::joystickConnected() const {
    return joystick_ && joystick_->isConnected();
}

std::string InputFlowApp::captureMode() const {
    return captureMode_;
}

bool InputFlowApp::keyboardAccessibilityGranted() {
    if (keyboard_) {
        keyboard_->refreshAccessibility();
        keyboardTrusted_ = keyboard_->accessibilityGranted();
    }
    return keyboardTrusted_;
}

std::string InputFlowApp::configPath(const std::string& name) const {
    return (fs::path(configDirectory_) / (name + ".json")).string();
}

}  // namespace inputflow
