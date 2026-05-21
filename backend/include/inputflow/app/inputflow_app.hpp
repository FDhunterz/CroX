#pragma once

#include <memory>
#include <string>
#include <vector>
#include "inputflow/types.hpp"
#include "inputflow/joystick/joystick_mapping_engine.hpp"
#include "inputflow/config/mapping_validator.hpp"

namespace inputflow {

class InputFlowApp {
public:
    explicit InputFlowApp(std::string configDirectory);

    bool initialize();
    void shutdown();

    bool start();
    void stop();
    EngineState state() const;

    MappingProfile profile() const;
    void setProfile(MappingProfile profile);
    std::vector<ValidationError> setMappings(const std::vector<JoystickMapping>& mappings);

    bool loadConfig(const std::string& name = "default");
    bool saveConfig(const std::string& name = "default") const;

    std::string activeProfileName() const;
    std::vector<std::string> listProfiles() const;
    bool switchProfile(const std::string& name);
    bool createProfile(const std::string& name, bool copyCurrent = true);
    bool deleteProfile(const std::string& name);
    bool renameProfile(const std::string& from, const std::string& to);

    bool joystickConnected() const;
    /// "shared" (absorb/HID dinonaktifkan sementara)
    std::string captureMode() const;
    bool keyboardAccessibilityGranted();

private:
    bool rebuildJoystickPipeline();
    std::string configPath(const std::string& name) const;

    std::string configDirectory_;
    std::string activeProfile_{"default"};
    MappingProfile profile_;

    std::unique_ptr<IJoystickProvider> joystick_;
    std::unique_ptr<IInputProvider> keyboard_;
    bool keyboardTrusted_{false};
    std::unique_ptr<JoystickMappingEngine> engine_;

    EngineState state_{EngineState::Stopped};
    std::string captureMode_{"shared"};
};

}  // namespace inputflow
