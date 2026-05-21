#pragma once

#include <functional>
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include "inputflow/types.hpp"

namespace inputflow {

/// Lightweight in-process pub/sub for event-driven architecture.
class IEventBus {
public:
    using Handler = std::function<void(const void*)>;

    virtual ~IEventBus() = default;

    virtual void subscribe(const std::string& topic, Handler handler) = 0;
    virtual void publish(const std::string& topic, const void* payload) = 0;
    virtual void clear(const std::string& topic) = 0;
};

class EventBus final : public IEventBus {
public:
    void subscribe(const std::string& topic, Handler handler) override;
    void publish(const std::string& topic, const void* payload) override;
    void clear(const std::string& topic) override;

private:
    std::mutex mutex_;
    std::unordered_map<std::string, std::vector<Handler>> handlers_;
};

// Well-known topics
inline constexpr const char* TOPIC_KEY_EVENT = "key.event";
inline constexpr const char* TOPIC_LOG = "log.entry";
inline constexpr const char* TOPIC_STATE = "engine.state";
inline constexpr const char* TOPIC_MACRO_STARTED = "macro.started";
inline constexpr const char* TOPIC_MACRO_FINISHED = "macro.finished";
inline constexpr const char* TOPIC_MACRO_CANCELLED = "macro.cancelled";

}  // namespace inputflow
