#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>

namespace inputflow {

/// Thread-safe async task queue for macro execution and timed work.
class MacroTaskQueue {
public:
    using Task = std::function<void()>;

    MacroTaskQueue();
    ~MacroTaskQueue();

    MacroTaskQueue(const MacroTaskQueue&) = delete;
    MacroTaskQueue& operator=(const MacroTaskQueue&) = delete;

    void start();
    void stop();
    void enqueue(Task task);
    void clear();

    size_t pendingCount() const;

private:
    void workerLoop();

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<Task> tasks_;
    std::thread worker_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stopRequested_{false};
};

}  // namespace inputflow
