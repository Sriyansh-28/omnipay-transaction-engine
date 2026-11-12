#pragma once

#include "omnipay/concurrency/ConcurrentQueue.hpp"
#include <vector>
#include <thread>
#include <future>
#include <functional>
#include <memory>
#include <atomic>

namespace omnipay {

/**
 * @brief High-Throughput Worker Thread Pool for Transaction Processing.
 * Uses thread-safe bounded concurrent queues, atomic counters, and RAII lifecycle management.
 */
class TransactionProcessorPool {
public:
    using Task = std::function<void()>;

    explicit TransactionProcessorPool(
        size_t threadCount = 0, // 0 = std::thread::hardware_concurrency()
        size_t queueCapacity = 50000);

    ~TransactionProcessorPool();

    // Disable copy/move
    TransactionProcessorPool(const TransactionProcessorPool&) = delete;
    TransactionProcessorPool& operator=(const TransactionProcessorPool&) = delete;

    /**
     * @brief Submit a generic task and return a std::future for its result
     */
    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        using ReturnType = typename std::invoke_result<F, Args...>::type;

        auto taskPtr = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> fut = taskPtr->get_future();

        bool pushed = queue_.push([taskPtr]() {
            (*taskPtr)();
        });

        if (!pushed) {
            throw std::runtime_error("Cannot submit task: TransactionProcessorPool is shutting down");
        }

        tasksSubmitted_.fetch_add(1, std::memory_order_relaxed);
        return fut;
    }

    /**
     * @brief Gracefully wait for all pending queue items to drain and terminate threads
     */
    void shutdown();

    size_t getThreadCount() const { return threads_.size(); }
    size_t getQueueSize() const { return queue_.size(); }
    uint64_t getCompletedTaskCount() const { return tasksCompleted_.load(std::memory_order_relaxed); }
    uint64_t getSubmittedTaskCount() const { return tasksSubmitted_.load(std::memory_order_relaxed); }
    bool isRunning() const { return running_.load(std::memory_order_relaxed); }

private:
    void workerRoutine(size_t workerId);

    std::vector<std::thread> threads_;
    ConcurrentQueue<Task> queue_;
    std::atomic<bool> running_{true};
    std::atomic<uint64_t> tasksSubmitted_{0};
    std::atomic<uint64_t> tasksCompleted_{0};
};

} // namespace omnipay
