#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <optional>

namespace omnipay {

/**
 * @brief Thread-Safe Bounded Blocking Concurrent Queue
 * Demonstrates lock-based concurrency, condition variables, and RAII.
 */
template <typename T>
class ConcurrentQueue {
public:
    explicit ConcurrentQueue(size_t capacity = 10000)
        : capacity_(capacity), shutdown_(false) {}

    ~ConcurrentQueue() {
        shutdown();
    }

    // Disable copy
    ConcurrentQueue(const ConcurrentQueue&) = delete;
    ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;

    /**
     * @brief Blocking push into the queue with backpressure when full
     * @return true if pushed, false if queue is shutting down
     */
    bool push(T item) {
        std::unique_lock<std::mutex> lock(mutex_);
        cvNotFull_.wait(lock, [this]() {
            return queue_.size() < capacity_ || shutdown_;
        });

        if (shutdown_) {
            return false;
        }

        queue_.push(std::move(item));
        cvNotEmpty_.notify_one();
        return true;
    }

    /**
     * @brief Timed push with timeout
     */
    bool pushWithTimeout(T item, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        bool ok = cvNotFull_.wait_for(lock, timeout, [this]() {
            return queue_.size() < capacity_ || shutdown_;
        });

        if (!ok || shutdown_) {
            return false;
        }

        queue_.push(std::move(item));
        cvNotEmpty_.notify_one();
        return true;
    }

    /**
     * @brief Blocking pop from the queue
     * @param outItem Output destination for popped item
     * @return true if popped, false if queue is empty and shutting down
     */
    bool pop(T& outItem) {
        std::unique_lock<std::mutex> lock(mutex_);
        cvNotEmpty_.wait(lock, [this]() {
            return !queue_.empty() || shutdown_;
        });

        if (queue_.empty() && shutdown_) {
            return false;
        }

        outItem = std::move(queue_.front());
        queue_.pop();
        cvNotFull_.notify_one();
        return true;
    }

    /**
     * @brief Non-blocking try-pop
     */
    std::optional<T> tryPop() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return std::nullopt;
        }

        T item = std::move(queue_.front());
        queue_.pop();
        cvNotFull_.notify_one();
        return item;
    }

    /**
     * @brief Signals queue shutdown and wakes all blocked threads
     */
    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            shutdown_ = true;
        }
        cvNotEmpty_.notify_all();
        cvNotFull_.notify_all();
    }

    bool isShutdown() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return shutdown_;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    size_t capacity() const {
        return capacity_;
    }

private:
    const size_t capacity_;
    bool shutdown_{false};
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cvNotEmpty_;
    std::condition_variable cvNotFull_;
};

} // namespace omnipay
