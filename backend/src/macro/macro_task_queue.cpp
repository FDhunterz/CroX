#include "inputflow/macro/macro_task_queue.hpp"

namespace inputflow {

MacroTaskQueue::MacroTaskQueue() = default;

MacroTaskQueue::~MacroTaskQueue() {
    stop();
}

void MacroTaskQueue::start() {
    if (running_.exchange(true)) return;
    stopRequested_ = false;
    worker_ = std::thread([this] { workerLoop(); });
}

void MacroTaskQueue::stop() {
    if (!running_.exchange(false)) return;
    stopRequested_ = true;
    cv_.notify_all();
    if (worker_.joinable()) worker_.join();
    clear();
}

void MacroTaskQueue::enqueue(Task task) {
    {
        std::lock_guard lock(mutex_);
        tasks_.push(std::move(task));
    }
    cv_.notify_one();
}

void MacroTaskQueue::clear() {
    std::lock_guard lock(mutex_);
    std::queue<Task> empty;
    std::swap(tasks_, empty);
}

size_t MacroTaskQueue::pendingCount() const {
    std::lock_guard lock(mutex_);
    return tasks_.size();
}

void MacroTaskQueue::workerLoop() {
    while (!stopRequested_) {
        Task task;
        {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, [this] {
                return stopRequested_ || !tasks_.empty();
            });
            if (stopRequested_ && tasks_.empty()) break;
            if (tasks_.empty()) continue;
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        if (task) task();
    }
}

}  // namespace inputflow
