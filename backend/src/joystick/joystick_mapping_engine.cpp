#include "inputflow/joystick/joystick_mapping_engine.hpp"
#include "inputflow/config/mapping_combo.hpp"
#include <chrono>
#include <cmath>
#include <thread>

namespace inputflow {

JoystickMappingEngine::JoystickMappingEngine(IJoystickProvider& joystick, IInputProvider& keyboard)
    : joystick_(joystick), keyboard_(keyboard) {}

JoystickMappingEngine::~JoystickMappingEngine() {
    stop();
}

bool JoystickMappingEngine::start() {
    if (running_.load()) return true;

    stopRequested_ = false;
    if (!joystick_.initialize()) return false;

    running_ = true;
    pollThread_ = std::thread([this] { pollLoop(); });
    return true;
}

void JoystickMappingEngine::stop() {
    stopRequested_ = true;
    if (pollThread_.joinable()) pollThread_.join();
    running_ = false;

    std::lock_guard lock(mutex_);
    for (const auto& m : mappings_) {
        if (activeById_[m.id]) releaseMapping(m);
    }
    activeById_.clear();
    comboPrev_.clear();
    keyboard_.releaseCameraMouse();
    cameraRightHeld_ = false;
    keyboard_.releaseAllHeld();
}

void JoystickMappingEngine::setMappings(const std::vector<JoystickMapping>& mappings) {
    std::lock_guard lock(mutex_);
    for (const auto& m : mappings_) {
        if (activeById_[m.id]) releaseMapping(m);
    }
    mappings_ = mappings;
    for (auto& m : mappings_) normalizeMappingButtons(m);
    activeById_.clear();
    comboPrev_.clear();
}

void JoystickMappingEngine::setCamera(const CameraControlSettings& camera) {
    std::lock_guard lock(mutex_);
    camera_ = camera;
}

std::vector<JoystickMapping> JoystickMappingEngine::mappings() const {
    std::lock_guard lock(mutex_);
    return mappings_;
}

bool JoystickMappingEngine::isRunning() const {
    return running_.load();
}

float JoystickMappingEngine::applyDeadzone(float v, float deadzone) {
    if (std::fabs(v) < deadzone) return 0.f;
    const float sign = v < 0.f ? -1.f : 1.f;
    return sign * (std::fabs(v) - deadzone) / (1.f - deadzone);
}

void JoystickMappingEngine::pollLoop() {
    while (!stopRequested_) {
        JoystickState state;
        if (!joystick_.poll(state)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }

        CameraControlSettings camera;
        std::vector<JoystickMapping> maps;
        {
            std::lock_guard lock(mutex_);
            camera = camera_;
            maps = mappings_;
        }

        if (camera.enabled) processCamera(state);

        for (const auto& m : maps) {
            if (!m.enabled) continue;
            const bool now = allComboPressed(m, state);
            const bool was = comboPrev_[m.id];
            if (now && !was) applyMapping(m);
            else if (!now && was) releaseMapping(m);
            comboPrev_[m.id] = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
}

void JoystickMappingEngine::processCamera(const JoystickState& state) {
    CameraControlSettings camera;
    {
        std::lock_guard lock(mutex_);
        camera = camera_;
    }
    if (!camera.enabled) return;

    const bool useRight = camera.stick != "left";
    float sx = useRight ? state.rx : state.lx;
    float sy = useRight ? state.ry : state.ly;
    sx = applyDeadzone(sx, camera.deadzone);
    sy = applyDeadzone(sy, camera.deadzone);
    if (camera.invertY) sy = -sy;

    const bool active = std::fabs(sx) > 0.001f || std::fabs(sy) > 0.001f;

    if (camera.requireRightButton) {
        if (active && !cameraRightHeld_) {
            keyboard_.rightMouseDown();
            cameraRightHeld_ = true;
        } else if (!active && cameraRightHeld_) {
            keyboard_.rightMouseUp();
            cameraRightHeld_ = false;
        }
        if (!active) return;
    }

    const int dx = static_cast<int>(sx * camera.sensitivity);
    const int dy = static_cast<int>(sy * camera.sensitivity);
    if (dx != 0 || dy != 0) keyboard_.moveMouseRelative(dx, dy);
}

void JoystickMappingEngine::applyMapping(const JoystickMapping& m) {
    if (m.hasKey) keyboard_.sendKeyDown(m.key, m.modifiers);
    else keyboard_.sendModifiersDown(m.modifiers);
    activeById_[m.id] = true;
}

void JoystickMappingEngine::releaseMapping(const JoystickMapping& m) {
    if (m.hasKey) keyboard_.sendKeyUp(m.key, m.modifiers);
    else keyboard_.sendModifiersUp(m.modifiers);
    activeById_[m.id] = false;
}

}  // namespace inputflow
