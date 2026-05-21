#include "inputflow/core/debounce_filter.hpp"

namespace inputflow {

void DebounceFilter::setDebounceMs(uint32_t ms) {
    std::lock_guard lock(mutex_);
    debounceMs_ = ms;
}

bool DebounceFilter::shouldProcess(const KeyEvent& event) {
    if (debounceMs_ == 0) return true;

    const uint64_t id = (static_cast<uint64_t>(event.key) << 8) |
                        static_cast<uint64_t>(event.action);

    const auto now = std::chrono::steady_clock::now();
    std::lock_guard lock(mutex_);
    auto it = lastSeen_.find(id);
    if (it != lastSeen_.end()) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second);
        if (elapsed.count() < static_cast<int64_t>(debounceMs_)) {
            return false;
        }
    }
    lastSeen_[id] = now;
    return true;
}

}  // namespace inputflow
