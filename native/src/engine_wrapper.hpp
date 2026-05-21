#pragma once

#include <memory>
#include <string>
#include <vector>
#include "inputflow/app/inputflow_app.hpp"

namespace inputflow::napi_bridge {

class EngineWrapper {
public:
    explicit EngineWrapper(std::string configDirectory);

    bool initialize();
    void shutdown();
    bool start();
    void stop();
    std::string state() const;

    bool loadConfig(const std::string& name);
    bool saveConfig(const std::string& name);
    std::string getConfigJson() const;
    bool setConfigJson(const std::string& json);
    std::string validateMappingsJson(const std::string& json) const;
    bool joystickConnected() const;
    std::string captureMode() const;
    bool keyboardAccessibilityGranted();

    std::string activeProfileName() const;
    std::string listProfilesJson() const;
    bool switchProfile(const std::string& name);
    bool createProfile(const std::string& name, bool copyCurrent);
    bool deleteProfile(const std::string& name);

private:
    std::unique_ptr<InputFlowApp> app_;
};

}  // namespace inputflow::napi_bridge
