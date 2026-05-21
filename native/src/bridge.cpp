#include <napi.h>
#include "engine_wrapper.hpp"
#include <memory>

namespace {

std::unique_ptr<inputflow::napi_bridge::EngineWrapper> g_app;

Napi::Value Initialize(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    const std::string dir = info.Length() > 0 && info[0].IsString()
        ? info[0].As<Napi::String>().Utf8Value()
        : "./configs";
    if (g_app) g_app->shutdown();
    g_app = std::make_unique<inputflow::napi_bridge::EngineWrapper>(dir);
    return Napi::Boolean::New(env, g_app->initialize());
}

Napi::Value Start(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, g_app && g_app->start());
}

Napi::Value Stop(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (g_app) g_app->stop();
    return env.Undefined();
}

Napi::Value GetState(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::String::New(env, g_app ? g_app->state() : "stopped");
}

Napi::Value LoadConfig(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    const std::string name = info.Length() > 0 ? info[0].As<Napi::String>().Utf8Value() : "default";
    return Napi::Boolean::New(env, g_app && g_app->loadConfig(name));
}

Napi::Value SaveConfig(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    const std::string name = info.Length() > 0 ? info[0].As<Napi::String>().Utf8Value() : "default";
    return Napi::Boolean::New(env, g_app && g_app->saveConfig(name));
}

Napi::Value GetConfigJson(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::String::New(env, g_app ? g_app->getConfigJson() : "{}");
}

Napi::Value SetConfigJson(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (!g_app || info.Length() < 1) return Napi::Boolean::New(env, false);
    return Napi::Boolean::New(env, g_app->setConfigJson(info[0].As<Napi::String>().Utf8Value()));
}

Napi::Value ValidateConfig(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (!g_app || info.Length() < 1) return Napi::String::New(env, "[]");
    return Napi::String::New(env, g_app->validateMappingsJson(info[0].As<Napi::String>().Utf8Value()));
}

Napi::Value JoystickConnected(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, g_app && g_app->joystickConnected());
}

Napi::Value GetCaptureMode(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::String::New(env, g_app ? g_app->captureMode() : "shared");
}

Napi::Value KeyboardAccessibility(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, g_app && g_app->keyboardAccessibilityGranted());
}

Napi::Value GetActiveProfile(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::String::New(env, g_app ? g_app->activeProfileName() : "default");
}

Napi::Value ListProfiles(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::String::New(env, g_app ? g_app->listProfilesJson() : "[]");
}

Napi::Value SwitchProfile(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (!g_app || info.Length() < 1) return Napi::Boolean::New(env, false);
    return Napi::Boolean::New(env, g_app->switchProfile(info[0].As<Napi::String>().Utf8Value()));
}

Napi::Value CreateProfile(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (!g_app || info.Length() < 1) return Napi::Boolean::New(env, false);
    const bool copy = info.Length() >= 2 && info[1].As<Napi::Boolean>().Value();
    return Napi::Boolean::New(env, g_app->createProfile(info[0].As<Napi::String>().Utf8Value(), copy));
}

Napi::Value DeleteProfile(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (!g_app || info.Length() < 1) return Napi::Boolean::New(env, false);
    return Napi::Boolean::New(env, g_app->deleteProfile(info[0].As<Napi::String>().Utf8Value()));
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set("initialize", Napi::Function::New(env, Initialize));
    exports.Set("start", Napi::Function::New(env, Start));
    exports.Set("stop", Napi::Function::New(env, Stop));
    exports.Set("getState", Napi::Function::New(env, GetState));
    exports.Set("loadConfig", Napi::Function::New(env, LoadConfig));
    exports.Set("saveConfig", Napi::Function::New(env, SaveConfig));
    exports.Set("getConfigJson", Napi::Function::New(env, GetConfigJson));
    exports.Set("setConfigJson", Napi::Function::New(env, SetConfigJson));
    exports.Set("validateConfig", Napi::Function::New(env, ValidateConfig));
    exports.Set("joystickConnected", Napi::Function::New(env, JoystickConnected));
    exports.Set("getCaptureMode", Napi::Function::New(env, GetCaptureMode));
    exports.Set("keyboardAccessibility", Napi::Function::New(env, KeyboardAccessibility));
    exports.Set("getActiveProfile", Napi::Function::New(env, GetActiveProfile));
    exports.Set("listProfiles", Napi::Function::New(env, ListProfiles));
    exports.Set("switchProfile", Napi::Function::New(env, SwitchProfile));
    exports.Set("createProfile", Napi::Function::New(env, CreateProfile));
    exports.Set("deleteProfile", Napi::Function::New(env, DeleteProfile));
    return exports;
}

}  // namespace

NODE_API_MODULE(inputflow_native, Init)
