#pragma once

#include <chrono>
#include <mutex>
#include <unordered_map>
#include "inputflow/types.hpp"

namespace inputflow {

class DebounceFilter {
public:
    void setDebounceMs(uint32_t ms);
    bool shouldProcess(const KeyEvent& event);

private:
    std::mutex mutex_;
    uint32_t debounceMs_{50};
    std::unordered_map<uint64_t, std::chrono::steady_clock::time_point> lastSeen_;
};

}  // namespace inputflow
