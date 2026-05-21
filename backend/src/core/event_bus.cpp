#include "inputflow/core/i_event_bus.hpp"

namespace inputflow {

void EventBus::subscribe(const std::string& topic, Handler handler) {
    std::lock_guard lock(mutex_);
    handlers_[topic].push_back(std::move(handler));
}

void EventBus::publish(const std::string& topic, const void* payload) {
    std::vector<Handler> copy;
    {
        std::lock_guard lock(mutex_);
        auto it = handlers_.find(topic);
        if (it == handlers_.end()) return;
        copy = it->second;
    }
    for (const auto& h : copy) {
        if (h) h(payload);
    }
}

void EventBus::clear(const std::string& topic) {
    std::lock_guard lock(mutex_);
    handlers_.erase(topic);
}

}  // namespace inputflow
