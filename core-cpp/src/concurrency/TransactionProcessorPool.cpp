#include "omnipay/concurrency/TransactionProcessorPool.hpp"

namespace omnipay {

TransactionProcessorPool::TransactionProcessorPool(size_t threadCount, size_t queueCapacity)
    : queue_(queueCapacity), running_(true) {
    if (threadCount == 0) {
        threadCount = std::thread::hardware_concurrency();
        if (threadCount == 0) threadCount = 4;
    }

    threads_.reserve(threadCount);
    for (size_t i = 0; i < threadCount; ++i) {
        threads_.emplace_back(&TransactionProcessorPool::workerRoutine, this, i);
    }
}

TransactionProcessorPool::~TransactionProcessorPool() {
    shutdown();
}

void TransactionProcessorPool::shutdown() {
    bool expected = true;
    if (running_.compare_exchange_strong(expected, false)) {
        queue_.shutdown();

        for (auto& t : threads_) {
            if (t.joinable()) {
                t.join();
            }
        }
        threads_.clear();
    }
}

void TransactionProcessorPool::workerRoutine(size_t workerId) {
    (void)workerId;
    Task task;
    while (queue_.pop(task)) {
        try {
            task();
        } catch (...) {
            // Protect worker thread from unhandled exceptions
        }
        tasksCompleted_.fetch_add(1, std::memory_order_relaxed);
    }
}

} // namespace omnipay
