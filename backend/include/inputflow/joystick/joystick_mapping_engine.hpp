#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include "inputflow/types.hpp"
#include "inputflow/joystick/i_joystick_provider.hpp"
#include "inputflow/input/i_input_provider.hpp"

namespace inputflow {

/// Membaca joystick dan mengirim keyboard sesuai pemetaan user.
class JoystickMappingEngine {
public:
    JoystickMappingEngine(IJoystickProvider& joystick, IInputProvider& keyboard);
    ~JoystickMappingEngine();

    bool start();
    void stop();

    void setMappings(const std::vector<JoystickMapping>& mappings);
    void setCamera(const CameraControlSettings& camera);
    std::vector<JoystickMapping> mappings() const;

    bool isRunning() const;

private:
    void pollLoop();
    void processCamera(const JoystickState& state);
    void applyMapping(const JoystickMapping& m);
    void releaseMapping(const JoystickMapping& m);
    static float applyDeadzone(float v, float deadzone);

    IJoystickProvider& joystick_;
    IInputProvider& keyboard_;

    std::thread pollThread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stopRequested_{false};

    mutable std::mutex mutex_;
    std::vector<JoystickMapping> mappings_;
    std::unordered_map<std::string, bool> comboPrev_;
    std::unordered_map<std::string, bool> activeById_;
    CameraControlSettings camera_;
    bool cameraRightHeld_{false};
};

}  // namespace inputflow
